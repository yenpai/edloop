#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include "edev_usbdev.h"

struct edev_usbdev {
	edev_ioevent       ioevent;
	edev_usbdev_cb     notify;
	int                sock;
	struct sockaddr_nl snl;
};

static const char * kobject_actions[] = {
	[EDEV_UEVENT_ACTION_ADD]     = "add",
	[EDEV_UEVENT_ACTION_REMOVE]  = "remove",
	[EDEV_UEVENT_ACTION_CHANGE]  = "change",
	[EDEV_UEVENT_ACTION_MOVE]    = "move",
	[EDEV_UEVENT_ACTION_ONLINE]  = "online",
	[EDEV_UEVENT_ACTION_OFFLINE] = "offline",
};

static const char * netlink_message_parse(const char * buf, size_t len, const char * key)
{
	size_t keylen = strlen(key);
	size_t offset;

	for (offset = 0 ; offset < len ; offset += strlen(buf + offset) + 1)
	{
		if (buf[offset] == '\0')
			break;

		if (strncmp(buf + offset, key, keylen) == 0)
		{
			if (buf[offset + keylen] == '=')
			{
				return buf + offset + keylen + 1;
			}
		}
	}

	return NULL;
}

static int hotplug_netlink_parse_usb(char * buf, ssize_t len, edev_usbdev_info * info)
{
	const char * tmp;
	char * ptr;

	/* set busnum if have BUSNUM */
	if ((tmp = netlink_message_parse(buf, len, "BUSNUM")) != NULL)
		info->busnum = (uint8_t) (0xFF & strtoul(tmp, NULL, 10)); 

	/* set devnum if have DEVNUM */
	if ((tmp = netlink_message_parse(buf, len, "DEVNUM")) != NULL)
		info->devnum = (uint8_t) (0xFF & strtoul(tmp, NULL, 10));
	
	/* check busnum and devnum */
	if (info->busnum == 0 || info->devnum == 0)
	{
		/* set that if have DEVICE */
		if ((tmp = netlink_message_parse(buf, len, "DEVICE")) != NULL)
		{
			/* Parse a device path such as /dev/bus/usb/003/004 */
			if ((ptr = (char *) strrchr(tmp,'/')) != NULL)
			{
				info->busnum = (uint8_t)(0xFF & strtoul(ptr - 3, NULL, 10));
				info->devnum = (uint8_t)(0xFF & strtoul(ptr + 1, NULL, 10));
			}
		}
		
		if (info->busnum == 0 || info->devnum == 0)
			return -1;
	}

	/* set idVendor and idProduct if have PRODUCT */
	if ((tmp = netlink_message_parse(buf, len, "PRODUCT")) != NULL)
	{
		/* Parse a usb_ids such as 1307/163/100 */
		if ((ptr = (char *) strchr(tmp,'/')) != NULL)
		{
			info->idVendor  = (uint16_t) (0xFFFF & strtoul(tmp, NULL, 16));
			info->idProduct = (uint16_t) (0xFFFF & strtoul(ptr + 1, NULL, 16));
		}
	}

	/* check idVendor and idProduct */
	if (info->idVendor == 0 || info->idProduct == 0)
		return -2;

	return 0;
}

static int hotplug_netlink_parse(char * buf, ssize_t len, edev_usbdev_info * info)
{
	const char * tmp;
	int i;

	/* check that have action type (default keys in kobject_uevent) */
	if ((tmp = netlink_message_parse(buf, len, "ACTION")) == NULL)
		return -1;

	for (i = 0 ; i < EDEV_UEVENT_ACTION_MAX ; i++)
	{
		if (strcmp(tmp, kobject_actions[i]) == 0)
		{
			info->action = i;
			break;
		}
	}

	if (i == EDEV_UEVENT_ACTION_MAX)
		return -2;

	/* check that have device path (default keys in kobject_uevent) */
	if ((tmp = netlink_message_parse(buf, len, "DEVPATH")) == NULL)
		return -3;
	info->devpath = tmp;

	/* check that have subsystem (default keys in kobject_uevent ) */
	if ((tmp = netlink_message_parse(buf, len, "SUBSYSTEM")) == NULL)
		return -4;
	info->subsystem = tmp;

	return 0;
}

static int hotplug_netlink_read(edev_usbdev * hp)
{
	char buf[1024];
	struct iovec  iov = { .iov_base = buf, .iov_len = sizeof(buf)};
	struct msghdr meh = { .msg_iov = &iov, .msg_iovlen = 1, .msg_name = &hp->snl, .msg_namelen = sizeof(hp->snl) };
	edev_usbdev_info  info;
	ssize_t len;
	int ret;

	memset(buf, 0, sizeof(buf));
	while ((len = recvmsg(hp->sock, &meh, 0)) < 0)
	{
		if (errno == EAGAIN) 
			continue;
		return -1;
	}

	/* Invalid message */
	if (len < 32)
		return -2; 

	memset(&info, 0, sizeof(edev_usbdev_info));

	/* Parsing default keys in message */
	if ((ret = hotplug_netlink_parse(buf, len, &info)) < 0)
	{
		//printf("hotplug_netlink_parse ret[%d]\n", ret);
		return -3;
	}

	/* Parsing usb keys in message */
	if (strcmp(info.subsystem, "usb") == 0)
	{
		if ((ret = hotplug_netlink_parse_usb(buf, len, &info)) < 0)
		{
			//printf("hotplug_netlink_parse_usb ret[%d]\n", ret);
			return -4;
		}
	}
	else
	{
		return -99;
	}

	/* Notification */
	if (hp->notify)
		hp->notify(hp, &info);

	return 0;
}

static int hotplug_netlink_socket(edev_usbdev * hp)
{
	int sock = -1;

	sock = socket(PF_NETLINK, SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, NETLINK_KOBJECT_UEVENT);
	if (sock == -1 && errno == EINVAL)
		sock = socket(PF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);
	if (sock == -1)
		return -1;

	memset(&hp->snl, 0, sizeof(hp->snl));
	hp->snl.nl_family = AF_NETLINK;
	hp->snl.nl_groups = 1; // Kernel

	if (bind(sock, (struct sockaddr *) &hp->snl, sizeof(hp->snl)) != 0)
	{
		close(sock);
		return -2;
	}

	hp->sock = sock;
	return 0;
}

static void usbdev_ioevent_handle(edev_ioevent * io, int UNUSED(fd), unsigned int revents)
{
	edev_usbdev * hp = container_of(io, edev_usbdev, ioevent);

	if (revents & EDIO_READ)
		hotplug_netlink_read(hp);
}

int edev_usbdev_attach(edev_usbdev * hp)
{
	int ret = 0;

	if (hp->sock < 0 && hotplug_netlink_socket(hp) < 0)
		return -1;

	if (edev_ioevent_attach(&hp->ioevent, hp->sock, EDIO_READ | EDIO_NONBLOCK | EDIO_CLOEXEC | EDIO_CLOAUTO) < 0)
	{
		close(hp->sock);
		hp->sock = -1;
		return -2;
	}

	return ret;
}

void edev_usbdev_detach(edev_usbdev * hp)
{
	edev_ioevent_detach(&hp->ioevent);
}

edev_usbdev * edev_usbdev_new(edloop * loop, edev_usbdev_cb notify)
{
	edev_usbdev * hp;

	if ((hp = malloc(sizeof(*hp))) == NULL)
		return NULL;

	memset(hp, 0, sizeof(*hp));
	edev_ioevent_base_init(&hp->ioevent, loop, usbdev_ioevent_handle);

	hp->sock   = -1;
	hp->notify = notify;

	return hp;
}
