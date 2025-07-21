#!/bin/bash
set -e

# Fast mass copy using rsync with intelligent sync
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
    echo "  - Intelligent sync (only copies changed files)"
    echo "  - Built-in progress monitoring"
    echo "  - Resumable transfers"
    echo "  - Optimized for many small files"
    echo "  - Real-time speed display"
    echo "  - Preserves source folder name"
    echo "  - Automatic cleanup of obsolete files"
    echo ""
    echo "Examples:"
    echo "  $0 /home/user/files /mnt/sd /dev/sdc1"
    echo "  $0 /var/data /media/usb /dev/sdb1"
    echo ""
    exit 0
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

# Extract source folder name
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
echo "  Source folder name: $SOURCE_FOLDER_NAME"
echo "  Destination: $DESTINATION"
echo "  Device: $DEVICE"
echo "  Mode: Intelligent sync (only changed files)"
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

# Get source size in bytes
SOURCE_SIZE=$(du -sb "$SOURCE" | cut -f1)
echo "   Source size: $(du -sh "$SOURCE" | cut -f1)"

# Initialize with default
DESTINATION_FREE="0"

# Method 1: Try df -B1 (most compatible)
DF_OUTPUT=$(df -B1 "$DESTINATION" 2>/dev/null | tail -n 1)
if [ -n "$DF_OUTPUT" ]; then
    # Extract available space (4th column)
    DESTINATION_FREE=$(echo "$DF_OUTPUT" | awk '{print $4}' | grep -E '^[0-9]+$' || echo "0")
fi

# Method 2: Fallback to df with KB
if [ "$DESTINATION_FREE" = "0" ]; then
    DF_KB=$(df "$DESTINATION" 2>/dev/null | tail -1 | awk '{print $4}' | tr -d 'K' | grep -E '^[0-9]+$' || echo "0")
    if [ "$DF_KB" != "0" ]; then
        DESTINATION_FREE=$((DF_KB * 1024))
    fi
fi

# Ensure DESTINATION_FREE is numeric
if ! [[ "$DESTINATION_FREE" =~ ^[0-9]+$ ]]; then
    DESTINATION_FREE="0"
fi

echo "   Free space: $(echo "$DESTINATION_FREE" | awk '{printf "%.1fG", $1/1024/1024/1024}')"

# Check space only if we have valid numbers
if [ "$DESTINATION_FREE" -gt 0 ] && [[ "$SOURCE_SIZE" =~ ^[0-9]+$ ]]; then
    REQUIRED_SPACE=$((SOURCE_SIZE + SOURCE_SIZE / 10))
    echo "   Required space (with 10% margin): $(echo "$REQUIRED_SPACE" | awk '{printf "%.1fG", $1/1024/1024/1024}')"
    
    if [ "$REQUIRED_SPACE" -gt "$DESTINATION_FREE" ]; then
        echo -e "${RED}Error: Not enough space at destination${NC}"
        echo "   Required: $(echo "$REQUIRED_SPACE" | awk '{printf "%.1fG", $1/1024/1024/1024}')"
        echo "   Available: $(echo "$DESTINATION_FREE" | awk '{printf "%.1fG", $1/1024/1024/1024}')"
        exit 1
    fi
    echo -e "${GREEN}   ✓ Sufficient space available${NC}"
else
    echo -e "${YELLOW}   Warning: Could not verify space accurately, proceeding...${NC}"
    echo "   Debug: DESTINATION_FREE='$DESTINATION_FREE', SOURCE_SIZE='$SOURCE_SIZE'"
fi

# STEP 3: Analyze sync requirements
echo -e "${YELLOW}3. Analyzing sync requirements...${NC}"

TOTAL_FILES=$(find "$SOURCE" -type f 2>/dev/null | wc -l)
echo "   Source files: $TOTAL_FILES"

# Check if destination exists to determine sync type
if [ -d "$DESTINATION/$SOURCE_FOLDER_NAME" ]; then
    DEST_FILES=$(find "$DESTINATION/$SOURCE_FOLDER_NAME" -type f 2>/dev/null | wc -l)
    echo "   Existing destination files: $DEST_FILES"
    echo "   Sync mode: Update existing destination"
else
    echo "   Sync mode: Initial copy (destination doesn't exist)"
fi

# STEP 4: Device speed test
echo -e "${YELLOW}4. Testing device write speed...${NC}"
TEST_START=$(date +%s)
dd if=/dev/zero of="$DESTINATION/speedtest.tmp" bs=10M count=1 oflag=direct status=none 2>/dev/null || true
TEST_END=$(date +%s)
TEST_TIME=$((TEST_END - TEST_START))
if [ $TEST_TIME -eq 0 ]; then TEST_TIME=1; fi
WRITE_SPEED=$((10 / TEST_TIME))
rm -f "$DESTINATION/speedtest.tmp"

echo "   Write speed: ~${WRITE_SPEED}MB/s"

# STEP 5: Rsync sync with intelligent behavior
echo -e "${YELLOW}5. Starting intelligent sync...${NC}"
echo "   Destination folder: $DESTINATION/$SOURCE_FOLDER_NAME/"
echo "   Rsync will:"
echo "   - Only copy new/modified files"
echo "   - Remove obsolete files from destination"
echo "   - Resume interrupted transfers"
echo ""

