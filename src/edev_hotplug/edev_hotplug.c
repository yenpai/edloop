#include <errno.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include "edev_hotplug.h"

typedef struct {
	struct list_head node;
	uint8_t key;
	char    value[128];
} uevent_rule;

struct edev_hotplug {
	edev_ioevent       ioevent;
	edev_hotplug_cb    notify;
	int                sock;
	struct sockaddr_nl snl;
	struct list_head   rules;
};

static const char * uevent_keys[] = {
	[EDEV_HOTPLUG_UEKEY_ACTION]    = "ACTION",
    [EDEV_HOTPLUG_UEKEY_DEVPATH]   = "DEVPATH",
    [EDEV_HOTPLUG_UEKEY_SUBSYSTEM] = "SUBSYSTEM",
    [EDEV_HOTPLUG_UEKEY_DRIVER]    = "DRIVER",
    [EDEV_HOTPLUG_UEKEY_DEVICE]    = "DEVICE",
    [EDEV_HOTPLUG_UEKEY_PRODUCT]   = "PRODUCT",

	[EDEV_HOTPLUG_UEKEY_MAJOR]     = "MAJOR",
    [EDEV_HOTPLUG_UEKEY_MINOR]     = "MINOR",
    [EDEV_HOTPLUG_UEKEY_DEVNAME]   = "DEVNAME",
    [EDEV_HOTPLUG_UEKEY_DEVTYPE]   = "DEVTYPE",
    [EDEV_HOTPLUG_UEKEY_BUSNUM]    = "BUSNUM",
    [EDEV_HOTPLUG_UEKEY_DEVNUM]    = "DEVNUM",
};

static const char * kobject_actions[] = {
	[EDEV_HOTPLUG_ADD]     = "add",
	[EDEV_HOTPLUG_REMOVE]  = "remove",
	[EDEV_HOTPLUG_CHANGE]  = "change",
	[EDEV_HOTPLUG_MOVE]    = "move",
	[EDEV_HOTPLUG_ONLINE]  = "online",
	[EDEV_HOTPLUG_OFFLINE] = "offline",
};

static uevent_rule * hotplug_rule_find(struct list_head * list, uint8_t key, char * value)
{
	uevent_rule * rule;

	list_for_each_entry(rule, list, node)
	{
		if (key == rule->key && strcmp(value, rule->value) == 0)
			return rule;
	}

	return NULL;
}

static void hotplug_rule_del(struct list_head * list, uint8_t key, char * value)
{
	uevent_rule * rule;

	if ((rule = hotplug_rule_find(list, key, value)) != NULL)
	{
		list_del_init(&rule->node);
		free(rule);
	}
}

static int hotplug_rule_add(struct list_head * list, uint8_t key, char * value)
{
	struct list_head * head = list;
	uevent_rule * rule, * tmp;

	if ((rule = hotplug_rule_find(list, key, value)) != NULL)
		return 0;

	if ((rule = malloc(sizeof(uevent_rule))) == NULL)
		return -2;

	memset(rule, 0, sizeof(uevent_rule));
	INIT_LIST_HEAD(&rule->node);
	rule->key = key;
	strncpy(rule->value, value, sizeof(rule->value) - 1);

	list_for_each_entry(tmp, list, node)
	{
		if (tmp->key > rule->key)
		{
			head = &tmp->node;
			break;
		}
	}

	list_add_tail(&rule->node, head);
	return 0;
}

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

static int hotplug_netlink_parse(char * buf, ssize_t len, edev_hotplug_info * info)
{
	uint8_t key;
	uint8_t action;

	memset(info, 0, sizeof(edev_hotplug_info));

	for (key = EDEV_HOTPLUG_DEFAULT_UEKEY_MIN ; key <= EDEV_HOTPLUG_DEFAULT_UEKEY_MAX ; key++)
		info->uevents[key] = netlink_message_parse(buf, len, uevent_keys[key]);

	/* check that have default keys from kernel uevent */
	if (info->uevents[EDEV_HOTPLUG_UEKEY_ACTION] == NULL)
		return -1;
	if (info->uevents[EDEV_HOTPLUG_UEKEY_DEVPATH] == NULL)
		return -2;
	if (info->uevents[EDEV_HOTPLUG_UEKEY_SUBSYSTEM] == NULL)
		return -3;

	/* check and parsing action */
	for (action = 0 ; action < EDEV_HOTPLUG_ACTION_MAX ; action++)
	{
		if (strcmp(info->uevents[EDEV_HOTPLUG_UEKEY_ACTION], kobject_actions[action]) == 0)
		{
			info->action = action;
			break;
		}
	}

	if (action == EDEV_HOTPLUG_ACTION_MAX)
		return -4;

	/* Get more uevent value for usb */
	if (strcmp(info->uevents[EDEV_HOTPLUG_UEKEY_SUBSYSTEM], "usb") == 0)
	{
		for (key = EDEV_HOTPLUG_USB_UEKEY_MIN ; key <= EDEV_HOTPLUG_USB_UEKEY_MAX ; key++)
			info->uevents[key] = netlink_message_parse(buf, len, uevent_keys[key]);
	}

	return 0;
}

