#!/usr/bin/env python3
"""Build the per-zoom PMTiles extracts the firmware reads from the SD card.

Each file holds one zoom level, named Z{zoom}.pmtiles, and the firmware opens
the one that matches the map zoom. The map draws a 3x3 grid of 256 px tiles,
so an extract smaller than that grid, or one that does not contain the tile
under the position, leaves part of the screen with nothing to paint: at z6 the
region covers 1x2 tiles and at z8 exactly 3x3, which is why the top row of the
screen went blank while panning.

The bounding box of every zoom is therefore computed in tile units, not in
degrees: it is the region's own tile block widened, when needed, to a square
block around the centre tile. The box is written on exact tile edges, so the
extract holds exactly that block and no neighbour row slips in. The region is
never shrunk, so the zooms that already covered the screen keep the same
coverage.

The side of that block follows the board, not a constant: Maps::initMap()
grows its grid one tile at a time until the largest screen dimension fits with
half a tile of scroll margin on each side, so a 320x480 S3 asks for a 3x3 grid
and a 1024 px P4 for a 5x5 one. Pass --screen with the largest side in pixels
and the script derives it with the same formula, or set --grid to skip the
derivation.

Around that grid the extract keeps a ring of pan room, because the screen is not
always showing the tile the position sits on: the ring is what the map scrolls
into before the archive ends. It is wider at the low zooms, where the region is
a tile or two and a drag crosses the whole thing, and thins to one tile from z10.
On top of the ring the region itself is padded, so a region wider than the grid
still has tiles to scroll into, which is what the union did not do.

The planet archive is a daily build with a predictable name, so the source is
not part of the command: it is https://build.protomaps.com/YYYYMMDD.pmtiles for
today, falling back to the days before when today's is not published yet or does
not answer. --date pins one day, --src or the PMTILES environment variable skip
the calculation, and any other archive that answers range requests works the
same way.

Dry run by default: it prints the tiles and the command without touching the
card or the network. Pass --run to execute.
"""

import argparse
import datetime
import math
import os
import re
import subprocess
import sys
import time

BUILD_BASE = "https://build.protomaps.com"


def lon2tilex(lon, zoom):
    return (lon + 180.0) / 360.0 * (1 << zoom)


def lat2tiley(lat, zoom):
    rad = math.radians(lat)
    return (1.0 - math.log(math.tan(rad) + 1.0 / math.cos(rad)) / math.pi) / 2.0 * (1 << zoom)


def tilex2lon(tilex, zoom):
    return tilex / (1 << zoom) * 360.0 - 180.0


def tiley2lat(tiley, zoom):
    n = math.pi - 2.0 * math.pi * tiley / (1 << zoom)
    return math.degrees(math.atan(math.sinh(n)))


def parse_range(text):
    """Zoom list: 8, "6-15" or a mix such as "6,7,9-11"."""
    zooms = []
    for chunk in text.split(","):
        parts = chunk.split("-")
        if len(parts) == 1:
            zooms.append(int(parts[0]))
        else:
            zooms.extend(range(int(parts[0]), int(parts[1]) + 1))
    return zooms


def parse_floats(text):
    return [float(value) for value in text.split(",")]


def human_bytes(size):
    if size < 1024:
        return "%d B" % size
    for unit in ("KB", "MB"):
        size /= 1024.0
        if size < 1024.0:
            return "%.1f %s" % (size, unit)
    return "%.2f GB" % (size / 1024.0)


LOG_LINE = re.compile(r"^\d{4}/\d{2}/\d{2} \d{2}:\d{2}:\d{2} ")


def stream_extract(command):
    """Run one extract, keeping its progress bar and dropping its log lines.

    The binary reports how long it took, how many requests it made and how much
    it overfetched, which the summary line below already says in one place. The
    lines that explain a failure are kept, and the bar is forwarded as it comes,
    carriage returns and all, so it redraws in place instead of scrolling.
    """
    process = subprocess.Popen(command, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, bufsize=0)
    buffered = b""
    while True:
        chunk = process.stdout.read(1024)
        if not chunk:
            break
        buffered += chunk
        while True:
            cuts = [pos for pos in (buffered.find(b"\r"), buffered.find(b"\n")) if pos >= 0]
            if not cuts:
                break
            cut = min(cuts)
            unit, buffered = buffered[:cut + 1], buffered[cut + 1:]
            text = unit.rstrip(b"\r\n").decode("utf-8", "replace")
            if LOG_LINE.match(text):
                if "fail" in text.lower() or "error" in text.lower():
                    sys.stderr.write("       %s\n" % text)
                    sys.stderr.flush()
                continue
            sys.stdout.write(unit.decode("utf-8", "replace"))
            sys.stdout.flush()
    return process.wait()


