#include <errno.h>
#include "edev_dbus.h"

struct edev_dbus {
	edobject           object;
	edloop *           loop;
	DBusConnection *   conn;
	edev_dbus_cb       handle;
	struct list_head   list; /* watchIo List */
	bool dbus_filter_binded;
	bool dbus_watch_binded;
};

typedef struct {
	edev_ioevent ioevent;
	edev_dbus *  owner;
	DBusWatch *  watch;
	struct list_head entry;
} watchIo;

static void watch_ioevent_handle(edev_ioevent * io, int UNUSED(fd), unsigned int revents)
{
	watchIo * wio = container_of(io, watchIo, ioevent);
	uint32_t  flags = 0;

	if (revents & EDIO_ERR)
		flags |= DBUS_WATCH_ERROR;
	if (revents & EDIO_READ)
		flags |= DBUS_WATCH_READABLE;
	if (revents & EDIO_WRITE)
		flags |= DBUS_WATCH_WRITABLE;
	
	dbus_watch_handle(wio->watch, flags);

	do {
		dbus_connection_read_write_dispatch(wio->owner->conn, 0);
	} while (DBUS_DISPATCH_DATA_REMAINS == dbus_connection_get_dispatch_status(wio->owner->conn));
}

static dbus_bool_t dbus_watch_add(DBusWatch * watch, void * ptr)
{
	edev_dbus * dbus = ptr;
	watchIo   * wio;

	if ((wio = malloc(sizeof(watchIo))) == NULL)
		return FALSE;

	memset(wio, 0, sizeof(watchIo));
	edev_ioevent_init(&wio->ioevent, dbus->loop, watch_ioevent_handle);

	INIT_LIST_HEAD(&wio->entry);
	wio->owner = dbus;
	wio->watch = watch;

	list_add_tail(&wio->entry, &dbus->list);
	return TRUE;
}

static void dbus_watch_remove(DBusWatch * watch, void * ptr)
{
	edev_dbus * dbus = ptr;
	watchIo * wio, * tmp;

	list_for_each_entry_safe(wio, tmp, &dbus->list, entry)
	{
		if (wio->watch == watch)
		{
			list_del(&wio->entry);
			edev_ioevent_detach(&wio->ioevent);
			edev_ioevent_unref(&wio->ioevent);
			break;
		}
	}
}

static void dbus_watch_toggle(DBusWatch * watch, void * ptr)
{
	(dbus_watch_get_enabled(watch)) ? 
		dbus_watch_add(watch, ptr) : 
		dbus_watch_remove(watch, ptr) ;
}

static DBusHandlerResult dbus_filter_handle(DBusConnection * UNUSED(conn), DBusMessage * message, void * ptr)
{
	edev_dbus * dbus = ptr;

	if (dbus->handle)
		return dbus->handle(dbus, message);

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void edev_dbus_finalize(edobject * obj)
{
	edev_dbus * dbus = container_of(obj, edev_dbus, object);
	watchIo * wio;
	
	if (dbus->dbus_filter_binded)
		dbus_connection_remove_filter(dbus->conn, dbus_filter_handle, dbus);

	while (!list_empty(&dbus->list))
	{
		wio = list_first_entry(&dbus->list, watchIo, entry);
		list_del(&wio->entry);
		edev_ioevent_detach(&wio->ioevent);
		edev_ioevent_unref(&wio->ioevent);
	}
}

int edev_dbus_attach(edev_dbus * dbus)
{
	int fd, flags;
	unsigned int events;
	watchIo * wio;

	if (dbus->dbus_watch_binded == false)
	{
		if (!dbus_connection_set_watch_functions(
					dbus->conn, 
					dbus_watch_add,
					dbus_watch_remove,
					dbus_watch_toggle,
					dbus, NULL))
		{
			return -1;
		}

		dbus->dbus_watch_binded = true;
	}

	if (dbus->dbus_filter_binded == false)
	{
		if (dbus_connection_add_filter(
					dbus->conn,
					dbus_filter_handle,
					dbus, NULL) == FALSE)
		{
			return -2;
		}

		dbus->dbus_filter_binded = true;
	}

	list_for_each_entry(wio, &dbus->list, entry)
	{
		fd     = dbus_watch_get_unix_fd(wio->watch);
		flags  = dbus_watch_get_flags(wio->watch);
		events = 0;
		if (flags & DBUS_WATCH_READABLE)
			events |= EDIO_READ;
		if (flags & DBUS_WATCH_WRITABLE)
			events |= EDIO_WRITE;
		if (fd >= 0 && events)
			edev_ioevent_attach(&wio->ioevent, fd, events);
	}

	return 0;
}

void edev_dbus_detach(edev_dbus * dbus)
{
	watchIo * wio;

	list_for_each_entry(wio, &dbus->list, entry)
		edev_ioevent_detach(&wio->ioevent);
}

void edev_dbus_init(edev_dbus * dbus, edloop * loop, DBusConnection * conn, edev_dbus_cb handle)
{
	edobject_init(&dbus->object);
	edobject_register_finalize(&dbus->object, edev_dbus_finalize);

	INIT_LIST_HEAD(&dbus->list);
	dbus->handle = handle;
	dbus->loop = loop;
	dbus->conn = conn;
	dbus->dbus_watch_binded  = false;
	dbus->dbus_filter_binded = false;
}

edev_dbus * edev_dbus_new(edloop * loop, DBusConnection * conn, edev_dbus_cb handle)
{
	edev_dbus * dbus;

	if (loop == NULL || conn == NULL)
		return NULL;

	if ((dbus = malloc(sizeof(edev_dbus))) == NULL)
		return NULL;

	memset(dbus, 0, sizeof(edev_dbus));
	edev_dbus_init(dbus, loop, conn, handle);

	return dbus;
}
