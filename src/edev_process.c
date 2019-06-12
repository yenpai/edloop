#include "edloop.h"

static void edev_process_finalize(edobject * UNUSED(obj))
{
}

int edev_process_attach(edev_process * process, pid_t pid)
{
	edev_source * source = edev_process_to_source(process);
	edloop * loop = edev_process_to_loop(process);

	if (loop == NULL)
		return -1;

	if (pid == 0)
		return -2;

	process->pid = pid;

	if (edloop_attach(loop, source) < 0)
		return -3;

	return 0;
}

void edev_process_detach(edev_process * process)
{
	edev_source * source = edev_process_to_source(process);
	edloop_detach(source->loop, source);
}

edev_process * edev_process_new(edloop * loop, edev_process_cb done)
{
	edev_process * process;

	if (loop == NULL)
		return NULL;
	if ((process = malloc(sizeof(*process))) == NULL)
		return NULL;

	memset(process, 0, sizeof(*process));
	edev_source_base_init(&process->source, loop, EDEV_PROCESS_TYPE, edev_process_finalize);

	process->pid  = 0;
	process->done = done;
	return process;
}