static bool hotplug_netlink_check_rules(edev_hotplug * hp, edev_hotplug_info * info)
{
	struct list_head * list = &hp->rules;
	uevent_rule * rule;
	bool    match;
	uint8_t key;

	if (list_empty(list))
		return true;

	rule  = list_first_entry(list, uevent_rule, node);
	key   = rule->key;
	match = false;

	list_for_each_entry(rule, list, node)
	{
		if (key != rule->key)
		{
			if (match == false)
				break;

			key   = rule->key;
			match = false;
		}
		
		if (match == true)
			continue;

		if (info->uevents[key] == NULL)
			break;

		if (strcmp(info->uevents[key], rule->value) == 0)
			match = true;
	}

	return match;
}

static int hotplug_netlink_read(edev_hotplug * hp)
{
	char buf[1024];
	struct iovec  iov = { .iov_base = buf, .iov_len = sizeof(buf)};
	struct msghdr meh = { .msg_iov = &iov, .msg_iovlen = 1, .msg_name = &hp->snl, .msg_namelen = sizeof(hp->snl) };
	edev_hotplug_info  info;
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

	/* Parsing default keys in message */
	if ((ret = hotplug_netlink_parse(buf, len, &info)) < 0)
		return -3;

	/* Check rules filter */
	if (hotplug_netlink_check_rules(hp, &info) == false)
		return -4;

	/* Notification */
	if (hp->notify)
		hp->notify(hp, &info);

	return 0;
}

static int hotplug_netlink_socket(edev_hotplug * hp)
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

static void hotplug_ioevent_handle(edev_ioevent * io, int UNUSED(fd), unsigned int revents)
{
	edev_hotplug * hp = container_of(io, edev_hotplug, ioevent);

	if (revents & EDIO_READ)
		hotplug_netlink_read(hp);
}

const char * edev_hotplug_uevent_key_to_str(edev_hotplug_uevent_key_e key)
{
	return (key < EDEV_HOTPLUG_UEKEY_MAX) ? uevent_keys[key] : NULL ;
}

int edev_hotplug_filter_uevent_set(edev_hotplug * hp, bool enable, uint8_t key, char * value)
{
	if (key >= EDEV_HOTPLUG_UEKEY_MAX)
		return -1;

	if (value == NULL)
		return -1;

	if (enable)
		return hotplug_rule_add(&hp->rules, key, value);
	
	hotplug_rule_del(&hp->rules, key, value);
	return 0;
}

int edev_hotplug_filter_action_set(edev_hotplug * hp, bool enable, uint8_t action)
{
	if (action >= EDEV_HOTPLUG_ACTION_MAX)
		return -1;
	
	return edev_hotplug_filter_uevent_set(hp, enable, EDEV_HOTPLUG_UEKEY_ACTION, (char *) kobject_actions[action]);
}

int edev_hotplug_attach(edev_hotplug * hp)
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

void edev_hotplug_detach(edev_hotplug * hp)
{
	edev_ioevent_detach(&hp->ioevent);
}

void edev_hotplug_init(edev_hotplug * hp, edloop * loop, edev_hotplug_cb notify)
{
	edev_ioevent_init(&hp->ioevent, loop, hotplug_ioevent_handle);

	INIT_LIST_HEAD(&hp->rules);
	hp->notify = notify;
	hp->sock   = -1;
}

edev_hotplug * edev_hotplug_new(edloop * loop, edev_hotplug_cb notify)
{
	edev_hotplug * hp;

	if (loop == NULL)
		return NULL;

	if ((hp = malloc(sizeof(*hp))) == NULL)
		return NULL;

	memset(hp, 0, sizeof(*hp));
	edev_hotplug_init(hp, loop, notify);

	return hp;
}
