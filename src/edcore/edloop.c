#include <errno.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/wait.h>
#include "edloop.h"

/* uClibc < 0.9.30.3 does not define EPOLLRDHUP for Linux >= 2.6.17 */
#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0x2000
#endif

/*****************************************************************************/

#ifdef DEBUG_TRACE_SOURCE_LIVE
#include <stdio.h>
#define LOG_SOURCE_ATTACH(O)  do { printf("Source[%p] type[%d] Attached.\n",  O, O->type); } while(0)
#define LOG_SOURCE_RECLAIM(O) do { printf("Source[%p] type[%d] Reclaimed.\n", O, O->type); } while(0)
#else
#define LOG_SOURCE_ATTACH(O)  do {} while(0)
#define LOG_SOURCE_RECLAIM(O) do {} while(0)
#endif

/*****************************************************************************/

struct edloop {
	edobject          object;
    int               epfd;

    struct list_head  source_list[EDEV_TYPE_MAX];
	pthread_mutex_t   source_mutex;

	int               access;
	pthread_mutex_t   access_mutex;
	pthread_cond_t    access_cond;

    edev_ioevent *    waker;
    int               status;
    bool              cancel;
	edcustom_data  cdata;
};

/*****************************************************************************/

static edloop * global_loop_instance = NULL;
static bool     global_sigchld_exist = false;

static void signal_handler(int signo)
{
	if (signo == SIGCHLD && global_loop_instance)
	{
		global_sigchld_exist = true;
		edloop_wakeup(global_loop_instance);
	}
}

static void signal_install(int signo, void (*handler)(int), struct sigaction* old, bool add)
{
	struct sigaction s;
	struct sigaction * act = NULL;

	sigaction(signo, NULL, &s);

	if (add && s.sa_handler == SIG_DFL)
	{
		/* Backup and Install */
		memcpy(old, &s, sizeof(struct sigaction));
		s.sa_handler = handler;
		s.sa_flags = 0;
		act = &s;
	}
	else if (add == false && s.sa_handler == handler)
	{
		/* Restore */
		act = old;
	}

	if (act != NULL)
		sigaction(signo, act, NULL);
}

static void signal_ignore(int signo, bool ignore)
{
	struct sigaction s;
	void *new_handler = NULL;

	sigaction(signo, NULL, &s);
	if (ignore && s.sa_handler == SIG_DFL)
		new_handler = SIG_IGN; /* Ignore */
	else if (!ignore && s.sa_handler == SIG_IGN)
		new_handler = SIG_DFL; /* Restore */

	if (new_handler)
	{
		s.sa_handler = new_handler;
		s.sa_flags = 0;
		sigaction(signo, &s, NULL);
	}
}

static void signal_setup(bool add)
{
	static struct sigaction old_sigchld;
	signal_install(SIGCHLD, signal_handler, &old_sigchld, add);
	signal_ignore(SIGPIPE, add);
}

/*****************************************************************************/

static void waker_consume(edev_ioevent * UNUSED(waker), int fd, unsigned int UNUSED(revents))
{
	uint64_t count;
	while (read(fd, &count, sizeof(uint64_t)) > 0) ;
}

static void waker_update(edev_ioevent * waker)
{
	uint64_t count;
	int fd;

	if ((fd = edev_ioevent_get_unix_fd(waker)) >= 0)
	{
		count = 1;
		while (write(fd, &count, sizeof(uint64_t)) < 0)
		{
			if (errno != EINTR)
			{
				break;
			}
		}
	}
}

static int waker_init(edloop * loop)
{
	edev_ioevent * io;
	int fd;

	if ((fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) < 0)
		return -1;

	if ((io = edev_ioevent_new(loop, waker_consume)) == NULL)
	{
		close(fd);
		return -2;
	}

	if (edev_ioevent_attach(io, fd, EDIO_READ | EDIO_CLOAUTO) < 0)
	{
		close(fd);
		edev_ioevent_unref(io);
		return -3;
	}

	loop->waker = io;
	return 0;
}

/*****************************************************************************/

