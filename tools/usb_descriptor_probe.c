#include <errno.h>
#include <fcntl.h>
#include <linux/usb/ch9.h>
#include <linux/usbdevice_fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int control(int fd, unsigned char req, unsigned char type, unsigned short value,
                   unsigned short index, void *data, unsigned short length) {
    struct usbdevfs_ctrltransfer x = {
        .bRequestType = type, .bRequest = req, .wValue = value, .wIndex = index,
        .wLength = length, .timeout = 2000, .data = data
    };
    return ioctl(fd, USBDEVFS_CONTROL, &x);
}

int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "usage: %s USB_FD\n", argv[0]); return 2; }
    int fd = atoi(argv[1]); unsigned char d[256];
    int n = control(fd, USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
                    USB_DT_DEVICE << 8, 0, d, sizeof(struct usb_device_descriptor));
    if (n < (int)sizeof(struct usb_device_descriptor)) { fprintf(stderr, "device descriptor failed: %s\n", strerror(errno)); return 1; }
    struct usb_device_descriptor *h = (struct usb_device_descriptor *)d;
    printf("VID=0x%04x PID=0x%04x USB=%u.%02u class=0x%02x subclass=0x%02x protocol=0x%02x configs=%u\n",
           h->idVendor, h->idProduct, h->bcdUSB >> 8, h->bcdUSB & 255, h->bDeviceClass,
           h->bDeviceSubClass, h->bDeviceProtocol, h->bNumConfigurations);
    n = control(fd, USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
                USB_DT_CONFIG << 8, 0, d, sizeof(d));
    if (n < 0) { fprintf(stderr, "config descriptor failed: %s\n", strerror(errno)); return 1; }
    int off = 0;
    while (off + 2 <= n) {
        unsigned char len = d[off], type = d[off + 1];
        if (len < 2 || off + len > n) break;
        if (type == USB_DT_INTERFACE && len >= 9)
            printf("INTERFACE number=%u alt=%u class=0x%02x subclass=0x%02x protocol=0x%02x endpoints=%u\n",
                   d[off+2], d[off+3], d[off+5], d[off+6], d[off+7], d[off+4]);
        else if (type == USB_DT_ENDPOINT && len >= 7)
            printf("ENDPOINT address=0x%02x direction=%s type=%u max_packet=%u interval=%u\n",
                   d[off+2], (d[off+2] & USB_DIR_IN) ? "IN" : "OUT", d[off+3] & 3,
                   d[off+4] | (d[off+5] << 8), d[off+6]);
        off += len;
    }
    return 0;
}
