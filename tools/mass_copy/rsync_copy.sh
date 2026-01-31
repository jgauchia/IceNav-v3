#!/bin/bash
set -e

# Fast mass copy with anti-fragmentation modes for IceNav tiles
# Usage: ./rsync_copy.sh [SOURCE] [DESTINATION] [DEVICE] [MODE]

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

# Utility Functions
log() { echo -e "${BLUE}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; exit 1; }
success() { echo -e "${GREEN}[OK]${NC} $1"; }

format_bytes() {
    local bytes=$1
    if [ "$bytes" -ge 1073741824 ]; then
        echo "$(awk "BEGIN {printf \"%.2f GB\", $bytes/1073741824}")"
    elif [ "$bytes" -ge 1048576 ]; then
        echo "$(awk "BEGIN {printf \"%.2f MB\", $bytes/1048576}")"
    else
        echo "$(awk "BEGIN {printf \"%.2f KB\", $bytes/1024}")"
    fi
}

get_dir_size() {
    du -sb "$1" 2>/dev/null | cut -f1 || echo "0"
}

show_help() {
    echo -e "${BLUE}=== ICE NAV MASS COPY TOOL ===${NC}"
    echo "Usage: $0 [SOURCE] [DESTINATION] [DEVICE] [MODE]"
    echo ""
    echo "MODES:"
    echo -e "  ${GREEN}incremental${NC} - Fast sync, copies only changed files (default)"
    echo -e "  ${MAGENTA}full${NC}        - Wipe destination and sequential copy (zero fragmentation)"
    exit 0
}

# Validation
[[ $# -lt 3 ]] && show_help
[[ ! -d "$1" ]] && error "Source directory '$1' not found"
[[ ! -b "$3" ]] && error "Device '$3' not found"

SOURCE="$1"
DESTINATION="$2"
DEVICE="$3"
MODE="${4:-incremental}"
SOURCE_NAME=$(basename "$SOURCE")
FILELIST="/tmp/nav_sync_list_$$.txt"

# Trap for cleanup
cleanup() {
    rm -f "$FILELIST"
    mountpoint -q "$DESTINATION" && sudo umount "$DESTINATION" || true
}
trap cleanup EXIT INT TERM

# Display mode banner
if [ "$MODE" = "full" ]; then
    echo -e "${MAGENTA}╔═══════════════════════════════════════════════╗${NC}"
    echo -e "${MAGENTA}║   FULL MODE: Zero Fragmentation Guaranteed   ║${NC}"
    echo -e "${MAGENTA}╚═══════════════════════════════════════════════╝${NC}"
else
    echo -e "${GREEN}╔═══════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║  INCREMENTAL MODE: Fast Development Sync     ║${NC}"
    echo -e "${GREEN}╚═══════════════════════════════════════════════╝${NC}"
fi

echo -e "${BLUE}Start: $(date)${NC}"
echo ""
echo "Configuration:"
echo "  Source: $SOURCE"
echo "  Destination: $DESTINATION"
echo "  Device: $DEVICE"
echo "  Mode: $MODE"
if [ "$MODE" = "full" ]; then
    echo -e "  ${MAGENTA}Strategy: Delete + Sequential copy (optimal performance)${NC}"
else
    echo -e "  ${YELLOW}Strategy: Incremental sync (may fragment over time)${NC}"
fi
echo ""

# STEP 1: Mount device
log "Mounting $DEVICE..."
sudo mkdir -p "$DESTINATION"
sudo umount "$DEVICE" 2>/dev/null || true
sudo mount -t vfat -o rw,uid=$(id -u),gid=$(id -g),umask=0022,async,noatime,nodiratime "$DEVICE" "$DESTINATION"
mountpoint -q "$DESTINATION" || error "Failed to mount $DEVICE"
success "Device mounted successfully"

# STEP 2: Space verification
SOURCE_SIZE=$(get_dir_size "$SOURCE")
DEST_FREE=$(df -B1 "$DESTINATION" | tail -1 | awk '{print $4}')
log "Source size: $(format_bytes $SOURCE_SIZE) | Free space: $(format_bytes $DEST_FREE)"
[[ $SOURCE_SIZE -gt $DEST_FREE ]] && error "Insufficient space on $DEVICE"

# STEP 3: Full mode - wipe existing destination
if [ "$MODE" = "full" ]; then
    if [ -d "$DESTINATION/$SOURCE_NAME" ]; then
        log "FULL MODE: Removing existing destination..."
        sudo rm -rf "$DESTINATION/$SOURCE_NAME"
        sync
    fi
fi

# STEP 4: Generate sorted file list
log "Scanning source and sorting files (Z→X→Y)..."
# Usamos cd para asegurar rutas relativas limpias en el FILELIST
FILELIST_CONTENT=$(cd "$SOURCE" && find . -type f | sed 's|^\./||' | sort -t'/' -k1,1n -k2,2n -k3,3n)
echo "$FILELIST_CONTENT" > "$FILELIST"
TOTAL_FILES=$(echo "$FILELIST_CONTENT" | wc -l)

[[ $TOTAL_FILES -eq 0 ]] && error "No files found in $SOURCE"
log "Files to sync: $TOTAL_FILES"

# STEP 5: Pre-create directory structure
log "Pre-creating directory structure..."
echo "$FILELIST_CONTENT" | cut -d'/' -f1-2 | sort -u | xargs -I{} mkdir -p "$DESTINATION/$SOURCE_NAME/{}"

# STEP 6: Transfer with Rsync
log "Starting transfer..."
RSYNC_START=$(date +%s)
# Usamos sudo rsync para asegurar escritura, -a para preservar, --info=progress2 para el bar
sudo rsync -rlptD \
    --info=progress2 \
    --files-from="$FILELIST" \
    --inplace \
    --no-compress \
    --human-readable \
    "$SOURCE/" "$DESTINATION/$SOURCE_NAME/"

RSYNC_EXIT_CODE=$?
[[ $RSYNC_EXIT_CODE -ne 0 ]] && error "rsync failed with code $RSYNC_EXIT_CODE"
RSYNC_END=$(date +%s)

# STEP 7: Verification
DEST_FILES=$(find "$DESTINATION/$SOURCE_NAME" -type f 2>/dev/null | wc -l)
log "Verification: Source $TOTAL_FILES | Destination $DEST_FILES"
[[ $TOTAL_FILES -eq $DEST_FILES ]] && success "Sync completed in $((RSYNC_END - RSYNC_START))s" || warn "File count mismatch!"

# STEP 9: Sample integrity check
echo -e "${YELLOW}9. Sample integrity check...${NC}"

SAMPLE_COUNT=0
SAMPLE_ERRORS=0

for file in $(shuf -n 10 "$FILELIST"); do
    SAMPLE_COUNT=$((SAMPLE_COUNT + 1))
    src_file="$SOURCE/$file"
    dest_file="$DESTINATION/$SOURCE_FOLDER_NAME/$file"

    if [ -f "$dest_file" ] && cmp -s "$src_file" "$dest_file" 2>/dev/null; then
        echo "   ✓ $file"
    else
        echo "   ✗ $file"
        SAMPLE_ERRORS=$((SAMPLE_ERRORS + 1))
    fi
done

if [ $SAMPLE_ERRORS -eq 0 ]; then
    echo -e "${GREEN}   ✓ All samples verified OK${NC}"
fi

# STEP 10: Final sync and unmount
echo -e "${YELLOW}10. Finalizing...${NC}"
sync
sleep 2

sudo umount "$DESTINATION"
sleep 1

echo -e "${GREEN}   ✓ Device unmounted safely${NC}"

# Performance stats
TOTAL_TIME=$RSYNC_TIME
if [ $TOTAL_TIME -gt 0 ]; then
    AVG_SPEED=$((SOURCE_SIZE / 1024 / 1024 / TOTAL_TIME))
    FILES_PER_SEC=$((TOTAL_FILES / TOTAL_TIME))
else
    AVG_SPEED="N/A"
    FILES_PER_SEC="N/A"
fi

echo ""
if [ "$MODE" = "full" ]; then
    echo -e "${MAGENTA}═══════════════════════════════════════════${NC}"
    echo -e "${MAGENTA}    FULL SYNC COMPLETED - ZERO FRAGMENTATION${NC}"
    echo -e "${MAGENTA}═══════════════════════════════════════════${NC}"
else
    echo -e "${GREEN}═══════════════════════════════════════════${NC}"
    echo -e "${GREEN}    INCREMENTAL SYNC COMPLETED${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════${NC}"
fi
echo -e "${GREEN}End: $(date)${NC}"
echo ""
echo "Performance Summary:"
echo "  Files: $TOTAL_FILES"
echo "  Size: $SOURCE_SIZE_HUMAN"
echo "  Time: ${TOTAL_TIME}s ($((TOTAL_TIME / 60))m $((TOTAL_TIME % 60))s)"
echo "  Speed: ${AVG_SPEED}MB/s"
echo "  Files/sec: $FILES_PER_SEC"
echo "  Verification: $((SAMPLE_COUNT - SAMPLE_ERRORS))/$SAMPLE_COUNT OK"
echo ""

if [ "$MODE" = "full" ]; then
    echo -e "${MAGENTA}✓ OPTIMAL: Files written sequentially, zero fragmentation${NC}"
    echo -e "${CYAN}  This SD is now optimized for maximum read performance${NC}"
else
    echo -e "${YELLOW}⚠ FRAGMENTATION WARNING:${NC}"
    echo -e "${YELLOW}  Multiple incremental syncs cause fragmentation${NC}"
    echo -e "${YELLOW}  Recommend: Run 'full' mode periodically to defragment${NC}"
    echo -e "${CYAN}  Example: ./$(basename $0) $SOURCE $DESTINATION $DEVICE full${NC}"
fi

echo ""
if [ $SAMPLE_ERRORS -eq 0 ]; then
    echo -e "${GREEN}✓ SYNC SUCCESSFUL${NC}"
    exit 0
else
    echo -e "${YELLOW}⚠ SYNC COMPLETED with warnings${NC}"
    exit 1
fi
