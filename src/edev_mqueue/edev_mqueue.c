#include <errno.h>
#include "edev_mqueue.h"

typedef struct {
	struct list_head node;
	edev_mqueue_tlv  tlv;
} mqdata;

static void edev_mqueue_finalize(edobject * obj)
{
	edev_mqueue * mq = container_of(obj, edev_mqueue, oneshot.source.object);
	mqdata * bundle, * tmp;

	list_for_each_entry_safe(bundle, tmp, &mq->queue, node)
	{
		list_del(&bundle->node);
		free(bundle);
	}
}

static void edev_mqueue_handle(edev_oneshot * shot)
{
	edev_mqueue * mq = container_of(shot, edev_mqueue, oneshot);
	struct list_head head = LIST_HEAD_INIT(head);
	mqdata * bundle;

	/* Move all current mqdata to tmp list */
	pthread_mutex_lock(&mq->mutex);
	if (!list_empty(&mq->queue))
		list_splice_tail_init(&mq->queue, &head);
	pthread_mutex_unlock(&mq->mutex);

	/* Dispatch */
	while (!list_empty(&head))
	{
		bundle = list_first_entry(&head, mqdata, node);
		list_del(&bundle->node);

		if (mq->handle)
			mq->handle(mq, &bundle->tlv);

		free(bundle);
	}

	/* Keep attach in loop */
	edev_oneshot_attach(shot);
}

int edev_mqueue_append(edev_mqueue * mq, edev_mqueue_tlv * tlv)
{
	mqdata * bundle;

	if ((bundle = malloc(sizeof(mqdata) + tlv->length)) == NULL)
		return -1;

	memset(bundle, 0, sizeof(mqdata));
	INIT_LIST_HEAD(&bundle->node);
	bundle->tlv.type   = tlv->type;
	bundle->tlv.length = tlv->length;
	if (tlv->length > 0)
		memcpy(bundle->tlv.value, tlv->value, tlv->length);

	pthread_mutex_lock(&mq->mutex);
	list_add_tail(&bundle->node, &mq->queue);
	pthread_mutex_unlock(&mq->mutex);

	edev_oneshot_action(&mq->oneshot);
	edloop_wakeup(edev_mqueue_to_loop(mq));
	return 0;
}

int edev_mqueue_attach(edev_mqueue * mq)
{
	return edev_oneshot_attach(&mq->oneshot);
}

void edev_mqueue_detach(edev_mqueue * mq)
{
	edev_oneshot_detach(&mq->oneshot);
}

void edev_mqueue_init(edev_mqueue * mq, edloop * loop, edev_mqueue_cb handle)
{
	pthread_mutexattr_t attr;

	edev_source_init(&mq->oneshot.source, loop, EDEV_ONESHOT_TYPE, edev_mqueue_finalize);
	mq->oneshot.done = edev_mqueue_handle;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mq->mutex, &attr);
	pthread_mutexattr_destroy(&attr);

	INIT_LIST_HEAD(&mq->queue);
	mq->handle = handle;
}

edev_mqueue * edev_mqueue_new(edloop * loop, edev_mqueue_cb handle)
{
	edev_mqueue  * mq;

	if (loop == NULL)
		return NULL;

	if ((mq = malloc(sizeof(*mq))) == NULL)
		return NULL;

	memset(mq, 0, sizeof(*mq));
	edev_mqueue_init(mq, loop, handle);

	return mq;
}
