#include "edbase.h"
#include "edobject.h"
#include <limits.h>

void edobject_unref(edobject * object)
{
	if (object->refcount > 1)
	{
		object->refcount--;
		return;
	}

	if (object->finalize)
		object->finalize(object);

	free(object);
}

edobject * edobject_ref(edobject * object)
{
	if (object->refcount < UINT_MAX)
	{
		object->refcount++;
		return object;
	}

	return NULL;
}

void edobject_init(edobject * object, edobject_finalize_cb finalize)
{
	object->refcount = 1;
	object->finalize = finalize;
}
