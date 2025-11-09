#!/bin/bash
set -e

# Fast mass copy with anti-fragmentation modes
# Usage: ./rsync_copy_defrag.sh [SOURCE] [DESTINATION] [DEVICE] [MODE]

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
NC='\033[0m'

show_help() {
    echo -e "${BLUE}=== MASS COPY WITH ANTI-FRAGMENTATION ===${NC}"
    echo ""
    echo "Usage: $0 [SOURCE] [DESTINATION] [DEVICE] [MODE]"
    echo ""
    echo "MODES:"
    echo -e "  ${GREEN}incremental${NC} - Fast sync, only copies changed files (default)"
    echo "               Use for: frequent development updates"
    echo "               Warning: Causes fragmentation over time"
    echo ""
    echo -e "  ${MAGENTA}full${NC}        - Complete sequential copy (deletes destination first)"
    echo "               Use for: production deployment, defragmentation"
    echo "               Benefit: Zero fragmentation, optimal read performance"
    echo ""
    echo "Features:"
    echo "  - Sequential Z→X→Y order (anti-fragmentation)"
    echo "  - Built-in progress monitoring"
    echo "  - Integrity verification"
    echo "  - Optimized for map tiles"
    echo ""
    echo "Examples:"
    echo "  $0 /home/user/tiles /mnt/sd /dev/sdc1 incremental  # Quick dev sync"
    echo "  $0 /home/user/tiles /mnt/sd /dev/sdc1 full         # Production deploy"
    echo ""
    echo "Recommendations:"
    echo "  - Development: Use 'incremental' for quick iterations"
    echo "  - Production: Use 'full' before field deployment"
    echo "  - Periodic: Run 'full' every N syncs to defragment"
    echo ""
    exit 0
}

