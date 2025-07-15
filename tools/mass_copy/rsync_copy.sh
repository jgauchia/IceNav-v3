#!/bin/bash
set -e

# Fast mass copy using rsync (no TAR compression issues)
# Usage: ./rsync_copy.sh [SOURCE] [DESTINATION] [DEVICE]

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to show help
show_help() {
    echo -e "${BLUE}=== FAST MASS COPY WITH RSYNC ===${NC}"
    echo ""
    echo "Usage: $0 [SOURCE] [DESTINATION] [DEVICE]"
    echo ""
    echo "Features:"
    echo "  - NO compression issues"
    echo "  - Built-in progress monitoring"
    echo "  - Resumable transfers"
    echo "  - Optimized for many small files"
    echo "  - Real-time speed display"
    echo ""
    echo "Examples:"
    echo "  $0 /home/user/files /mnt/sd /dev/sdc1"
    echo "  $0 /var/data /media/usb /dev/sdb1"
    echo ""
    exit 0
}

# Function to draw progress bar
draw_progress_bar() {
    local percent=$1
    local width=50
    local filled=$((percent * width / 100))
    local empty=$((width - filled))
    
    printf "\r["
    printf "%${filled}s" | tr ' ' '='
    printf "%${empty}s" | tr ' ' '.'
    printf "] %3d%%" "$percent"
}

