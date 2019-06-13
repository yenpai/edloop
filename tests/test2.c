#include <stdio.h>
#include "edloop.h"
#include "edev_mqueue.h"

void mqueue_handle(edev_mqueue * mq, edev_mqueue_tlv * tlv)
{
	edev_mqueue_tlv mqdata;
	printf("Recv type = %d\n", tlv->type);

	if (tlv->type < 100)
	{
		mqdata.type   = tlv->type + 1000;
		mqdata.length = 0;
		edev_mqueue_append(mq, &mqdata);
	}
	else if (tlv->type == 100)
	{
		edloop_cancel(edloop_default());
	}
}

void timer_done(edev_timeout * timer)
{
	edev_mqueue * mq = timer->data.ptr;
	edev_mqueue_tlv mqdata;
	static int count = 0;

	if (count < 100)
	{
		mqdata.type   = ++count;
		mqdata.length = 0;

		edev_mqueue_append(mq, &mqdata);
		edev_timeout_start(timer, 100);

		printf("next time\n");
	}
}

static void * thread_main(void * arg)
{
	int index;
	edev_mqueue * mqueue = edev_mqueue_ref(arg);
	edev_mqueue_tlv mqdata;

	printf("%s():thread start now.\n", __func__);

	pthread_detach(pthread_self());
	edev_mqueue_ref(mqueue);

	for (index = 2000 ; index < 2200 ; index++)
	{
		mqdata.type   = index;
		mqdata.length = 0;
		edev_mqueue_append(mqueue, &mqdata);
		usleep(50 * 1000);
	}
	
	edev_mqueue_unref(mqueue);
	printf("%s():thread finish now.\n", __func__);

	return NULL;
}

int main(void)
{
	edloop * loop = edloop_default();
	edev_mqueue  * mqueue;
	edev_timeout * timer;
	pthread_t pid;

	mqueue = edev_mqueue_new(loop, mqueue_handle);
	edev_mqueue_attach(mqueue);
	edev_mqueue_unref(mqueue);
	printf("%s(): mqueue attach now.\n", __func__);

	timer = edev_timeout_new(loop, timer_done);
	timer->data.ptr = mqueue;
	edev_timeout_start(timer, 1000);
	edev_timeout_unref(timer);
	printf("%s(): timer start now. interval[%d]\n", __func__, 1000);

	pthread_create(&pid, NULL, thread_main, mqueue);

	edloop_loop(loop);
	edloop_done(loop);
	edloop_unref(loop);

	printf("exit\n");
	return 0;
}

