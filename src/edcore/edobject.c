#include "edbase.h"
#include "edobject.h"
#include <limits.h>

typedef struct {
	struct list_head     entry;
	edobject_finalize_cb func;
} finalizeItem;

void edobject_register_finalize(edobject * object, edobject_finalize_cb func)
{
	finalizeItem * item;

	if (func && (item = malloc(sizeof(finalizeItem))) != NULL)
	{
		INIT_LIST_HEAD(&item->entry);
		item->func = func;
		list_add(&item->entry, &object->finalizes);
	}
}

void edobject_unref(edobject * object)
{
	finalizeItem * item;

	if (object->refcount > 1)
	{
		object->refcount--;
		return;
	}

	while (!list_empty(&object->finalizes))
	{
		item = list_first_entry(&object->finalizes, finalizeItem, entry);
		list_del_init(&item->entry);
		item->func(object);
		free(item);
	}

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

void edobject_init(edobject * object)
{
	object->refcount = 1;
	INIT_LIST_HEAD(&object->finalizes);
}
