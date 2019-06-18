#include <errno.h>
#include "edev_inotify.h"

struct edev_inotify {
	edev_ioevent       ioevent;
	edev_inotify_cb    handle;
	int                fd;
};

static void inotify_ioevent_handle(edev_ioevent * io, int fd, unsigned int revents)
{
	edev_inotify * in = container_of(io, edev_inotify, ioevent);
	char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
	struct inotify_event * event;
	ssize_t len;
	char * ptr;

	if (!(revents & EDIO_READ))
		return;

	while ((len = read(fd, buf, sizeof(buf))) < 0)
	{
		if (errno != EINTR)
		{
			return;
		}
	}

	/* Loop over all events in the buffer */
	if (in->handle)
	{
		for (ptr = buf; ptr < buf + len; ptr += sizeof(struct inotify_event) + event->len)
		{
			event = (struct inotify_event *) ptr;
			in->handle(in, event);
		}
	}

}

int edev_inotify_watch_add(edev_inotify * in, const char * pathname, uint32_t mask)
{
	return (in->fd >= 0) ? inotify_add_watch(in->fd, pathname, mask) : -1 ;
}

int edev_inotify_watch_del(edev_inotify * in, int wd)
{
	return (in->fd >= 0) ? inotify_rm_watch(in->fd, wd) : -1 ;
}

int edev_inotify_attach(edev_inotify * in)
{
	int ret = 0;

	if (in->fd < 0 && (in->fd = inotify_init1(IN_NONBLOCK)) < 0)
		return -1;

	if (edev_ioevent_attach(&in->ioevent, in->fd, EDIO_READ | EDIO_NONBLOCK | EDIO_CLOEXEC | EDIO_CLOAUTO) < 0)
	{
		close(in->fd);
		in->fd = -1;
		return -2;
	}

	return ret;
}

void edev_inotify_detach(edev_inotify * in)
{
	edev_ioevent_detach(&in->ioevent);
}

void edev_inotify_init(edev_inotify * in, edloop * loop, edev_inotify_cb handle)
{
	edev_ioevent_init(&in->ioevent, loop, inotify_ioevent_handle);
	in->handle = handle;
	in->fd = -1;
}

edev_inotify * edev_inotify_new(edloop * loop, edev_inotify_cb handle)
{
	edev_inotify * in;

	if (loop == NULL)
		return NULL;

	if ((in = malloc(sizeof(edev_inotify))) == NULL)
		return NULL;

	memset(in, 0, sizeof(edev_inotify));
	edev_inotify_init(in, loop, handle);

	return in;
}
