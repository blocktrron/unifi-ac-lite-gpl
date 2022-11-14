/* ebt_set
 *
 * Authors:
 * Keh-Ming Luoh <kmluoh@ubnt.com>
 *
 * November, 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/types.h>
#include <linux/netfilter/ipset/ip_set.h>
#include "../include/ebtables_u.h"

struct ebt_set_info {
	ip_set_id_t index;
	__u8 dim;
	__u8 flags;
	__u8 family;
};

static int
get_version(unsigned *version)
{
	int res, sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	struct ip_set_req_version req_version;
	socklen_t size = sizeof(req_version);

	if (sockfd < 0)
		ebt_print_error("Can't open socket to ipset.\n");

	if (fcntl(sockfd, F_SETFD, FD_CLOEXEC) == -1) {
		ebt_print_error("Could not set close on exec: %s\n",
			      strerror(errno));
	}

	req_version.op = IP_SET_OP_VERSION;
	res = getsockopt(sockfd, SOL_IP, SO_IP_SET, &req_version, &size);
	if (res != 0)
		ebt_print_error("Kernel module ip_set is not loaded in.\n");

	*version = req_version.version;

	return sockfd;
}

	static void
get_set_byid(char *setname, ip_set_id_t idx)
{
	struct ip_set_req_get_set req;
	socklen_t size = sizeof(struct ip_set_req_get_set);
	int res, sockfd;

	sockfd = get_version(&req.version);
	req.op = IP_SET_OP_GET_BYINDEX;
	req.set.index = idx;
	res = getsockopt(sockfd, SOL_IP, SO_IP_SET, &req, &size);
	close(sockfd);

	if (res != 0)
		ebt_print_error(
			"Problem when communicating with ipset, errno=%d.\n",
			errno);
	if (size != sizeof(struct ip_set_req_get_set))
		ebt_print_error(
			"Incorrect return size from kernel during ipset lookup, "
			"(want %zu, got %zu)\n",
			sizeof(struct ip_set_req_get_set), (size_t)size);
	if (req.set.name[0] == '\0')
		ebt_print_error(
			"Set with index %i in kernel doesn't exist.\n", idx);

	strncpy(setname, req.set.name, IPSET_MAXNAMELEN);
}

static void
get_set_byname_only(const char *setname, struct ebt_set_info *info,
		    int sockfd, unsigned int version)
{
	struct ip_set_req_get_set req = { .version = version };
	socklen_t size = sizeof(struct ip_set_req_get_set);
	int res;

	req.op = IP_SET_OP_GET_BYNAME;
	strncpy(req.set.name, setname, IPSET_MAXNAMELEN);
	req.set.name[IPSET_MAXNAMELEN - 1] = '\0';
	res = getsockopt(sockfd, SOL_IP, SO_IP_SET, &req, &size);
	close(sockfd);

	if (res != 0)
		ebt_print_error(
			"Problem when communicating with ipset, errno=%d.\n",
			errno);
	if (size != sizeof(struct ip_set_req_get_set))
		ebt_print_error(
			"Incorrect return size from kernel during ipset lookup, "
			"(want %zu, got %zu)\n",
			sizeof(struct ip_set_req_get_set), (size_t)size);
	if (req.set.index == IPSET_INVALID_ID)
		ebt_print_error("Set %s doesn't exist.\n", setname);

	info->index = req.set.index;
}

static void
get_set_byname(const char *setname, struct ebt_set_info *info)
{
	struct ip_set_req_get_set_family req;
	socklen_t size = sizeof(struct ip_set_req_get_set_family);
	int res, sockfd, version;

	sockfd = get_version(&req.version);
	version = req.version;
	req.op = IP_SET_OP_GET_FNAME;
	strncpy(req.set.name, setname, IPSET_MAXNAMELEN);
	req.set.name[IPSET_MAXNAMELEN - 1] = '\0';
	res = getsockopt(sockfd, SOL_IP, SO_IP_SET, &req, &size);

	if (res != 0 && errno == EBADMSG)
		/* Backward compatibility */
		return get_set_byname_only(setname, info, sockfd, version);

	close(sockfd);
	if (res != 0)
		ebt_print_error(
			"Problem when communicating with ipset, errno=%d.\n",
			errno);
	if (size != sizeof(struct ip_set_req_get_set_family))
		ebt_print_error(
			"Incorrect return size from kernel during ipset lookup, "
			"(want %zu, got %zu)\n",
			sizeof(struct ip_set_req_get_set_family),
			(size_t)size);
	if (req.set.index == IPSET_INVALID_ID)
		ebt_print_error("Set %s doesn't exist.\n", setname);
	if (!(req.family == NFPROTO_IPV4 ||
	      req.family == NFPROTO_IPV6 ||
	      req.family == NFPROTO_UNSPEC))
		ebt_print_error(
			      "The protocol family of set %s is %s, "
			      "which is not applicable.\n",
			      setname,
			      req.family == NFPROTO_IPV4 ? "IPv4" : "IPv6");

	info->index = req.set.index;
}

