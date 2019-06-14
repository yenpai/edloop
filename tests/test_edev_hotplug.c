#include <stdio.h>
#include "edloop.h"
#include "edev_hotplug.h"

void hotplug_handle(edev_hotplug * UNUSED(hotplug), edev_hotplug_info * info)
{
	printf("hotplug event: action[%u] sysname[%s] devpath[%s] busnum[%u] devnum[%u] idVendor[0x%04x] idProduct[0x%04x]\n", 
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
	edev_hotplug * hp = edev_hotplug_new(loop, hotplug_handle);

	ret = edev_hotplug_attach(hp);
	printf("Attach hotplug to loop. ret[%d]\n", ret);

	edev_hotplug_unref(hp);
	hp = NULL;

	printf("Start loop ...\n");
	edloop_loop(loop);
	edloop_done(loop);
	edloop_unref(loop);
	loop = NULL;

	printf("exit\n");
	return 0;
}

