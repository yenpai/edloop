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
} edev_hotplug_action_e;

typedef struct {
	uint8_t      action;
	const char * subsystem;
	const char * devpath;
	uint8_t      busnum;
	uint8_t      devnum;
	uint16_t     idVendor;
	uint16_t     idProduct;
} edev_hotplug_info;

typedef void (* edev_hotplug_cb) (edev_hotplug *, edev_hotplug_info *);

_EDEV_SOURCE_EXTEND_METHOD_MACRO_(edev_hotplug, edev_hotplug);
int            edev_hotplug_attach(edev_hotplug *);
void           edev_hotplug_detach(edev_hotplug *);
edev_hotplug * edev_hotplug_new(edloop *, edev_hotplug_cb);

#endif
