#ifndef _EDEV_USBDEV_H_
#define _EDEV_USBDEV_H_

#include "edloop.h"

typedef struct edev_usbdev edev_usbdev;
typedef struct {
	const char * sysname;
	uint8_t busnum;
	uint8_t devaddr;
	uint8_t detached;
} edev_usbdev_info;

typedef void (* edev_usbdev_cb) (edev_usbdev *, edev_usbdev_info *);

_EDEV_SOURCE_EXTEND_METHOD_MACRO_(edev_usbdev, edev_usbdev);
int           edev_usbdev_attach(edev_usbdev *);
void          edev_usbdev_detach(edev_usbdev *);
edev_usbdev * edev_usbdev_new(edloop *, edev_usbdev_cb);

#endif
