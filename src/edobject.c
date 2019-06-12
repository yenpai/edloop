#include "edbase.h"
#include "edobject.h"
#include <limits.h>

void edobject_unref(edobject * o)
{
	if (o->refer_count > 1)
	{
		o->refer_count--;
		return;
	}

	if (o->finalize_cb)
		o->finalize_cb(o);

	free(o);
}

edobject * edobject_ref(edobject * o)
{
	if (o->refer_count < UINT_MAX)
	{
		o->refer_count++;
		return o;
	}

	return NULL;
}

void edobject_base_init(edobject * o, edobject_finalize_cb cb)
{
	o->refer_count = 1;
	o->finalize_cb = cb;
}
