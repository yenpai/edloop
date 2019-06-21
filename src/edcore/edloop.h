#ifndef _EDLOOP_H_
#define _EDLOOP_H_

#include "edbase.h"
#include "edobject.h"
#include "list.h"

/*****************************************************************************/

typedef struct edloop       edloop;
typedef struct edev_source  edev_source;
typedef struct edev_process edev_process;
typedef struct edev_oneshot edev_oneshot;
typedef struct edev_timeout edev_timeout;
typedef struct edev_ioevent edev_ioevent;

/*****************************************************************************/

typedef void (* edev_process_cb)(edev_process *, int rtn);
typedef void (* edev_oneshot_cb)(edev_oneshot *);
typedef void (* edev_timeout_cb)(edev_timeout *);
typedef void (* edev_ioevent_cb)(edev_ioevent *, int fd, unsigned int revents);

/*****************************************************************************/

typedef union {
	void *   ptr;
	uint8_t  u8;
	uint16_t u16;
	uint32_t u32;
	int8_t   s8;
	int16_t  s16;
	int32_t  s32;
} edcustom_data;

typedef enum {
	EDEV_PROCESS_TYPE = 0,
	EDEV_ONESHOT_TYPE = 1,
	EDEV_TIMEOUT_TYPE = 2,
	EDEV_IOEVENT_TYPE = 3,
	EDEV_RECLAIM_TYPE = 4,
#define EDEV_TYPE_MAX   5
} edev_source_type;

struct edev_source {
	edobject          object;
	struct list_head  entry;
	bool              attach;
	bool              reclaim;
	edloop *          loop;
	edev_source_type  type;
};

struct edev_process {
	edev_source       source;
	pid_t             pid;
	edev_process_cb   done;
	edcustom_data     cdata;
};

struct edev_oneshot {
	edev_source       source;
	bool              action;
	edev_oneshot_cb   done;
	edcustom_data     cdata;
};

struct edev_timeout {
	edev_source       source;
	struct timeval    time;
	edev_timeout_cb   done;
	edcustom_data     cdata;
};

struct edev_ioevent {
	edev_source       source;
	int               fd;
	unsigned int      flags;
	unsigned int      revents;
	bool              bind;
	bool              err;
	bool              eof;
	edev_ioevent_cb   handle;
	edcustom_data     cdata;
};

/*****************************************************************************/

typedef enum {
	EDIO_READ     = (1 << 0),
	EDIO_WRITE    = (1 << 1),
	EDIO_EOF      = (1 << 2),
	EDIO_ERR      = (1 << 3),
	EDIO_NONBLOCK = (1 << 4), /* Set O_NONBLOCK */
	EDIO_CLOEXEC  = (1 << 5), /* Set FD_CLOEXEC */
	EDIO_CLOAUTO  = (1 << 6), /* Close FD when object finalize */
} edev_ioevent_flag;

/*****************************************************************************/

_EDOBJECT_EXTEND_METHOD_MACRO_(edloop, edloop);
void     edloop_detach(edloop *, edev_source *);
int      edloop_attach(edloop *, edev_source *);
void     edloop_cancel(edloop *);
void     edloop_wakeup(edloop *);
void     edloop_done(edloop *);
int      edloop_loop(edloop *);
edloop * edloop_default(void);
edloop * edloop_new(void);

/*****************************************************************************/
_EDOBJECT_EXTEND_METHOD_MACRO_(edev_source, edev_source);
void edev_source_init(edev_source *, edloop *, edev_source_type);
#define _EDEV_SOURCE_EXTEND_METHOD_MACRO_(TYPE, NAME) \
	_EDOBJECT_EXTEND_METHOD_MACRO_(TYPE, NAME); \
	static inline edev_source * NAME##_to_source(TYPE * o) { return (edev_source *) o; } \
	static inline edloop * NAME##_to_loop(TYPE * o) { return ((edev_source *) o)->loop; }

/*****************************************************************************/

_EDEV_SOURCE_EXTEND_METHOD_MACRO_(edev_process, edev_process);
int            edev_process_attach(edev_process *, pid_t pid);
void           edev_process_detach(edev_process *);
void           edev_process_init(edev_process *, edloop *, edev_process_cb);
edev_process * edev_process_new(edloop *, edev_process_cb);

/*****************************************************************************/

_EDEV_SOURCE_EXTEND_METHOD_MACRO_(edev_oneshot, edev_oneshot);
int            edev_oneshot_action(edev_oneshot *);
int            edev_oneshot_attach(edev_oneshot *);
void           edev_oneshot_detach(edev_oneshot *);
void           edev_oneshot_init(edev_oneshot *, edloop *, edev_oneshot_cb);
edev_oneshot * edev_oneshot_new(edloop *, edev_oneshot_cb);

/*****************************************************************************/

_EDEV_SOURCE_EXTEND_METHOD_MACRO_(edev_timeout, edev_timeout);
int            edev_timeout_get_remain(edev_timeout *);
int            edev_timeout_start(edev_timeout *, unsigned int msec);
void           edev_timeout_stop(edev_timeout *);
void           edev_timeout_init(edev_timeout *, edloop *, edev_timeout_cb);
edev_timeout * edev_timeout_new(edloop *, edev_timeout_cb);

/*****************************************************************************/

_EDEV_SOURCE_EXTEND_METHOD_MACRO_(edev_ioevent, edev_ioevent);
int            edev_ioevent_get_unix_fd(edev_ioevent *);
int            edev_ioevent_attach(edev_ioevent *, int fd, unsigned int flags);
void           edev_ioevent_detach(edev_ioevent *);
void           edev_ioevent_close(edev_ioevent *);
void           edev_ioevent_init(edev_ioevent *, edloop *, edev_ioevent_cb);
edev_ioevent * edev_ioevent_new(edloop *, edev_ioevent_cb);

/*****************************************************************************/

void edutil_time_curr(struct timeval * tv);
void edutil_time_next(struct timeval * tv, unsigned int msec);
long edutil_time_diff(struct timeval * t1, struct timeval * t2);

#endif
