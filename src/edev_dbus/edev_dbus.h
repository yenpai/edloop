#ifndef _EDEV_DBUS_H_
#define _EDEV_DBUS_H_

#include "dbus/dbus.h"
#include "edloop.h"

typedef struct edev_dbus edev_dbus;
typedef DBusHandlerResult (* edev_dbus_cb) (edev_dbus *, DBusMessage *);

typedef enum {
	EDEV_DBUS_SIGNAL,
	EDEV_DBUS_METHOD,
} edev_dbus_type;

typedef struct {
	edev_dbus_type type;
	const char * ifname;
	const char * mtname;
	edev_dbus_cb handle;
} edev_dbus_event;

_EDOBJECT_EXTEND_METHOD_MACRO_(edev_dbus, edev_dbus);
int         edev_dbus_subscribe(edev_dbus *, edev_dbus_event *);
int         edev_dbus_attach(edev_dbus *);
void        edev_dbus_detach(edev_dbus *);
edev_dbus * edev_dbus_new(edloop *, DBusConnection *, edev_dbus_cb);

#endif