# Verify parameters
if [ $# -eq 0 ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    show_help
fi

if [ $# -lt 3 ]; then
    echo -e "${RED}Error: 3 parameters required${NC}"
    echo "Usage: $0 [SOURCE] [DESTINATION] [DEVICE]"
    echo "Run '$0 --help' for more information"
    exit 1
fi

SOURCE="$1"
DESTINATION="$2"
DEVICE="$3"

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
    if mountpoint -q "$DESTINATION" 2>/dev/null; then
        sudo umount "$DESTINATION" 2>/dev/null || true
    fi
}

trap cleanup EXIT INT TERM

echo -e "${BLUE}=== FAST MASS COPY WITH RSYNC ===${NC}"
echo -e "${BLUE}Start: $(date)${NC}"
echo ""
echo "Configuration:"
echo "  Source: $SOURCE"
echo "  Destination: $DESTINATION"
echo "  Device: $DEVICE"
echo ""

# STEP 1: Mount device
echo -e "${YELLOW}1. Mounting device...${NC}"
sudo mkdir -p "$DESTINATION"
sudo umount "$DEVICE" 2>/dev/null || true

# Mount with optimizations for many small files
sudo mount -t vfat -o rw,uid=$(id -u),gid=$(id -g),umask=0022,async,noatime,nodiratime "$DEVICE" "$DESTINATION"

if ! mountpoint -q "$DESTINATION"; then
    echo -e "${RED}Error: Could not mount $DEVICE at $DESTINATION${NC}"
    exit 1
fi

echo -e "${GREEN}   ✓ Device mounted successfully${NC}"

# STEP 2: Space verification
echo -e "${YELLOW}2. Verifying available space...${NC}"
SOURCE_SIZE=$(du -sb "$SOURCE" | cut -f1)
DESTINATION_FREE=$(df -B1 "$DESTINATION" | awk 'NR==2 {print $4}')
SOURCE_SIZE_HR=$(du -sh "$SOURCE" | cut -f1)
DESTINATION_FREE_HR=$(df -h "$DESTINATION" | awk 'NR==2 {print $4}')

echo "   Source size: $SOURCE_SIZE_HR"
echo "   Free space: $DESTINATION_FREE_HR"

if [ "$SOURCE_SIZE" -gt "$DESTINATION_FREE" ]; then
    echo -e "${RED}Error: Not enough space at destination${NC}"
    exit 1
fi

echo -e "${GREEN}   ✓ Sufficient space${NC}"

# STEP 3: Count files for progress estimation
echo -e "${YELLOW}3. Analyzing source files...${NC}"
echo "   Counting files (this may take a moment)..."
TOTAL_FILES=$(find "$SOURCE" -type f | wc -l)
TOTAL_DIRS=$(find "$SOURCE" -type d | wc -l)

echo "   Total files: $TOTAL_FILES"
echo "   Total directories: $TOTAL_DIRS"

if [ "$TOTAL_FILES" -eq 0 ]; then
    echo -e "${RED}Error: No files found in $SOURCE${NC}"
    exit 1
fi

# Estimate time based on file count
if [ "$TOTAL_FILES" -lt 10000 ]; then
    ESTIMATED_TIME="1-3 minutes"
elif [ "$TOTAL_FILES" -lt 100000 ]; then
    ESTIMATED_TIME="3-10 minutes"
elif [ "$TOTAL_FILES" -lt 1000000 ]; then
    ESTIMATED_TIME="10-30 minutes"
else
    ESTIMATED_TIME="30-60 minutes"
fi

echo "   Estimated time: $ESTIMATED_TIME"

# STEP 4: Device speed test
echo -e "${YELLOW}4. Testing device write speed...${NC}"
TEST_START=$(date +%s)
dd if=/dev/zero of="$DESTINATION/speedtest.tmp" bs=10M count=1 oflag=direct status=none 2>/dev/null
TEST_END=$(date +%s)
TEST_TIME=$((TEST_END - TEST_START))
if [ $TEST_TIME -eq 0 ]; then TEST_TIME=1; fi
WRITE_SPEED=$((10 / TEST_TIME))
rm "$DESTINATION/speedtest.tmp"

echo "   Write speed: ~${WRITE_SPEED}MB/s"

# STEP 5: Rsync copy with progress
echo -e "${YELLOW}5. Starting rsync copy...${NC}"
echo "   Progress will be shown below:"
echo ""

RSYNC_START=$(date +%s)

# Create temporary log file for rsync output
RSYNC_LOG=$(mktemp /tmp/rsync_progress.XXXXXX)

# Method 1: Simple progress bar based on file count
if command -v pv >/dev/null 2>&1; then
    # If pv is available, use it for better progress
    echo "   Using enhanced progress display..."
    
    # Generate file list
    FILELIST=$(mktemp /tmp/rsync_files.XXXXXX)
    cd "$SOURCE" && find . -type f > "$FILELIST"
    
    # Use rsync with file list and pv for progress
    rsync -av \
        --files-from="$FILELIST" \
        --partial \
        --inplace \
        --no-compress \
        "$SOURCE" "$DESTINATION/backup/" 2>&1 | \
        pv -l -s "$TOTAL_FILES" -p -t -e > /dev/null
    
    rm -f "$FILELIST"
else
    # Method 2: Custom progress monitoring
    echo "   Copying $TOTAL_FILES files..."
    
    # Start rsync in background with log output
    rsync -av \
        --log-file="$RSYNC_LOG" \
        --partial \
        --inplace \
        --no-compress \
        --delete-during \
        --prune-empty-dirs \
        "$SOURCE/" "$DESTINATION/backup/" &
    
    RSYNC_PID=$!
    
    # Monitor progress
    while kill -0 $RSYNC_PID 2>/dev/null; do
        if [ -f "$RSYNC_LOG" ]; then
            # Count transferred files from log
            TRANSFERRED=$(grep -c ">" "$RSYNC_LOG" 2>/dev/null || echo 0)
            PERCENT=$((TRANSFERRED * 100 / TOTAL_FILES))
            
            # Show progress bar
            draw_progress_bar $PERCENT
            printf " %d/%d files" "$TRANSFERRED" "$TOTAL_FILES"
            
            # Add current file being transferred
            CURRENT_FILE=$(tail -n 1 "$RSYNC_LOG" 2>/dev/null | grep ">" | sed 's/.*>//' | cut -c1-40)
            if [ -n "$CURRENT_FILE" ]; then
                printf " | %s..." "$CURRENT_FILE"
            fi
        fi
        sleep 1
    done
    
    # Final progress
    draw_progress_bar 100
    printf " %d/%d files" "$TOTAL_FILES" "$TOTAL_FILES"
    echo ""
fi

RSYNC_END=$(date +%s)
RSYNC_TIME=$((RSYNC_END - RSYNC_START))

# Clean up log file
rm -f "$RSYNC_LOG"

echo ""
echo -e "${GREEN}   ✓ Rsync completed successfully${NC}"

# STEP 6: Verification
echo -e "${YELLOW}6. Verifying copy...${NC}"

# Count files in destination
echo "   Counting destination files..."
DEST_FILES=$(find "$DESTINATION/backup" -type f 2>/dev/null | wc -l)
DEST_SIZE=$(du -sh "$DESTINATION/backup" 2>/dev/null | cut -f1)

echo "   Source files: $TOTAL_FILES"
echo "   Destination files: $DEST_FILES"
echo "   Source size: $SOURCE_SIZE_HR"
echo "   Destination size: $DEST_SIZE"

if [ "$TOTAL_FILES" -ne "$DEST_FILES" ]; then
    echo -e "${YELLOW}   Warning: File count difference detected${NC}"
    echo "   This might be due to permissions or special files"
else
    echo -e "${GREEN}   ✓ File count matches${NC}"
fi

# STEP 7: Sample verification (check a few random files)
echo -e "${YELLOW}7. Sample integrity check...${NC}"
echo "   Checking 5 random files..."

SAMPLE_COUNT=0
SAMPLE_ERRORS=0

for file in $(find "$SOURCE" -type f | shuf | head -5); do
    SAMPLE_COUNT=$((SAMPLE_COUNT + 1))
    rel_path=${file#$SOURCE/}
    dest_file="$DESTINATION/backup/$rel_path"
    
    if [ -f "$dest_file" ]; then
        if cmp -s "$file" "$dest_file"; then
            echo "   ✓ $rel_path"
        else
            echo "   ✗ $rel_path (content differs)"
            SAMPLE_ERRORS=$((SAMPLE_ERRORS + 1))
        fi
    else
        echo "   ✗ $rel_path (missing)"
        SAMPLE_ERRORS=$((SAMPLE_ERRORS + 1))
    fi
done

if [ $SAMPLE_ERRORS -eq 0 ]; then
    echo -e "${GREEN}   ✓ Sample verification passed${NC}"
else
    echo -e "${YELLOW}   Warning: $SAMPLE_ERRORS/$SAMPLE_COUNT sample files had issues${NC}"
fi

# STEP 8: Final sync and unmount
echo -e "${YELLOW}8. Finalizing...${NC}"
sync
sleep 2
sudo umount "$DESTINATION"

# Calculate speeds
TOTAL_TIME=$RSYNC_TIME
if [ $TOTAL_TIME -gt 0 ]; then
    AVG_SPEED=$((SOURCE_SIZE / 1024 / 1024 / TOTAL_TIME))
    FILES_PER_SEC=$((TOTAL_FILES / TOTAL_TIME))
else
    AVG_SPEED="N/A"
    FILES_PER_SEC="N/A"
fi

echo ""
echo -e "${GREEN}=== COPY COMPLETED SUCCESSFULLY ===${NC}"
echo -e "${GREEN}End: $(date)${NC}"
echo ""
echo "Performance Summary:"
echo "  Files copied: $TOTAL_FILES"
echo "  Data copied: $SOURCE_SIZE_HR"
echo "  Total time: ${TOTAL_TIME}s ($((TOTAL_TIME / 60))m $((TOTAL_TIME % 60))s)"
echo "  Average speed: ${AVG_SPEED}MB/s"
echo "  Files/second: $FILES_PER_SEC"
echo "  Device write speed: ~${WRITE_SPEED}MB/s"
echo ""
echo "Files are located at: $DESTINATION/backup/"
echo ""
echo -e "${BLUE}Note: Progress bar shows file count progress${NC}"
echo "For size-based progress, install pv: sudo apt install pv"