#include <stdio.h>
#include "edloop.h"
#include "edev_hotplug.h"

void hotplug_handle(edev_hotplug * UNUSED(hotplug), edev_hotplug_info * info)
{
	uint8_t key = EDEV_HOTPLUG_UEKEY_ACTION;

	printf("hotplug event:\n");

    printf("  --> %16s    = %s [%u]\n", 
			edev_hotplug_uevent_key_to_str(key), info->uevents[key], info->action);

	key++;
	for (; key < EDEV_HOTPLUG_UEKEY_MAX ; key++)
	{
		if (info->uevents[key])
		{
			printf("  --> %16s    = %s\n", 
					edev_hotplug_uevent_key_to_str(key), info->uevents[key]);
		}
	}
}

int main(void)
{
	int ret;
	edloop * loop = edloop_default();
	edev_hotplug * hp = edev_hotplug_new(loop, hotplug_handle);

	edev_hotplug_filter_action_set(hp, true, EDEV_HOTPLUG_ADD);
	edev_hotplug_filter_action_set(hp, true, EDEV_HOTPLUG_REMOVE);
	edev_hotplug_filter_uevent_set(hp, true, EDEV_HOTPLUG_UEKEY_SUBSYSTEM, "usb");

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