def tiles_grid_for(max_screen_dim, tile_px=256):
    """Same rule as Maps::initMap(): how many tiles the sprite grid holds."""
    grid = 3
    while math.trunc((grid * tile_px - max_screen_dim) / 2) < tile_px // 2:
        grid += 1
    return grid


def margin_for(zoom):
    """Tiles of pan room on every side of the region.

    A drag across the screen moves about two tiles, and the ring is what the map
    can pan into before the archive runs out. The region is one tile wide at z6
    and a handful up to z9, so there a single tile of ring is half a screen and
    the edge shows as soon as the finger moves: the low zooms get a wider one,
    which costs a few hundred tiles, and from z10 one tile is enough.
    """
    return max(1, min(5, 11 - zoom))


def block_for(zoom, region, center, min_tiles, margin):
    min_lon, min_lat, max_lon, max_lat = region
    lat, lon = center
    # The region is padded, never replaced: without the pad the block is exactly
    # the region whenever the region is wider than the grid, which is every
    # zoom from z10 up, and the map has no ring to pan into at all.
    x0 = int(math.floor(lon2tilex(min_lon, zoom))) - margin
    x1 = int(math.ceil(lon2tilex(max_lon, zoom))) - 1 + margin
    y0 = int(math.floor(lat2tiley(max_lat, zoom))) - margin
    y1 = int(math.ceil(lat2tiley(min_lat, zoom))) - 1 + margin
    # Centred like the renderer does it: tilesGrid/2 to the left and the rest
    # to the right, so an even grid still lands under the position.
    half = min_tiles // 2
    cx = int(lon2tilex(lon, zoom))
    cy = int(lat2tiley(lat, zoom))
    x0 = min(x0, cx - half)
    x1 = max(x1, cx + (min_tiles - 1 - half))
    y0 = min(y0, cy - half)
    y1 = max(y1, cy + (min_tiles - 1 - half))
    return x0, y0, x1, y1


