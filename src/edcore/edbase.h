#ifndef _EDBASE_H_
#define _EDBASE_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>

/*****************************************************************************/
/* Unused Attribute Macros                                                   */
/*****************************************************************************/
#ifndef UNUSED
#ifdef __GNUC__
#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#define UNUSED(x) UNUSED_ ## x
#endif
#endif

#ifndef UNUSED_FUNCTION
#ifdef __GNUC__
#define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_ ## x
#else
#define UNUSED_FUNCTION(x) UNUSED_ ## x
#endif
#endif

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
#define container_of(PTR, TYPE, MEMBER) ({						\
		 const typeof(((TYPE *) NULL)->MEMBER) *__mptr = (PTR); \
		 (TYPE *) ((char *) __mptr - offsetof(TYPE, MEMBER)); })
#endif

/*****************************************************************************/

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) \
	       (sizeof(arr) / sizeof(((typeof(arr)){})[0]))
#endif

#endif
