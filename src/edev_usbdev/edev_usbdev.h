#ifndef _EDEV_USBDEV_H_
#define _EDEV_USBDEV_H_

#include "edloop.h"

typedef struct edev_usbdev edev_usbdev;

typedef enum {
	/* Reference kernel source in include/linux/kobject.h */
	EDEV_UEVENT_ACTION_ADD,
	EDEV_UEVENT_ACTION_REMOVE,
	EDEV_UEVENT_ACTION_CHANGE,
	EDEV_UEVENT_ACTION_MOVE,
	EDEV_UEVENT_ACTION_ONLINE,
	EDEV_UEVENT_ACTION_OFFLINE,
	EDEV_UEVENT_ACTION_MAX,
} edev_uevent_action_e;

typedef struct {
	uint8_t  action;
	const char * devpath;
	const char * subsystem;
	uint8_t  busnum;
	uint8_t  devnum;
	uint16_t idVendor;
	uint16_t idProduct;
} edev_usbdev_info;

typedef void (* edev_usbdev_cb) (edev_usbdev *, edev_usbdev_info *);

_EDEV_SOURCE_EXTEND_METHOD_MACRO_(edev_usbdev, edev_usbdev);
int           edev_usbdev_attach(edev_usbdev *);
void          edev_usbdev_detach(edev_usbdev *);
edev_usbdev * edev_usbdev_new(edloop *, edev_usbdev_cb);

#endif
