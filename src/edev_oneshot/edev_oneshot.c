#include "edloop.h"

static void edev_oneshot_finalize(edobject * UNUSED(obj))
{
}

int edev_oneshot_action(edev_oneshot * oneshot)
{
	if (oneshot->action == false)
	{
		oneshot->action = true;
		return edev_oneshot_attach(oneshot);
	}

	return 0;
}

int edev_oneshot_attach(edev_oneshot * oneshot)
{
	edev_source * source = edev_oneshot_to_source(oneshot);
	edloop * loop = edev_oneshot_to_loop(oneshot);

	if (loop == NULL)
		return -1;

	if (edloop_attach(loop, source) < 0)
		return -2;

	return 0;
}

void edev_oneshot_detach(edev_oneshot * oneshot)
{
	edev_source * source = edev_oneshot_to_source(oneshot);
	edloop_detach(source->loop, source);
}

edev_oneshot * edev_oneshot_new(edloop * loop, edev_oneshot_cb done)
{
	edev_oneshot * oneshot;

	if (loop == NULL)
		return NULL;

	if ((oneshot = malloc(sizeof(*oneshot))) == NULL)
		return NULL;

	memset(oneshot, 0, sizeof(*oneshot));
	edev_source_base_init(&oneshot->source, loop, EDEV_ONESHOT_TYPE, edev_oneshot_finalize);

	oneshot->done   = done;
	oneshot->action = false;
	return oneshot;
}
