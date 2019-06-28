#ifndef _LINUX_SKBUFF_H_
#define _LINUX_SKBUFF_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

typedef struct {
	struct list_head entry;
	uint8_t * head;
	uint8_t * data;
	uint8_t * tail;
	uint8_t * end;
} skbuf;

static inline uint32_t skb_headroom(const skbuf * skb)
{
	return skb->head - skb->data;
}

static inline uint32_t skb_tailroom(const skbuf * skb)
{
	return skb->end - skb->tail;
}

static inline uint32_t skb_dataroom(const skbuf * skb)
{
	return skb->tail - skb->data;
}

static inline uint8_t * skb_pull(skbuf * skb, uint32_t len)
{
	uint8_t * ptr = skb->data;
	skb->data += len;
	return ptr;
}

static inline uint8_t * skb_push(skbuf * skb, uint32_t len)
{
	uint8_t * ptr = skb->data;
	skb->data -= len;
	return ptr;
}

static inline uint8_t * skb_put(skbuf * skb, uint32_t len)
{
	uint8_t * ptr = skb->tail;
	skb->tail += len;
	return ptr;
}

static inline uint8_t * skb_reset(skbuf * skb)
{
	skb->data = skb->head;
	skb->tail = skb->head;
	return skb->data;
}

static inline void skb_free(skbuf * skb)
{
	free(skb);
}

static inline skbuf * skb_new(uint32_t size)
{
	skbuf * skb;

	if ((skb = malloc(sizeof(skbuf) + size)) != NULL)
	{
		memset(skb, 0, sizeof(skbuf));
		INIT_LIST_HEAD(&skb->entry);
		skb->head = (uint8_t *) skb + sizeof(skbuf);
		skb->data = skb->head;
		skb->tail = skb->head;
		skb->end  = skb->head + size;
	}

	return skb;
}

#endif
