#include <stdio.h>
#include "edloop.h"
#include "edev_usbdev.h"

void usbdev_handle(edev_usbdev * UNUSED(usbdev), edev_usbdev_info * info)
{
	printf("usbdev event: action[%u] sysname[%s] devpath[%s] busnum[%u] devnum[%u] idVendor[0x%04x] idProduct[0x%04x]\n", 
			info->action,
			info->subsystem,
			info->devpath,
			info->busnum,
			info->devnum,
			info->idVendor,
			info->idProduct);
}

int main(void)
{
	int ret;
	edloop * loop = edloop_default();
	edev_usbdev * usbdev = edev_usbdev_new(loop, usbdev_handle);

	ret = edev_usbdev_attach(usbdev);
	printf("Attach usbdev to loop. ret[%d]\n", ret);

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