static void loop_reclaim_add(edev_source * source)
{
	edloop * loop = source->loop;
	struct list_head * list = &loop->source_list[EDEV_RECLAIM_TYPE];

	if (source->reclaim)
		return;

	if (!list_empty(&source->entry))
		list_del_init(&source->entry);

	source->attach  = true;
	source->reclaim = true;
	list_add(&source->entry, list);
}

/*****************************************************************************/

static int loop_oneshot_add(edloop * loop, edev_oneshot * oneshot)
{
	struct list_head * list = &loop->source_list[EDEV_ONESHOT_TYPE];
	edev_source * source = &oneshot->source;

	if (!source->attach && edev_oneshot_ref(oneshot) == NULL)
		return -1;

	if (!list_empty(&source->entry))
		list_del_init(&source->entry);

	if (source->attach == false)
		LOG_SOURCE_ATTACH(source);

	source->attach  = true;
	source->reclaim = false;

	if (oneshot->action)
		list_add(&source->entry, list);
	else
		list_add_tail(&source->entry, list);

	return 0;
}

static void loop_oneshot_dispatch(edloop * loop)
{
	struct list_head * list = &loop->source_list[EDEV_ONESHOT_TYPE];
	struct list_head active = LIST_HEAD_INIT(active);
	edev_source  * source, * tmp;
	edev_oneshot * oneshot;

	list_for_each_entry_safe(source, tmp, list, entry)
	{
		if (loop->cancel)
			break;

		if (source->reclaim)
			continue;

		if ((oneshot = container_of(source, edev_oneshot, source))->action == false)
			break;

		list_del_init(&source->entry);
		list_add(&active, &source->entry);
	}

	while (!list_empty(&active))
	{
		source = list_first_entry(&active, edev_source, entry);
		oneshot = container_of(source, edev_oneshot, source);

		oneshot->action = false;
		loop_reclaim_add(source);

		if (loop->cancel == false && oneshot->done)
			oneshot->done(oneshot);
	}
}

/*****************************************************************************/

static int loop_process_add(edloop * loop, edev_process * process)
{
	struct list_head * list = &loop->source_list[EDEV_PROCESS_TYPE];
	struct list_head * head = &loop->source_list[EDEV_PROCESS_TYPE];
	edev_source  * source = edev_process_to_source(process);
	edev_process * tmp;

	if (loop != global_loop_instance)
		return -9;
	
	if (!source->attach && edev_process_ref(process) == NULL)
		return -1;

	if (!list_empty(&source->entry))
		list_del_init(&source->entry);

	list_for_each_entry(tmp, list, source.entry)
	{
		if (tmp->pid > process->pid)
		{
			head = &tmp->source.entry;
			break;
		}
	}

	if (source->attach == false)
		LOG_SOURCE_ATTACH(source);

	source->attach  = true;
	source->reclaim = false;
	list_add_tail(&source->entry, head);

	return 0;
}

static void loop_process_dispatch(edloop * loop)
{
	struct list_head * list = &loop->source_list[EDEV_PROCESS_TYPE];
	edev_source  * source, * tmp;
	edev_process * process, * found;
	pid_t pid;
	int stat;

	global_sigchld_exist = false;

	while (!loop->cancel)
	{
		if ((pid = waitpid(-1, &stat, WNOHANG)) < 0 && errno == EINTR)
			continue;

		if (pid <= 0)
			return;

		found = NULL;

		list_for_each_entry_safe(source, tmp, list, entry)
		{
			if (loop->cancel)
				break;

			if (source->reclaim)
				continue;

			process = container_of(source, edev_process, source);

			if (process->pid < pid)
				continue;

			if (process->pid == pid)
				found = process;

			break;
		}

		if (loop->cancel == false && found)
		{
			process = found;
			loop_reclaim_add(&process->source);
			
			if (process->done)
				process->done(process, stat);
		}
	}
}

/*****************************************************************************/