static void
parse_dirs(const char *opt_arg, struct ebt_set_info *info)
{
	char *saved = strdup(opt_arg);
	char *ptr, *tmp = saved;

	while (info->dim < IPSET_DIM_MAX && tmp != NULL) {
		info->dim++;
		ptr = strsep(&tmp, ",");
		if (strncmp(ptr, "src", 3) == 0)
			info->flags |= (1 << info->dim);
		else if (strncmp(ptr, "dst", 3) != 0)
			ebt_print_error(
				"You must spefify (the comma separated list of) 'src' or 'dst'.");
	}

	if (tmp)
		ebt_print_error(
			      "Can't be more src/dst options than %i.",
			      IPSET_DIM_MAX);

	free(saved);
}

static struct option opts[] =
{
	{ "set"		, required_argument, 0, '1' },
	{ "set-flags"	, required_argument, 0, '2' },
	{ "set-family"	, required_argument, 0, '3' },
	{ 0 }
};

static void print_help()
{
	printf("set match options:\n"
	       " --set [!] name\n"
	       " --set-flags flags\n"
	       " --set-family { inet | inet6 }\n"
	       "		 'name' is the set name from to match,\n"
	       "		 'flags' are the comma separated list of\n"
	       "		 'src' and 'dst' specifications.\n");
}

static void init(struct ebt_entry_match *match)
{
	struct ebt_set_info *setinfo = (struct ebt_set_info *)match->data;

	setinfo->index = IPSET_INVALID_ID;
	setinfo->dim = 0;
	setinfo->flags = 0;
	setinfo->family = NFPROTO_IPV4;
}

#define OPT_SET		0x01
#define OPT_FLAGS	0x02
#define OPT_FAMILY	0x04
static int parse(int c, char **argv, int argc, const struct ebt_u_entry *entry,
   unsigned int *flags, struct ebt_entry_match **match)
{
	struct ebt_set_info *setinfo = (struct ebt_set_info *)(*match)->data;
	char *end;
	long int i;

	switch (c) {
	case '1':
		ebt_check_option2(flags, OPT_SET);

		if (ebt_check_inverse2(optarg)) {
			setinfo->flags |= IPSET_INV_MATCH;
		}

		if (strlen(optarg) > IPSET_MAXNAMELEN - 1)
			ebt_print_error(
				      "setname `%s' too long, max %d characters.",
				      optarg, IPSET_MAXNAMELEN - 1);

		get_set_byname(optarg, setinfo);
		break;

	case '2':
		ebt_check_option2(flags, OPT_FLAGS);

		parse_dirs(optarg, setinfo);
		break;

	case '3':
		ebt_check_option2(flags, OPT_FAMILY);

		if (!strcmp(optarg, "inet6")) {
			setinfo->family = NFPROTO_IPV6;
		} else if (!strcmp(optarg, "inet")) {
			setinfo->family = NFPROTO_IPV4;
		} else {
			ebt_print_error(
				      "ipset family `%s' is not supported.",
				      optarg);
		}
		break;

	default:
		return 0;
	}

	return 1;
}

static void final_check(const struct ebt_u_entry *entry,
   const struct ebt_entry_match *match, const char *name,
   unsigned int hookmask, unsigned int time)
{
	struct ebt_set_info *setinfo = (struct ebt_set_info *)match->data;

	if (setinfo->index == IPSET_INVALID_ID)
		ebt_print_error(
			"Invalid set name: You must specify `--set' with proper arguments");
	if (!setinfo->dim)
		ebt_print_error(
			"Invalid set flags: You must specify `--set-flags' with proper arguments");
}

#define family_name(f)  ((f) == NFPROTO_IPV4 ? "inet" : \
                         (f) == NFPROTO_IPV6 ? "inet6" : "any")

static void print(const struct ebt_u_entry *entry,
   const struct ebt_entry_match *match)
{
	struct ebt_set_info *setinfo = (struct ebt_set_info *)match->data;
	int i;
	char setname[IPSET_MAXNAMELEN];

	get_set_byid(setname, setinfo->index);
	printf("--set%s %s --set-flags",
	       (setinfo->flags & IPSET_INV_MATCH) ? " !" : "",
	       setname);
	for (i = 1; i <= setinfo->dim; i++) {
		printf("%s%s",
		       i == 1 ? " " : ",",
		       setinfo->flags & (1 << i) ? "src" : "dst");
	}
	printf(" --set-family %s ", family_name(setinfo->family));
}

static int compare(const struct ebt_entry_match *m1,
   const struct ebt_entry_match *m2)
{
	struct ebt_set_info *setinfo1 = (struct ebt_set_info *)m1->data;
	struct ebt_set_info *setinfo2 = (struct ebt_set_info *)m2->data;

	if (setinfo1->index != setinfo2->index)
		return 0;
	if (setinfo1->dim != setinfo2->dim)
		return 0;
	if (setinfo1->flags != setinfo2->flags)
		return 0;
	if (setinfo1->family != setinfo2->family)
		return 0;

	return 1;
}

static struct ebt_u_match set_match =
{
	.name		= "set",
	.size		= sizeof(struct ebt_set_info),
	.help		= print_help,
	.init		= init,
	.parse		= parse,
	.final_check	= final_check,
	.print		= print,
	.compare	= compare,
	.extra_ops	= opts,
};

__attribute__((constructor)) static void extension_init(void)
{
	ebt_register_match(&set_match);
}
