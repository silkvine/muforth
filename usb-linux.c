/*
 * This file is part of muFORTH: http://muforth.nimblemachines.com/
 *
 * Copyright (c) 2002-2012 David Frech. All rights reserved, and all wrongs
 * reversed. (See the file COPYRIGHT for details.)
 */

/*
 * Linux support for USB devices.
 */

#include "muforth.h"

#include <ctype.h>          /* isdigit */
#include <sys/types.h>
#include <dirent.h>         /* opendir, readdir */
#include <fcntl.h>          /* open */
#include <unistd.h>         /* close */
#include <sys/ioctl.h>      /* ioctl */

#include <linux/usb/ch9.h>
#include <linux/usbdevice_fs.h>

/* change to match your system! */
#define USB_ROOT1 "/proc/bus/usb"
#define USB_ROOT2 "/dev/bus/usb"

#define USB_PATH_MAX (strlen(USB_ROOT1)+16)

struct match
{
    int idVendor;
    int idProduct;
};

typedef int (*usb_match_fn)(char *, struct match *);    /* filepath, match_data */

/*
 * Returns
 *   0 if directory exhausted or error
 *  >0 if match found
 */
static int foreach_dirent(char *path, usb_match_fn fn, struct match *pmatch)
{
    char pathbuf[USB_PATH_MAX];
    DIR *pdir;
    struct dirent *pde;

    pdir = opendir(path);

    /* Return no match if directory couldn't be opened. */
    if (pdir == NULL) return 0;

    while ((pde = readdir(pdir)) != NULL)
    {
        /* bus and device entry names are 3 decimal digits */
        if (isdigit(pde->d_name[0]))
        {
            char *subpath;
            int matched;

            /* build path right-to-left */
            subpath = path_prefix(pde->d_name, pathbuf + USB_PATH_MAX, '\0', pathbuf);
            subpath = path_prefix(path, subpath, '/', pathbuf);
            matched = fn(subpath, pmatch);
            if (matched != 0) return matched;   /* error or match */
        }
    }
    return 0;   /* no match */
}

static int read_le16(__le16 *pw)
{
    __u8 *pb = (__u8 *)pw;
    return pb[0] + (pb[1] << 8);
}

static int match_device(char *dev, struct match *pmatch)
{
    int fd;
    struct usb_device_descriptor dev_desc;
    int count;

    fd = open(dev, O_RDONLY);
    if (fd == -1) return -1;

    count = read(fd, &dev_desc, USB_DT_DEVICE_SIZE);
    assert (count == USB_DT_DEVICE_SIZE, "couldn't read device descriptor");

    close(fd);

    if (read_le16(&dev_desc.idVendor) == pmatch->idVendor &&
        read_le16(&dev_desc.idProduct) == pmatch->idProduct)
        return open(dev, O_RDWR);   /* Re-open read-write */

    return 0;
}

static int enumerate_devices(char *bus, struct match *pmatch)
{
    return foreach_dirent(bus, match_device, pmatch);
}

/*
 * usb-find-device (vendor-id product-id -- handle -1 | 0)
 */
void mu_usb_find_device()
{
    struct match match;
    int matched;

    match.idVendor = ST1;
    match.idProduct = TOP;

    /* Enumerate USB device tree, looking for a match */
    matched = foreach_dirent(USB_ROOT1, enumerate_devices, &match);

    /* If nothing found, or opendir error, try the other bus */
    if (matched == 0)
        matched = foreach_dirent(USB_ROOT2, enumerate_devices, &match);

    if (matched < 0) return abort_strerror();

    if (matched == 0)
    {
        /* No match found */
        DROP(1);
        TOP = 0;
    }
    else
    {
        /* Matched; return the device's _open_ file descriptor */
        ST1 = matched;
        TOP = -1;
    }
}

/*
 * usb-close ( handle)
 */
void mu_usb_close()
{
    mu_close_file();
}

/*
 * usb-request (bmRequestType bRequest wValue wIndex wLength 'buffer device)
 */
void mu_usb_request()
{
    struct usbdevfs_ctrltransfer req;
    int fd;

    req.bRequestType = SP[6];       /* should be called bmRequestType */
    req.bRequest = SP[5];
    req.wValue = SP[4];
    req.wIndex = ST3;
    req.wLength = ST2;
    req.timeout = 4000 /* ms timeout */;
    req.data = (void *)ST1;
    fd = TOP;
    DROP(7);

    if (ioctl(fd, USBDEVFS_CONTROL, &req) == -1) return abort_strerror();
}