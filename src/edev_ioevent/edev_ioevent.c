#include "edloop.h"

static void edev_ioevent_finalize(edobject * obj)
{
	edev_ioevent * io = container_of(obj, edev_ioevent, source.object);

	if ((io->flags & EDIO_CLOAUTO) && io->fd >= 0)
	{
		close(io->fd);
		io->fd = -1;
	}
}

int edev_ioevent_get_unix_fd(edev_ioevent * io)
{
	return io->fd;
}

int edev_ioevent_attach(edev_ioevent * io, int fd, unsigned int flags)
{
	edev_source * source = edev_ioevent_to_source(io);
	edloop      * loop   = edev_ioevent_to_loop(io);
	int          old_fd    = io->fd;
	unsigned int old_flags = io->flags;

	if (loop == NULL)
		return -1;
	if (source->attach)
		return -2;
	if (fd < 0)
		return -3;
	if (!(flags & (EDIO_READ | EDIO_WRITE)))
		return -4;

	io->fd    = fd;
	io->flags = flags;

	if (edloop_attach(loop, source) < 0)
	{
		io->fd    = old_fd;
		io->flags = old_flags;
		return -5;
	}

	return 0;
}

void edev_ioevent_detach(edev_ioevent * io)
{
	edev_source * source = edev_ioevent_to_source(io);
	edloop_detach(source->loop, source);
}

void edev_ioevent_init(edev_ioevent * io, edloop * loop, edev_ioevent_cb handle)
{
	edev_source_init(&io->source, loop, EDEV_IOEVENT_TYPE, edev_ioevent_finalize);

	io->fd       = -1;
	io->flags    = 0;
	io->revents  = 0;
	io->err      = false;
	io->eof      = false;
	io->handle   = handle;
}

edev_ioevent * edev_ioevent_new(edloop * loop, edev_ioevent_cb handle)
{
	edev_ioevent * io;

	if (loop == NULL)
		return NULL;

	if ((io = malloc(sizeof(*io))) == NULL)
		return NULL;

	memset(io, 0, sizeof(*io));
	edev_ioevent_init(io, loop, handle);

	return io;
} 
