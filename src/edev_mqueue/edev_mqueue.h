#ifndef _EDEV_MQUEUE_H_
#define _EDEV_MQUEUE_H_

#include "edloop.h"

typedef struct edev_mqueue edev_mqueue;
typedef struct {
	uint32_t type;
	uint32_t length;
	union {
		uint8_t  value[0];
		uint8_t  u8;
		uint16_t u16;
		uint32_t u32;
		int8_t   s8;
		int16_t  s16;
		int32_t  s32;
		void *   ptr;
	};
} edev_mqueue_tlv;

typedef void (* edev_mqueue_cb) (edev_mqueue *, edev_mqueue_tlv *);

_EDEV_SOURCE_EXTEND_METHOD_MACRO_(edev_mqueue, edev_mqueue);
int           edev_mqueue_append(edev_mqueue *, edev_mqueue_tlv *);
void          edev_mqueue_clean(edev_mqueue *);
int           edev_mqueue_attach(edev_mqueue *);
void          edev_mqueue_detach(edev_mqueue *);
edev_mqueue * edev_mqueue_new(edloop *, edev_mqueue_cb);

#endif