static void loop_timeout_next_time(edloop * loop, int * timeout_val)
{
	struct list_head * list = &loop->source_list[EDEV_TIMEOUT_TYPE];
	edev_timeout * timeout;
	struct timeval tv;
	int msec;

	if (list_empty(list))
	{
		*timeout_val = -1;
		return;
	}

	edutil_time_curr(&tv);
	timeout = list_first_entry(list, edev_timeout, source.entry);
	msec = edutil_time_diff(&timeout->time, &tv);
	*timeout_val = (msec) < 0 ? 0 : msec ;
}

static int loop_timeout_add(edloop * loop, edev_timeout * timeout)
{
	struct list_head * list = &loop->source_list[EDEV_TIMEOUT_TYPE];
	struct list_head * head = &loop->source_list[EDEV_TIMEOUT_TYPE];
	edev_source    * source = edev_timeout_to_source(timeout);
	edev_timeout   * tmp;

	if (!source->attach && edev_timeout_ref(timeout) == NULL)
		return -1;

	if (!list_empty(&source->entry))
		list_del_init(&source->entry);

	list_for_each_entry(tmp, list, source.entry)
	{
		if (edutil_time_diff(&tmp->time, &timeout->time) > 0)
		{
			head = &tmp->source.entry;
			break;
		}
	}

	if (source->attach == false)
		LOG_SOURCE_ATTACH(source);

	source->attach  = true;
	source->reclaim = false;
	list_add_tail(&source->entry, head);

	return 0;
}

static void loop_timeout_dispatch(edloop * loop)
{
	struct list_head * list = &loop->source_list[EDEV_TIMEOUT_TYPE];
	struct list_head active = LIST_HEAD_INIT(active);
	edev_source  * source, * tmp;
	edev_timeout * timeout;
	struct timeval tv;

	edutil_time_curr(&tv);

	list_for_each_entry_safe(source, tmp, list, entry)
	{
		if (loop->cancel)
			break;

		if (source->reclaim)
			continue;

		timeout = container_of(source, edev_timeout, source);

		if (edutil_time_diff(&timeout->time, &tv) > 0)
			break;

		list_del_init(&source->entry);
		list_add(&active, &source->entry);
	}

	while (!list_empty(&active))
	{
		source = list_first_entry(&active, edev_source, entry);
		timeout = container_of(source, edev_timeout, source);

		loop_reclaim_add(source);

		if (loop->cancel == false && timeout->done)
			timeout->done(timeout);
	}
}

/*****************************************************************************/

static int loop_ioevent_add(edloop * loop, edev_ioevent * io)
{
	edev_source * source = edev_ioevent_to_source(io);
	struct epoll_event event;

	// TODO: How to support modify ?? (attach again)
	
	if (loop == NULL || source->attach)
		return -1;
	if (edev_ioevent_ref(io) == NULL)
		return -2;

	memset(&event, 0, sizeof(struct epoll_event));
	event.data.ptr = io;

	if (io->flags & EDIO_READ)
		event.events |= EPOLLIN | EPOLLRDHUP;
	if (io->flags & EDIO_WRITE)
		event.events |= EPOLLOUT;
	if (io->flags & EDIO_NONBLOCK)
		fcntl(io->fd, F_SETFL, fcntl(io->fd, F_GETFL) | O_NONBLOCK);
	if (io->flags & EDIO_CLOEXEC)
		fcntl(io->fd, F_SETFD, fcntl(io->fd, F_GETFD) | FD_CLOEXEC);

	if (epoll_ctl(loop->epfd, EPOLL_CTL_ADD, io->fd, &event) < 0)
	{
		edev_ioevent_unref(io);
		return -2;
	}

	io->revents      = 0;
	io->err          = false;
	io->eof          = false;

	if (source->attach == false)
		LOG_SOURCE_ATTACH(source);

	source->attach   = true;
	source->reclaim  = false;
	list_add_tail(&source->entry, &loop->source_list[EDEV_IOEVENT_TYPE]);

	return 0;
}

