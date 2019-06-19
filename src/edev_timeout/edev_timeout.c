#include "edloop.h"

int edev_timeout_get_remain(edev_timeout * timeout)
{
	struct timeval tv;
	int msec;

	if (!timeout->source.attach)
		return -1;

	edutil_time_curr(&tv);
	msec = edutil_time_diff(&timeout->time, &tv);
	return (msec < 0) ? 0 : msec ;
}

int edev_timeout_start(edev_timeout * timeout, unsigned int msec)
{
	edev_source * source = edev_timeout_to_source(timeout);
	edloop * loop = edev_timeout_to_loop(timeout);
	struct timeval * time = &timeout->time;

	if (loop == NULL)
		return -1;

	edutil_time_next(time, msec);

	if (edloop_attach(loop, source) < 0)
		return -2;

	return 0;
}

void edev_timeout_stop(edev_timeout * timeout)
{
	edev_source * source = edev_timeout_to_source(timeout);
	edloop_detach(source->loop, source);
}

void edev_timeout_init(edev_timeout * timeout, edloop * loop, edev_timeout_cb done)
{
	edev_source_init(&timeout->source, loop, EDEV_TIMEOUT_TYPE);
	timeout->time.tv_sec  = 0;
	timeout->time.tv_usec = 0;
	timeout->done = done;
}

edev_timeout * edev_timeout_new(edloop * loop, edev_timeout_cb done)
{
	edev_timeout * timeout;

	if (loop == NULL)
		return NULL;

	if ((timeout = malloc(sizeof(*timeout))) == NULL)
		return NULL;

	memset(timeout, 0, sizeof(*timeout));
	edev_timeout_init(timeout, loop, done);

	return timeout;
}
