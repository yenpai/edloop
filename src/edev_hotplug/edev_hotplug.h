#ifndef _EDEV_HOTPLUG_H_
#define _EDEV_HOTPLUG_H_

#include "edloop.h"

typedef struct edev_hotplug edev_hotplug;

typedef enum {
	/* Reference kernel source in include/linux/kobject.h */
	EDEV_HOTPLUG_ADD,
	EDEV_HOTPLUG_REMOVE,
	EDEV_HOTPLUG_CHANGE,
	EDEV_HOTPLUG_MOVE,
	EDEV_HOTPLUG_ONLINE,
	EDEV_HOTPLUG_OFFLINE,
	EDEV_HOTPLUG_ACTION_MAX,
} edev_hotplug_actions_e;

typedef enum {
	EDEV_HOTPLUG_UEKEY_ACTION,
	EDEV_HOTPLUG_UEKEY_DEVPATH,
	EDEV_HOTPLUG_UEKEY_SUBSYSTEM,

	EDEV_HOTPLUG_UEKEY_MAJOR,
	EDEV_HOTPLUG_UEKEY_MINOR,
	EDEV_HOTPLUG_UEKEY_DEVNAME,
	EDEV_HOTPLUG_UEKEY_DEVTYPE,
	
	EDEV_HOTPLUG_UEKEY_DRIVER,
	EDEV_HOTPLUG_UEKEY_DEVICE,
	
	EDEV_HOTPLUG_UEKEY_PRODUCT,
	EDEV_HOTPLUG_UEKEY_BUSNUM,
	EDEV_HOTPLUG_UEKEY_DEVNUM,

	EDEV_HOTPLUG_UEKEY_MAX,
} edev_hotplug_uevent_key_e;

typedef struct {
	uint8_t      busnum;
	uint8_t      devnum;
	uint16_t     idVendor;
	uint16_t     idProduct;
} edev_hotplug_usb_info;

typedef struct {
	uint8_t      action;
	const char * uevents[EDEV_HOTPLUG_UEKEY_MAX];
} edev_hotplug_info;

typedef void (* edev_hotplug_cb) (edev_hotplug *, edev_hotplug_info *);

_EDEV_SOURCE_EXTEND_METHOD_MACRO_(edev_hotplug, edev_hotplug);
int            edev_hotplug_filter_uevent_set(edev_hotplug *, bool enable, uint8_t key, char * value);
int            edev_hotplug_filter_action_set(edev_hotplug *, bool enable, uint8_t action);
int            edev_hotplug_attach(edev_hotplug *);
void           edev_hotplug_detach(edev_hotplug *);
edev_hotplug * edev_hotplug_new(edloop *, edev_hotplug_cb);

#endif
