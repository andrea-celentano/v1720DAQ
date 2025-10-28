#!/bin/bash
#Restart CAEN USB device (Vendor=21e1, Product=0016)

VENDOR_ID="21e1"
PRODUCT_ID="0016"

# Find the USB bus and device numbers
DEVICE_INFO=$(lsusb | grep "${VENDOR_ID}:${PRODUCT_ID}")
if [ -z "$DEVICE_INFO" ]; then
  echo "CAEN USB device not found!"
  exit 1
fi

BUS=$(echo $DEVICE_INFO | awk '{print $2}')
DEV=$(echo $DEVICE_INFO | awk '{print $4}' | sed 's/://')

echo "Found CAEN USB device on Bus $BUS Device $DEV"

# Check if usbreset is installed, otherwise compile it temporarily
if ! command -v usbreset &> /dev/null; then
  echo "usbreset not found. Creating temporary version..."
  cat << 'EOF' > /tmp/usbreset.c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
int main(int argc, char **argv) {
    const char *filename;
    int fd;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <device>\n", argv[0]);
        return 1;
    }
    filename = argv[1];
    fd = open(filename, O_WRONLY);
    if (fd < 0) {
        perror("Error opening output file");
        return 1;
    }
    printf("Resetting USB device %s\n", filename);
    ioctl(fd, USBDEVFS_RESET, 0);
    close(fd);
    return 0;
}
EOF
  gcc -o /tmp/usbreset /tmp/usbreset.c
  USBRESET="/tmp/usbreset"
else
  USBRESET=$(which usbreset)
fi

# Full device path
DEVICE_PATH="/dev/bus/usb/${BUS}/${DEV}"

# Need root privileges
if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (sudo $0)"
  exit 1
fi

# Perform the reset
echo "Restarting CAEN USB device..."
$USBRESET "$DEVICE_PATH"

echo "USB device restart complete."