static void loop_ioevent_disptach(edloop * loop)
{
	int n, nfds, timeout_msec = -1;
	struct epoll_event events[8];
	edev_source  * source;
	edev_ioevent * io;

	memset(events, 0, sizeof(events));
	
	loop_timeout_next_time(loop, &timeout_msec);
	nfds = epoll_wait(loop->epfd, events, ARRAY_SIZE(events), timeout_msec);

	for (n = 0; n < nfds; n++)
	{
		if ((io = events[n].data.ptr) == NULL)
			continue;

		io->revents = 0;

		if (events[n].events & (EPOLLERR|EPOLLHUP))
		{
			io->revents |= EDIO_ERR;
			io->err = true;
		}

		if (events[n].events & EPOLLRDHUP)
		{
			io->revents |= EDIO_EOF;
			io->eof = true;
		}

		if (events[n].events & EPOLLIN)
			io->revents |= EDIO_READ;

		if (events[n].events & EPOLLOUT)
			io->revents |= EDIO_WRITE;
	}

	for (n = 0; n < nfds; n++)
	{
		if (loop->cancel)
			break;

		if ((io = events[n].data.ptr) == NULL)
			continue;

		if (io->revents == 0)
			continue;

		if ((source = &io->source)->reclaim)
			continue;

		if (io->err || io->eof)
			loop_reclaim_add(source);

		if (io->handle)
			io->handle(io, io->fd, io->revents);

		io->revents = 0;
	}
}

/*****************************************************************************/

static void loop_source_del(edev_source * source)
{
	edloop * loop = source->loop;

	if (source->type == EDEV_IOEVENT_TYPE)
		epoll_ctl(loop->epfd, EPOLL_CTL_DEL, ((edev_ioevent *) source)->fd, 0);

	if (!list_empty(&source->entry))
		list_del_init(&source->entry);

	LOG_SOURCE_RECLAIM(source);

	source->reclaim = true;
	source->attach  = false;
	edev_source_unref(source);
}

static void loop_source_cleanup(edloop * loop, edev_source_type type)
{
	edev_source * source, * tmp;
	list_for_each_entry_safe(source, tmp, &loop->source_list[type], entry)
		loop_source_del(source);
}

/*****************************************************************************/

static void edloop_finalize(edobject * obj)
{
	edloop * loop = container_of(obj, edloop, object);
	int type;

	for (type = 0 ; type < EDEV_TYPE_MAX ; type++)
		loop_source_cleanup(loop, type);

	if (loop->waker)
	{
		edev_ioevent_unref(loop->waker);
		loop->waker = NULL;
	}

	if (loop->epfd > 0)
	{
		close(loop->epfd);
		loop->epfd = -1;
	}

	if (global_loop_instance == loop)
	{
		signal_setup(false);
		global_loop_instance = NULL;
		global_sigchld_exist = false;
	}
}

void edloop_detach(edloop * loop, edev_source * source)
{
	pthread_mutex_lock(&loop->access_mutex);
	loop->access++;
	pthread_mutex_unlock(&loop->access_mutex);

	edloop_wakeup(loop);

	pthread_mutex_lock(&loop->source_mutex);
	loop_reclaim_add(source);
	pthread_mutex_unlock(&loop->source_mutex);

	pthread_mutex_lock(&loop->access_mutex);
	if (--loop->access <= 0)
		pthread_cond_signal(&loop->access_cond);
	pthread_mutex_unlock(&loop->access_mutex);
}

int edloop_attach(edloop * loop, edev_source * source)
{
	int ret = -1;

	if (source->loop == NULL)
		source->loop = loop;

	pthread_mutex_lock(&loop->access_mutex);
	loop->access++;
	pthread_mutex_unlock(&loop->access_mutex);

	edloop_wakeup(loop);

	pthread_mutex_lock(&loop->source_mutex);
	switch (source->type)
	{
		case EDEV_PROCESS_TYPE:
			ret = loop_process_add(loop, (edev_process *) source);
			break;
		case EDEV_ONESHOT_TYPE:
			ret = loop_oneshot_add(loop, (edev_oneshot *) source);
			break;
		case EDEV_TIMEOUT_TYPE:
			ret = loop_timeout_add(loop, (edev_timeout *) source);
			break;
		case EDEV_IOEVENT_TYPE:
			ret = loop_ioevent_add(loop, (edev_ioevent *) source);
			break;
		case EDEV_RECLAIM_TYPE:
			ret = -1;
			break;
	}
	pthread_mutex_unlock(&loop->source_mutex);

	pthread_mutex_lock(&loop->access_mutex);
	if (--loop->access <= 0)
		pthread_cond_signal(&loop->access_cond);
	pthread_mutex_unlock(&loop->access_mutex);

	return ret;
}