RSYNC_START=$(date +%s)

# Rsync with intelligent sync - only copies what's needed
rsync -a \
    --info=progress2 \
    --partial \
    --inplace \
    --no-compress \
    --delete-during \
    --prune-empty-dirs \
    --human-readable \
    "$SOURCE/" "$DESTINATION/$SOURCE_FOLDER_NAME/"

# Check rsync exit status
RSYNC_EXIT_CODE=$?
if [ $RSYNC_EXIT_CODE -ne 0 ]; then
    echo -e "${RED}Error: rsync failed with exit code $RSYNC_EXIT_CODE${NC}"
    exit 1
fi

RSYNC_END=$(date +%s)
RSYNC_TIME=$((RSYNC_END - RSYNC_START))

echo ""
echo -e "${GREEN}   ✓ Sync completed successfully${NC}"

# STEP 6: Verification
echo -e "${YELLOW}6. Verifying sync result...${NC}"

DEST_FILES_FINAL=$(find "$DESTINATION/$SOURCE_FOLDER_NAME" -type f 2>/dev/null | wc -l)
DEST_SIZE=$(du -sh "$DESTINATION/$SOURCE_FOLDER_NAME" 2>/dev/null | cut -f1)

echo "   Source files: $TOTAL_FILES"
echo "   Destination files: $DEST_FILES_FINAL"
echo "   Source size: $(du -sh "$SOURCE" | cut -f1)"
echo "   Destination size: $DEST_SIZE"

if [ "$TOTAL_FILES" -eq "$DEST_FILES_FINAL" ]; then
    echo -e "${GREEN}   ✓ File count matches perfectly${NC}"
else
    DIFF=$((TOTAL_FILES - DEST_FILES_FINAL))
    echo -e "${YELLOW}   Warning: File count difference: $DIFF files${NC}"
fi

# STEP 7: Sample integrity check
echo -e "${YELLOW}7. Sample integrity check...${NC}"
echo "   Checking 10 random files..."

SAMPLE_COUNT=0
SAMPLE_ERRORS=0

for file in $(find "$SOURCE" -type f 2>/dev/null | shuf | head -10); do
    SAMPLE_COUNT=$((SAMPLE_COUNT + 1))
    rel_path=${file#$SOURCE/}
    dest_file="$DESTINATION/$SOURCE_FOLDER_NAME/$rel_path"
    
    if [ -f "$dest_file" ]; then
        if cmp -s "$file" "$dest_file" 2>/dev/null; then
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
    echo -e "${GREEN}   ✓ Sample verification passed (${SAMPLE_COUNT}/${SAMPLE_COUNT} files OK)${NC}"
else
    echo -e "${YELLOW}   Warning: $SAMPLE_ERRORS/$SAMPLE_COUNT sample files had issues${NC}"
fi

# STEP 8: Final sync and unmount
echo -e "${YELLOW}8. Finalizing...${NC}"
echo "   Syncing filesystem..."
sync
sleep 2

echo "   Unmounting device..."
sudo umount "$DESTINATION"
sleep 1

echo -e "${GREEN}   ✓ Device unmounted safely${NC}"

# Calculate performance stats
TOTAL_TIME=$RSYNC_TIME
if [ $TOTAL_TIME -gt 0 ]; then
    AVG_SPEED=$((SOURCE_SIZE / 1024 / 1024 / TOTAL_TIME))
    FILES_PER_SEC=$((TOTAL_FILES / TOTAL_TIME))
else
    AVG_SPEED="N/A"
    FILES_PER_SEC="N/A"
fi

echo ""
echo -e "${GREEN}=== SYNC COMPLETED SUCCESSFULLY ===${NC}"
echo -e "${GREEN}End: $(date)${NC}"
echo ""
echo "Performance Summary:"
echo "  Files processed: $TOTAL_FILES"
echo "  Data size: $(du -sh "$SOURCE" | cut -f1)"
echo "  Sync time: ${TOTAL_TIME}s ($((TOTAL_TIME / 60))m $((TOTAL_TIME % 60))s)"
echo "  Average speed: ${AVG_SPEED}MB/s"
echo "  Files/second: $FILES_PER_SEC"
echo "  Device write speed: ~${WRITE_SPEED}MB/s"
echo "  Sample verification: $((SAMPLE_COUNT - SAMPLE_ERRORS))/$SAMPLE_COUNT files OK"
echo ""
echo "Files synced to: $DESTINATION/$SOURCE_FOLDER_NAME/"
echo ""
echo -e "${BLUE}Rsync intelligently synced only new/modified files${NC}"
echo -e "${BLUE}Next sync will be faster as only changes will be copied${NC}"

# Final status
if [ $SAMPLE_ERRORS -eq 0 ]; then
    echo -e "${GREEN}✓ SYNC SUCCESSFUL - All verifications passed${NC}"
    exit 0
else
    echo -e "${YELLOW}⚠ SYNC COMPLETED with verification warnings${NC}"
    exit 1
fi