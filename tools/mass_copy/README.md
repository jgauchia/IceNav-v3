# Mass File Copy Script - User Manual

A high-performance bash script optimized for copying millions of small files to external devices using rsync.

## Overview

This script solves the common problem of slow file transfers when dealing with millions of small files. Traditional copy methods can take hours, while this optimized approach reduces transfer time to minutes.

## Features

- ✅ **No compression issues** - Copies files exactly as they are
- ✅ **Real-time progress monitoring** - See exactly what's being copied
- ✅ **Resumable transfers** - Continue interrupted copies
- ✅ **Optimized for millions of small files** - Much faster than traditional copy methods
- ✅ **Built-in integrity verification** - Sample file verification
- ✅ **No temporary files** - Direct copy without intermediate TAR files
- ✅ **Performance metrics** - Speed and time statistics

## Performance Comparison

| Method | 1 Million Small Files | Notes |
|--------|----------------------|-------|
| **rsync script** | **15-30 minutes** | ✅ Recommended |
| TAR + copy | 25-45 minutes | ⚠️ Compression issues |
| cp file-by-file | 60-120 minutes | ❌ Very slow |

## System Requirements

### Operating System
- Debian/Ubuntu Linux (tested)
- Other Linux distributions (should work)
- bash shell version 4.0 or higher

### Hardware Requirements
- Minimum 1GB RAM (more recommended for large datasets)
- Sufficient storage space on destination device
- USB 2.0 or higher port (USB 3.0+ recommended)

### Permissions
- sudo privileges for device mounting/unmounting
- Read access to source directories
- User should be in `disk` group (recommended)

## Dependencies

### Required (Usually Pre-installed)
```bash
# Check if installed
which rsync
which bash
which mount
```

### Optional (Recommended)
```bash
# Install for enhanced file verification
sudo apt install coreutils
```

### Adding User to Disk Group (Recommended)
```bash
# Add current user to disk group
sudo usermod -a -G disk $USER

# Verify group membership
groups $USER

# Log out and back in for changes to take effect
```

## Installation

### Quick Installation
1. Download the script file (`rsync_copy.sh`)
2. Make it executable:
   ```bash
   chmod +x rsync_copy.sh
   ```
3. Optionally move to system PATH:
   ```bash
   sudo cp rsync_copy.sh /usr/local/bin/
   ```

### Verification
```bash
# Test script help
./rsync_copy.sh --help

# Check system dependencies
rsync --version
```

## Usage

### Basic Syntax
```bash
./rsync_copy.sh [SOURCE] [DESTINATION_MOUNT] [DEVICE]
```

### Parameter Description
- **SOURCE**: Full path to directory containing files to copy
- **DESTINATION_MOUNT**: Mount point where device will be mounted (e.g., `/mnt/backup`)
- **DEVICE**: Device path (e.g., `/dev/sdb1`, `/dev/sdc1`)

### Identifying Your Device
```bash
# List all storage devices
lsblk

# Detailed device information
sudo fdisk -l

# Monitor device connections
dmesg | tail -20
```

Common device patterns:
- USB drives: `/dev/sdb1`, `/dev/sdc1`
- SD cards: `/dev/mmcblk0p1`, `/dev/mmcblk1p1`
- External drives: `/dev/sdd1`, `/dev/sde1`

## Examples

### Basic Examples
```bash
# Copy documents to USB drive
./rsync_copy.sh /home/user/documents /mnt/usb /dev/sdb1

# Copy photos to SD card
./rsync_copy.sh /home/user/photos /mnt/sd /dev/mmcblk0p1

# Copy project files to external drive
./rsync_copy.sh /var/www/html /mnt/backup /dev/sdc1
```

### Advanced Examples
```bash
# Copy with logging
./rsync_copy.sh /source /dest /dev/sdb1 2>&1 | tee transfer.log

# Copy specific subdirectory
./rsync_copy.sh /home/user/projects/important /mnt/backup /dev/sdb1

# Copy system files (as root)
sudo ./rsync_copy.sh /etc /mnt/backup /dev/sdb1
```

## Step-by-Step Process

### 1. Pre-Transfer Preparation
The script automatically performs:
- Device mounting with optimized settings
- Space verification (source vs. destination)
- File counting and time estimation
- Device write speed testing

### 2. Transfer Process
- Real-time progress display
- File-by-file transfer status
- Speed monitoring
- Error detection and reporting

### 3. Post-Transfer Verification
- File count comparison
- Sample integrity checking
- Performance summary
- Safe device unmounting

## Workflow Overview

```
Start → Mount Device → Verify Space → Count Files → 
Speed Test → Rsync Transfer → Verification → Unmount → Complete
```

