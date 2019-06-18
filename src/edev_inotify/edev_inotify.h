#ifndef _EDEV_INOTIFY_H_
#define _EDEV_INOTIFY_H_

#include "edloop.h"
#include <sys/inotify.h>

typedef struct edev_inotify edev_inotify;
typedef void (*edev_inotify_cb) (edev_inotify *, const struct inotify_event *);

_EDEV_SOURCE_EXTEND_METHOD_MACRO_(edev_inotify, edev_inotify);
void           edev_inotify_detach(edev_inotify *);
int            edev_inotify_attach(edev_inotify *);
int            edev_inotify_watch_add(edev_inotify *, const char * pathname, uint32_t mask);
int            edev_inotify_watch_del(edev_inotify *, int wd);
edev_inotify * edev_inotify_new(edloop *, edev_inotify_cb);

#endif
