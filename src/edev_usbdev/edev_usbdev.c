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

static int hotplug_netlink_parse(char * buf, ssize_t len, edev_usbdev_info * info)
{
	const char * tmp;
	char * pLastSlash;
	int i;

	memset(info, 0, sizeof(edev_usbdev_info));

	/* check that this is add or remove */
	if ((tmp = netlink_message_parse(buf, len, "ACTION")) == NULL)
		return -1;
	else if (strcmp(tmp, "remove") == 0)
		info->detached = 1;
	else if (strcmp(tmp, "add") == 0)
		info->detached = 0;
	else
		return -2;

	/* check that this is usb message */
	if ((tmp = netlink_message_parse(buf, len, "SUBSYSTEM")) == NULL)
		return -3;
	else if (strcmp(tmp, "usb") != 0)
		return -4;

	/* check that have bus number or device */
	if ((tmp = netlink_message_parse(buf, len, "BUSNUM")) == NULL)
	{
		if ((tmp = netlink_message_parse(buf, len, "DEVICE")) == NULL)
			return -5;

		/* Parse a device path such as /dev/bus/usb/003/004 */
		if ((pLastSlash = (char *) strrchr(tmp,'/')) == NULL)
			return -6;

		info->devaddr = (uint8_t)(0xFF & strtoul(pLastSlash + 1, NULL, 10));
		info->busnum  = (uint8_t)(0xFF & strtoul(pLastSlash - 3, NULL, 10));
		return 0;
	}

	info->busnum = (uint8_t) (0xFF & strtoul(tmp, NULL, 10)); 

	/* check that have device number */
	if ((tmp = netlink_message_parse(buf, len, "DEVNUM")) == NULL)
		return -7;

	info->devaddr = (uint8_t) (0xFF & strtoul(tmp, NULL, 10));

	/* check that have device path */
	if ((tmp = netlink_message_parse(buf, len, "DEVPATH")) == NULL)
		return -8;

	for (i = strlen(tmp) - 1 ; i > 0 ; --i)
	{
		if (tmp[i] == '/')
		{
			info->sysname = tmp + i + 1;
			break;
		}
	}

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

	/* Parsing message */
	if ((ret = hotplug_netlink_parse(buf, len, &info)) < 0)
		return -3;

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

	return sock;
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
