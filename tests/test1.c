#include <stdio.h>
#include "edloop.h"

void process1_done(edev_process * UNUSED(process), int code)
{
	printf("%s(): process done. code = %d\n", __func__, code);
	printf("%s(): call system(ls) ret = %d\n", __func__, system("ls"));
}

pid_t process1_start()
{
	pid_t pid;

	if ((pid = fork()) < 0)
	{
		printf("%s(): fork error!\n", __func__);
		exit(1);
	}
	else if (pid == 0) /* child */
	{
		printf("%s(): child process was fork and exec.\n", __func__);
		execlp("/bin/ls", "ls", NULL);
		exit(127);
	}

	return pid;
}

void shot1_handle(edev_oneshot * shot)
{
	static int count = 0;
	printf("%s(): shot1 done. count[%d]\n", __func__, count);

	if (count == 0)
	{
		count++;
		edev_oneshot_attach(shot);
		edev_oneshot_action(shot);
		printf("%s(): shot1 restart again.\n", __func__);
	}

}

void ioev1_handle(edev_ioevent * io, int UNUSED(fd), unsigned int revents)
{
	FILE * stream = io->cdata.ptr;
	char buf[256];

	if (revents & EDIO_READ)
	{
		memset(buf, 0, sizeof(buf));

		while (fgets(buf, sizeof(buf), stream) != NULL)
		{
			printf("%s(): [Read] => %s", __func__, buf);
		}

		if (feof(stream))
		{
			printf("%s(): stream read eof, close and detach.\n", __func__);
			goto CLOSE_AND_DETACH;
		}
	}

	if (revents & EDIO_EOF)
	{
		printf("%s(): EOF event, close and detach.\n", __func__);
		goto CLOSE_AND_DETACH;
	}

	if (revents & EDIO_ERR)
	{
		printf("%s(): ERR event, close and detach.\n", __func__);
		goto CLOSE_AND_DETACH;
	}

	return;

CLOSE_AND_DETACH:
	
	fclose(stream);
	edev_ioevent_detach(io);
}

void timer3_done(edev_timeout * timer)
{
	edev_timeout * timer2 = timer->cdata.ptr;
	printf("%s(): timeout.\n", __func__);

	if (timer2)
	{
		edev_timeout_stop(timer2);
		edev_timeout_unref(timer2);
		timer->cdata.ptr = NULL;
		printf("%s(): timer2 stop now.\n", __func__);
	}
	
	printf("%s(): timer3 finish now.\n", __func__);
}

void timer2_done(edev_timeout * timer2)
{
	int ret;
	printf("%s(): timeout.\n", __func__);
	ret = edev_timeout_start(timer2, 1500);
	printf("%s(): timer2 restart, interval[1500], ret[%d]\n", __func__, ret);
}

void timer1_done(edev_timeout * timer1)
{
	int ret;
	static int count = 0;
	static edev_timeout * timer2 = NULL;
	edloop * loop = edev_timeout_to_loop(timer1);

	printf("%s(): timeout count[%d].\n", __func__, count);

	switch (count)
	{
		case 0:
		{
			printf("%s(): call system(date) ret = %d\n", __func__, system("date"));
			break;
		}

		case 1:
		{
			timer2 = edev_timeout_new(loop, timer2_done);
			timer2->cdata.ptr = NULL;
			edev_timeout_start(timer2, 1500);
			printf("%s(): timer2 start now. interval[%d]\n", __func__, 1500);
			break;
		}

		case 2:
		{
			edev_timeout * timer3;
			timer3 = edev_timeout_new(loop, timer3_done);
			timer3->cdata.ptr = edev_timeout_ref(timer2);
			edev_timeout_start(timer3, 3500);
			edev_timeout_unref(timer3);
			printf("%s(): timer3 start now. interval[%d]\n", __func__, 3500);
			break;
		}

		case 3:
		{
			edev_timeout_unref(timer2);
			timer2 = NULL;
			break;
		}
		
		case 4:
		{
			edev_ioevent * ioev1;
			FILE * stream;

			ioev1 = edev_ioevent_new(loop, ioev1_handle);
			ioev1->cdata.ptr = (stream = popen("ls -al /", "r"));
			edev_ioevent_attach(ioev1, fileno(stream), EDIO_READ | EDIO_NONBLOCK | EDIO_CLOEXEC); 
			edev_ioevent_unref(ioev1);
			printf("%s(): ioevent1 start now. popen(ls -al /)\n", __func__);
			break;
		}

		case 5:
		{
			edev_oneshot * shot1;
			shot1 = edev_oneshot_new(loop, shot1_handle);
			edev_oneshot_attach(shot1);
			edev_oneshot_action(shot1);
			edev_oneshot_unref(shot1);
			printf("%s(): oneshot1 action now.\n", __func__);
			break;
		}

		case 6:
		{
			edev_process * process = edev_process_new(edloop_default(), process1_done);
			edev_process_attach(process, process1_start());
			edev_process_unref(process);
			printf("%s(): attach process1 now.\n", __func__);
			break;
		}

		case 9:
		{
			edloop_cancel(edev_timeout_to_loop(timer1));
			printf("%s(): edloop cancel now.\n", __func__);
			break;
		}
	}

	if (count < 10)
	{
		count++;
		ret = edev_timeout_start(timer1, 1000);
		printf("%s(): timer1 restart, interval[1000], ret[%d]\n", __func__, ret);
	}
}

int main(void)
{
	edloop * loop = edloop_default();
	edev_timeout * timer1;

	timer1 = edev_timeout_new(loop, timer1_done);
	edev_timeout_start(timer1, 1000);
	edev_timeout_unref(timer1);
	printf("%s(): timer1 start now. interval[%d]\n", __func__, 1000);

	edloop_loop(loop);
	edloop_done(loop);
	edloop_unref(loop);

	printf("exit\n");
	return 0;
}