void edloop_cancel(edloop * loop)
{
	loop->cancel = true;
	edloop_wakeup(loop);
}

void edloop_wakeup(edloop * loop)
{
	if (loop && loop->waker && loop->status > 0)
		waker_update(loop->waker);
}

void edloop_done(edloop * loop)
{
	int type;
	for (type = 0 ; type < EDEV_TYPE_MAX ; type++)
		loop_source_cleanup(loop, type);
}

int edloop_loop(edloop * loop)
{
	loop->cancel = false;
	loop->status++;

	pthread_mutex_lock(&loop->source_mutex);

	while (!loop->cancel)
	{
		/* critical section for other thread access */
		if (loop->access > 0)
		{
			pthread_mutex_unlock(&loop->source_mutex);
			pthread_mutex_lock(&loop->access_mutex);
			while (loop->access > 0)
				pthread_cond_wait(&loop->access_cond, &loop->access_mutex);
			loop->access = 0;
			pthread_mutex_unlock(&loop->access_mutex);
			pthread_mutex_lock(&loop->source_mutex);
		}

		/* reclaim */
		if (!list_empty(&loop->source_list[EDEV_RECLAIM_TYPE]))
			loop_source_cleanup(loop, EDEV_RECLAIM_TYPE);

		/* process */
		if (global_loop_instance == loop && global_sigchld_exist)
			loop_process_dispatch(loop);

		/* oneshot */
		if (!list_empty(&loop->source_list[EDEV_ONESHOT_TYPE]))
			loop_oneshot_dispatch(loop);

		/* timeout */
		if (!list_empty(&loop->source_list[EDEV_TIMEOUT_TYPE]))
			loop_timeout_dispatch(loop);

		/* ioevent_pollwait */
		if (!list_empty(&loop->source_list[EDEV_IOEVENT_TYPE]))
			loop_ioevent_disptach(loop);
	}

	pthread_mutex_unlock(&loop->source_mutex);

	if (--loop->status > 0)
		edloop_wakeup(loop);

	return loop->status;
}

edloop * edloop_default(void)
{
	edloop * self;

	if (global_loop_instance)
		return global_loop_instance;

	if ((self = edloop_new()) == NULL)
		return NULL;

	global_loop_instance = self;
	global_sigchld_exist = false;
	signal_setup(true);
	return self;
}

edloop * edloop_new(void)
{
	edloop * loop;
	pthread_mutexattr_t attr;
	int index;

	if ((loop = malloc(sizeof(edloop))) == NULL)
		return NULL;

	memset(loop, 0, sizeof(edloop));
	edobject_init(edloop_to_object(loop));
	edobject_register_finalize(edloop_to_object(loop), edloop_finalize);

	if ((loop->epfd = epoll_create1(EPOLL_CLOEXEC)) < 0)
	{
		edloop_unref(loop);
		return NULL;
	}

	for (index = 0 ; index < EDEV_TYPE_MAX ; index++)
		INIT_LIST_HEAD(&loop->source_list[index]);

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&loop->source_mutex, &attr);
	pthread_mutexattr_destroy(&attr);

	loop->access = 0;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&loop->access_mutex, &attr);
	pthread_mutexattr_destroy(&attr);
	pthread_cond_init(&loop->access_cond, NULL);

	if (waker_init(loop) < 0)
	{
		edloop_unref(loop);
		return NULL;
	}

	loop->status = 0;
	loop->cancel = false;

	return loop;
}

/*****************************************************************************/

void edev_source_init(edev_source * source, edloop * loop, edev_source_type type)
{
	edobject_init(edev_source_to_object(source));
	INIT_LIST_HEAD(&source->entry);
	source->loop    = loop;
	source->type    = type;
	source->attach  = false;
	source->reclaim = false;
}
