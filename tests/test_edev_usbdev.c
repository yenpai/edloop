#include <stdio.h>
#include "edloop.h"
#include "edev_usbdev.h"

void usbdev_handle(edev_usbdev * UNUSED(usbdev), edev_usbdev_info * info)
{
	printf("usbdev detect sysname[%s] bus[%u] dev[%u] action[%u]\n", 
			info->sysname, 
			info->busnum,
			info->devaddr,
			info->detached);
}

int main(void)
{
	edloop * loop = edloop_default();
	edev_usbdev * usbdev = edev_usbdev_new(loop, usbdev_handle);

	edev_usbdev_attach(usbdev);
	edev_usbdev_unref(usbdev);
	usbdev = NULL;

	printf("Start loop ...\n");
	edloop_loop(loop);
	edloop_done(loop);
	edloop_unref(loop);
	loop = NULL;

	printf("exit\n");
	return 0;
}