def bbox_for(block, zoom):
    # The CLI keeps every tile the box touches, so the outer edges sit just
    # inside the tile boundary instead of exactly on it.
    x0, y0, x1, y1 = block
    inset = 1e-9
    return (tilex2lon(x0, zoom) + inset,
            tiley2lat(y1 + 1, zoom) + inset,
            tilex2lon(x1 + 1, zoom) - inset,
            tiley2lat(y0, zoom) - inset)


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--src", default=os.environ.get("PMTILES"),
                        help="planet archive or URL, overrides the daily build calculation")
    parser.add_argument("--date", default=None, metavar="YYYYMMDD",
                        help="build day to read from (default: today, then the days before)")
    parser.add_argument("--days-back", type=int, default=2,
                        help="days before today kept as fallback sources (default 2)")
    parser.add_argument("--outdir", default="pmtiles", help="directory for the per-zoom files")
    parser.add_argument("--region", "--bbox", dest="region", default="0.15,40.52,3.34,42.87",
                        help="min_lon,min_lat,max_lon,max_lat of the region to keep")
    parser.add_argument("--center", default=None,
                        help="lat,lon the map boots at; defaults to the centre of --region")
    parser.add_argument("--screen", type=int, default=480, metavar="PX",
                        help="largest screen dimension, derives the grid like initMap (default 480)")
    parser.add_argument("--grid", type=int, default=None,
                        help="tiles per side the renderer draws, overriding --screen")
    parser.add_argument("--min-tiles", type=int, default=None,
                        help="side of the guaranteed tile block, overriding the ring (default grid+2xring)")
    parser.add_argument("--zooms", default="6-15", help="zoom levels: 8, 6-15 or 6,7,9-11")
    parser.add_argument("--pmtiles", default="./pmtiles", help="path to the pmtiles binary")
    parser.add_argument("--retries", type=int, default=2,
                        help="attempts after a failed extract, with a growing delay (default 2)")
    parser.add_argument("--run", action="store_true", help="run the extracts instead of printing them")
    args = parser.parse_args()

    region = parse_floats(args.region)
    if len(region) != 4:
        parser.error("--region is min_lon,min_lat,max_lon,max_lat")
    if args.center is None:
        center = ((region[1] + region[3]) / 2.0, (region[0] + region[2]) / 2.0)
    else:
        center = parse_floats(args.center)
    zooms = parse_range(args.zooms)
    grid = args.grid if args.grid is not None else tiles_grid_for(args.screen)
    print("grid %dx%d for a %d px screen" % (grid, grid, args.screen))

    if args.src:
        sources = [args.src]
    else:
        if args.date:
            try:
                first = datetime.datetime.strptime(args.date, "%Y%m%d").date()
            except ValueError:
                parser.error("--date wants YYYYMMDD")
            days = [first - datetime.timedelta(days=back) for back in range(args.days_back + 1)]
        else:
            today = datetime.date.today()
            days = [today - datetime.timedelta(days=back) for back in range(args.days_back + 1)]
        sources = ["%s/%s.pmtiles" % (BUILD_BASE, day.strftime("%Y%m%d")) for day in days]
    if len(sources) > 1:
        print("source: %s (then %s)" % (sources[0], ", ".join(sources[1:])))
    else:
        print("source: %s" % sources[0])
    source_index = 0

    total_tiles = 0
    total_bytes = 0
    started_all = time.monotonic()
    for index, zoom in enumerate(zooms, start=1):
        margin = margin_for(zoom)
        min_tiles = args.min_tiles if args.min_tiles is not None else grid + 2 * margin
        block = block_for(zoom, region, center, min_tiles, margin)
        min_lon, min_lat, max_lon, max_lat = bbox_for(block, zoom)
        width = block[2] - block[0] + 1
        height = block[3] - block[1] + 1
        tiles = width * height
        total_tiles += tiles
        output = os.path.join(args.outdir, "Z%d.pmtiles" % zoom)
        bbox_arg = "--bbox=%.9f,%.9f,%.9f,%.9f" % (min_lon, min_lat, max_lon, max_lat)
        command = [args.pmtiles, "extract", sources[source_index], output, bbox_arg,
                   "--minzoom=%d" % zoom,
                   "--maxzoom=%d" % zoom]
        print("[%d/%d] z%-2d %dx%-3d %6d tiles  x %d..%d  y %d..%d" %
              (index, len(zooms), zoom, width, height, tiles,
               block[0], block[2], block[1], block[3]), flush=True)
        if not args.run:
            # In a dry run the command is the output; when extracting it is
            # noise, the per-zoom line and the summary already say what ran.
            print("       %s" % " ".join(command), flush=True)
            continue
        os.makedirs(args.outdir, exist_ok=True)
        sys.stdout.flush()
        # The planet archive is read over HTTP, so a TLS handshake or a reset
        # connection is a normal failure: retry the same zoom before giving up,
        # otherwise one bad minute throws away the zooms that already worked.
        # When a day's build keeps failing, move to the day before for this and
        # the remaining zooms: a missing or flapping build must not cost the run.
        code = 0
        elapsed = 0.0
        while True:
            command = [args.pmtiles, "extract", sources[source_index], output, bbox_arg,
                       "--minzoom=%d" % zoom,
                       "--maxzoom=%d" % zoom]
            attempt = 0
            while attempt <= args.retries:
                started = time.monotonic()
                code = stream_extract(command)
                elapsed = time.monotonic() - started
                if code == 0:
                    break
                attempt += 1
                if attempt <= args.retries:
                    delay = 5 * attempt
                    print("       attempt %d of %d failed (exit %d), retrying in %d s" %
                          (attempt, args.retries + 1, code, delay), flush=True)
                    time.sleep(delay)
            if code == 0:
                break
            if source_index + 1 < len(sources):
                print("       %s did not answer, switching to %s" %
                      (sources[source_index], sources[source_index + 1]), flush=True)
                source_index += 1
                continue
            break
        if code != 0:
            print("[%d/%d] z%d failed with exit code %d after %.1f s" %
                  (index, len(zooms), zoom, code, elapsed), file=sys.stderr)
            return code
        size = os.path.getsize(output) if os.path.exists(output) else 0
        total_bytes += size
        print("       done in %.1f s · %s · total %s" %
              (elapsed, human_bytes(size), human_bytes(total_bytes)), flush=True)

    if args.run:
        print("%d files · %d tiles · %s in %.1f s" %
              (len(zooms), total_tiles, human_bytes(total_bytes),
               time.monotonic() - started_all), flush=True)
    else:
        print("%d files · %d tiles · dry run (--run to extract)" %
              (len(zooms), total_tiles), flush=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except BrokenPipeError:
        # Piping into head or less closes stdout early; that is not a failure.
        os._exit(0)