## File Organization

After successful transfer, files are organized as:
```
/mount/point/backup/
├── [original directory structure preserved]
├── file1.txt
├── file2.jpg
└── subdirectory/
    ├── file3.doc
    └── file4.pdf
```

## Performance Optimization

### For Best Performance
- Use USB 3.0+ ports and devices
- Ensure source is on fast storage (SSD preferred)
- Close unnecessary applications during transfer
- Use wired connections for network storage

### File System Recommendations
- **FAT32**: Good compatibility, 4GB file limit
- **exFAT**: Better for large files, good compatibility
- **ext4**: Best performance on Linux, limited Windows compatibility
- **NTFS**: Good for Windows compatibility

### Batch Processing Large Datasets
For extremely large datasets (10M+ files):
```bash
# Split into batches
./rsync_copy.sh /source/batch1 /mnt/backup1 /dev/sdb1
./rsync_copy.sh /source/batch2 /mnt/backup2 /dev/sdc1
```

## Troubleshooting

### Common Issues and Solutions

#### Device Not Detected
```bash
# Check device connection
lsblk | grep -E "(sd|mmcblk)"

# Check USB connections
lsusb

# Reconnect device and check dmesg
dmesg | tail -10
```

#### Permission Denied
```bash
# Check current user groups
groups

# Add user to necessary groups
sudo usermod -a -G disk,plugdev $USER

# Logout and login again
```

#### Mount Failures
```bash
# Manually unmount if stuck
sudo umount /dev/sdX1

# Check filesystem
sudo fsck.fat -v /dev/sdX1  # For FAT32
sudo fsck.exfat /dev/sdX1   # For exFAT
```

#### Slow Performance
- Check for background processes: `top` or `htop`
- Verify USB port speed: `lsusb -t`
- Monitor I/O: `sudo iotop`
- Check device health: `sudo smartctl -a /dev/sdX`

#### Transfer Interruption
Simply re-run the same command. rsync will automatically resume from where it left off.

## Monitoring and Logging

### Real-time Monitoring
```bash
# In another terminal, monitor progress
watch -n 5 'df -h /mnt/backup'

# Monitor transfer speed
sudo iotop -p $(pgrep rsync)
```

### Logging Options
```bash
# Basic logging
./rsync_copy.sh /source /dest /dev/sdb1 > transfer.log 2>&1

# Timestamped logging
./rsync_copy.sh /source /dest /dev/sdb1 2>&1 | ts '[%Y-%m-%d %H:%M:%S]' | tee transfer.log
```

## Security Considerations

### Data Safety
- Script performs read-only operations on source
- No source data is modified or deleted
- Destination device is safely unmounted
- Sample verification ensures data integrity

### Permission Management
- Requires sudo only for mounting operations
- Preserves original file permissions
- Respects file ownership where possible

## Integration and Automation

### Scheduled Backups
```bash
# Edit crontab
crontab -e

# Add weekly backup (Sundays at 2 AM)
0 2 * * 0 /path/to/rsync_copy.sh /home/user/data /mnt/backup /dev/sdb1
```

### Script Integration
```bash
# Call from other scripts
if ./rsync_copy.sh "$SOURCE" "$DEST" "$DEVICE"; then
    echo "Backup successful"
    # Additional actions
else
    echo "Backup failed"
    # Error handling
fi
```

## Performance Metrics

The script provides detailed statistics:
- **Transfer Time**: Total time for complete operation
- **Average Speed**: MB/s throughout transfer
- **Files per Second**: Processing rate
- **Device Speed**: Baseline write performance
- **Verification Results**: Integrity check status

## Best Practices

### Before Transfer
1. Test with small dataset first
2. Ensure adequate free space (20% margin recommended)
3. Close unnecessary applications
4. Use high-quality cables and ports

### During Transfer
1. Avoid disconnecting devices
2. Don't run other intensive I/O operations
3. Monitor for error messages
4. Keep system powered (for laptops)

### After Transfer
1. Review verification results
2. Test sample files on destination
3. Keep transfer logs for reference
4. Safely store backup devices

## Support and Maintenance

### Regular Updates
- Check for script updates periodically
- Update system packages: `sudo apt update && sudo apt upgrade`
- Verify dependency versions

### Performance Monitoring
- Keep transfer logs for trend analysis
- Monitor device health regularly
- Replace aging storage devices proactively

## Limitations

- Requires sudo privileges for mounting
- Limited to devices with standard filesystems
- Not suitable for encrypted devices (without manual setup)
- Performance depends on source and destination device speeds

---

**Note**: Always test with non-critical data first. Ensure proper backups exist before running large-scale transfers.