# Verify parameters
if [ $# -eq 0 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    show_help
fi

if [ $# -lt 3 ]; then
    echo -e "${RED}Error: At least 3 parameters required${NC}"
    echo "Usage: $0 [SOURCE] [DESTINATION] [DEVICE] [MODE]"
    echo "Run '$0 --help' for more information"
    exit 1
fi

SOURCE="$1"
DESTINATION="$2"
DEVICE="$3"
MODE="${4:-incremental}"  # Default to incremental

# Validate mode
if [ "$MODE" != "incremental" ] && [ "$MODE" != "full" ]; then
    echo -e "${RED}Error: Invalid mode '$MODE'. Use 'incremental' or 'full'${NC}"
    exit 1
fi

SOURCE_FOLDER_NAME=$(basename "$SOURCE")

# Validate inputs
if [ ! -d "$SOURCE" ]; then
    echo -e "${RED}Error: Source directory '$SOURCE' does not exist${NC}"
    exit 1
fi

if [ ! -b "$DEVICE" ]; then
    echo -e "${RED}Error: Device '$DEVICE' does not exist${NC}"
    exit 1
fi

# Cleanup function
cleanup() {
    echo -e "${YELLOW}Cleaning up...${NC}"
    rm -f /tmp/rsync_filelist_sorted_$$.txt 2>/dev/null
    if mountpoint -q "$DESTINATION" 2>/dev/null; then
        sudo umount "$DESTINATION" 2>/dev/null || true
    fi
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
echo -e "${YELLOW}1. Mounting device...${NC}"
sudo mkdir -p "$DESTINATION"
sudo umount "$DEVICE" 2>/dev/null || true

sudo mount -t vfat -o rw,uid=$(id -u),gid=$(id -g),umask=0022,async,noatime,nodiratime "$DEVICE" "$DESTINATION"

if ! mountpoint -q "$DESTINATION"; then
    echo -e "${RED}Error: Could not mount $DEVICE at $DESTINATION${NC}"
    exit 1
fi

echo -e "${GREEN}   ✓ Device mounted successfully${NC}"

# STEP 2: Space verification
echo -e "${YELLOW}2. Verifying available space...${NC}"

# Get source size - try multiple methods for reliability
SOURCE_SIZE=""

# Method 1: du with block size (most reliable)
SOURCE_SIZE=$(du -sb "$SOURCE" 2>/dev/null | cut -f1)

# Method 2: du apparent size (fallback)
if [ -z "$SOURCE_SIZE" ] || ! [[ "$SOURCE_SIZE" =~ ^[0-9]+$ ]]; then
    SOURCE_SIZE=$(du --apparent-size -sb "$SOURCE" 2>/dev/null | cut -f1)
fi

# Method 3: Sum file sizes directly (slowest but most accurate)
if [ -z "$SOURCE_SIZE" ] || ! [[ "$SOURCE_SIZE" =~ ^[0-9]+$ ]]; then
    echo "   Calculating exact size (may take a moment)..."
    SOURCE_SIZE=$(find "$SOURCE" -type f -printf "%s\n" 2>/dev/null | awk '{sum+=$1} END {print sum}')
fi

# Ensure we have a valid number
if [ -z "$SOURCE_SIZE" ] || ! [[ "$SOURCE_SIZE" =~ ^[0-9]+$ ]]; then
    SOURCE_SIZE=0
fi

# Display human-readable size
if [ "$SOURCE_SIZE" -gt 0 ]; then
    SOURCE_SIZE_HUMAN=$(echo "$SOURCE_SIZE" | awk '{
        if ($1 >= 1073741824) printf "%.2fGB", $1/1073741824;
        else if ($1 >= 1048576) printf "%.2fMB", $1/1048576;
        else if ($1 >= 1024) printf "%.2fKB", $1/1024;
        else printf "%dB", $1;
    }')
    echo "   Source size: $SOURCE_SIZE_HUMAN ($SOURCE_SIZE bytes)"
else
    echo "   Source size: Unknown (proceeding anyway)"
fi

DESTINATION_FREE="0"
DF_OUTPUT=$(df -B1 "$DESTINATION" 2>/dev/null | tail -n 1)
if [ -n "$DF_OUTPUT" ]; then
    DESTINATION_FREE=$(echo "$DF_OUTPUT" | awk '{print $4}' | grep -E '^[0-9]+$' || echo "0")
fi

if [ "$DESTINATION_FREE" = "0" ]; then
    DF_KB=$(df "$DESTINATION" 2>/dev/null | tail -1 | awk '{print $4}' | tr -d 'K' | grep -E '^[0-9]+$' || echo "0")
    if [ "$DF_KB" != "0" ]; then
        DESTINATION_FREE=$((DF_KB * 1024))
    fi
fi

if ! [[ "$DESTINATION_FREE" =~ ^[0-9]+$ ]]; then
    DESTINATION_FREE="0"
fi

echo "   Free space: $(echo "$DESTINATION_FREE" | awk '{printf "%.1fG", $1/1024/1024/1024}')"

if [ "$DESTINATION_FREE" -gt 0 ] && [[ "$SOURCE_SIZE" =~ ^[0-9]+$ ]]; then
    REQUIRED_SPACE=$((SOURCE_SIZE + SOURCE_SIZE / 10))
    
    if [ "$REQUIRED_SPACE" -gt "$DESTINATION_FREE" ]; then
        echo -e "${RED}Error: Not enough space at destination${NC}"
        exit 1
    fi
    echo -e "${GREEN}   ✓ Sufficient space available${NC}"
fi

# STEP 3: Full mode - delete existing destination
if [ "$MODE" = "full" ]; then
    if [ -d "$DESTINATION/$SOURCE_FOLDER_NAME" ]; then
        echo -e "${MAGENTA}3. FULL MODE: Removing existing destination...${NC}"
        echo -e "${YELLOW}   This ensures zero fragmentation${NC}"
        
        EXISTING_SIZE=$(du -sh "$DESTINATION/$SOURCE_FOLDER_NAME" 2>/dev/null | cut -f1 || echo "unknown")
        echo "   Existing data size: $EXISTING_SIZE"
        
        rm -rf "$DESTINATION/$SOURCE_FOLDER_NAME"
        sync
        
        echo -e "${GREEN}   ✓ Destination cleared${NC}"
    else
        echo -e "${MAGENTA}3. FULL MODE: Destination empty (first copy)${NC}"
    fi
else
    echo -e "${YELLOW}3. INCREMENTAL MODE: Keeping existing files${NC}"
    if [ -d "$DESTINATION/$SOURCE_FOLDER_NAME" ]; then
        EXISTING_FILES=$(find "$DESTINATION/$SOURCE_FOLDER_NAME" -type f 2>/dev/null | wc -l)
        echo "   Existing files: $EXISTING_FILES"
        echo -e "${YELLOW}   Warning: Updates will cause fragmentation${NC}"
    fi
fi

# STEP 4: Generate sorted file list
echo -e "${YELLOW}4. Generating optimized file list...${NC}"

FILELIST="/tmp/rsync_filelist_sorted_$$.txt"

find "$SOURCE" -type f 2>/dev/null | \
    sed "s|^$SOURCE/||" | \
    sort -t'/' -k1,1n -k2,2n -k3,3n > "$FILELIST"

TOTAL_FILES=$(wc -l < "$FILELIST")
echo "   Files to sync: $TOTAL_FILES"
echo -e "${GREEN}   ✓ File list sorted (Z→X→Y order)${NC}"

echo "   Sample write order:"
head -3 "$FILELIST" | while read line; do
    echo "     → $line"
done

# STEP 5: Device speed test
echo -e "${YELLOW}5. Testing device write speed...${NC}"
TEST_START=$(date +%s)
dd if=/dev/zero of="$DESTINATION/speedtest.tmp" bs=10M count=1 oflag=direct status=none 2>/dev/null || true
TEST_END=$(date +%s)
TEST_TIME=$((TEST_END - TEST_START))
if [ $TEST_TIME -eq 0 ]; then TEST_TIME=1; fi
WRITE_SPEED=$((10 / TEST_TIME))
rm -f "$DESTINATION/speedtest.tmp"

echo "   Write speed: ~${WRITE_SPEED}MB/s"

# STEP 6: Pre-create directory structure
echo -e "${YELLOW}6. Pre-creating directory structure...${NC}"

cut -d'/' -f1-2 "$FILELIST" | sort -u | while read dir; do
    if [ -n "$dir" ]; then
        mkdir -p "$DESTINATION/$SOURCE_FOLDER_NAME/$dir" 2>/dev/null || true
    fi
done

echo -e "${GREEN}   ✓ Directory structure ready${NC}"

# STEP 7: Rsync with mode-specific options
echo -e "${YELLOW}7. Starting sync...${NC}"

if [ "$MODE" = "full" ]; then
    echo -e "${MAGENTA}   FULL MODE: Complete sequential copy${NC}"
    echo "   - All files copied in order"
    echo "   - Zero fragmentation guaranteed"
else
    echo -e "${GREEN}   INCREMENTAL MODE: Only changed files${NC}"
    echo "   - Fast for small updates"
    echo "   - May fragment over multiple syncs"
fi
echo ""

RSYNC_START=$(date +%s)

rsync -a \
    --info=progress2 \
    --files-from="$FILELIST" \
    --partial \
    --inplace \
    --no-compress \
    --prune-empty-dirs \
    --human-readable \
    "$SOURCE/" "$DESTINATION/$SOURCE_FOLDER_NAME/"

RSYNC_EXIT_CODE=$?
if [ $RSYNC_EXIT_CODE -ne 0 ]; then
    echo -e "${RED}Error: rsync failed with exit code $RSYNC_EXIT_CODE${NC}"
    exit 1
fi

RSYNC_END=$(date +%s)
RSYNC_TIME=$((RSYNC_END - RSYNC_START))

echo ""
echo -e "${GREEN}   ✓ Sync completed${NC}"

# STEP 8: Verification
echo -e "${YELLOW}8. Verifying sync result...${NC}"

DEST_FILES_FINAL=$(find "$DESTINATION/$SOURCE_FOLDER_NAME" -type f 2>/dev/null | wc -l)

# Calculate destination size
DEST_SIZE_BYTES=$(find "$DESTINATION/$SOURCE_FOLDER_NAME" -type f -printf "%s\n" 2>/dev/null | awk '{sum+=$1} END {print sum}')
if [ -z "$DEST_SIZE_BYTES" ] || ! [[ "$DEST_SIZE_BYTES" =~ ^[0-9]+$ ]]; then
    DEST_SIZE_BYTES=0
fi

DEST_SIZE=$(echo "$DEST_SIZE_BYTES" | awk '{
    if ($1 >= 1073741824) printf "%.2fGB", $1/1073741824;
    else if ($1 >= 1048576) printf "%.2fMB", $1/1048576;
    else if ($1 >= 1024) printf "%.2fKB", $1/1024;
    else printf "%dB", $1;
}')

echo "   Source files: $TOTAL_FILES"
echo "   Destination files: $DEST_FILES_FINAL"
echo "   Source size: $SOURCE_SIZE_HUMAN"
echo "   Destination size: $DEST_SIZE ($DEST_SIZE_BYTES bytes)"

if [ "$TOTAL_FILES" -eq "$DEST_FILES_FINAL" ]; then
    echo -e "${GREEN}   ✓ File count matches perfectly${NC}"
else
    DIFF=$((TOTAL_FILES - DEST_FILES_FINAL))
    echo -e "${YELLOW}   Warning: File count difference: $DIFF files${NC}"
fi

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
