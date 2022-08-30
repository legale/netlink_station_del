#include <stdio.h> /* printf */
#include <stdbool.h> /* bool, true, false macros */
#include <errno.h> /* printf */
#include <string.h>
#include <time.h>
#include <arpa/inet.h> /* inet_ntop() */
#include <unistd.h> /* close() */
#include <linux/nl80211.h> /* 802.11 netlink interface */
#include <linux/netlink.h> /*netlink macros and structures */
#include <net/if.h>

/* libnl-3 */
#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/msg.h>

/*libnl-gen-3*/
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>


/* used macros */
#define NL_OWN_PORT (1<<2)
#define ETH_ALEN 6


/* cli arguments parse macro and functions */
#define NEXT_ARG() do { argv++; if (--argc <= 0) incomplete_command(); } while(0)
#define NEXT_ARG_OK() (argc - 1 > 0)
#define PREV_ARG() do { argv--; argc++; } while(0)

static char *argv0; /* ptr to the program name string */
static void usage(void) {
    fprintf(stdout, ""
                    "Usage:   %s [command value] ... [command value]    \n"
                    "            command: dev | mac | help              \n"
                    "\n"
                    "Example: %s dev wlan0 mac 00:ff:12:a3:e3:b2        \n"
                    "\n", argv0, argv0);
    exit(-1);
}

struct nl_sock {
    struct sockaddr_nl s_local;
    struct sockaddr_nl s_peer;
    int s_fd;
    int s_proto;
    unsigned int s_seq_next;
    unsigned int s_seq_expect;
    int s_flags;
    struct nl_cb *s_cb;
    size_t s_bufsize;
};

struct nl80211_state {
    struct nl_sock *nl_sock;
    int nl80211_id;
} nl80211State = {
        .nl_sock = NULL,
        .nl80211_id = 0
};

/* convert hex mac address string FF:FF:FF:FF:FF:FF to 6 binary bytes */
static int mac_addr_atoi(uint8_t *mac, const char *hex) {
    if (strlen(hex) != sizeof("FF:FF:FF:FF:FF:FF") - 1) {
        printf("error: wrong len: %lu expected: %lu\n", strlen(hex), sizeof("FF:FF:FF:FF:FF:FF") - 1);
        return 0;
    }
    mac[0] = strtol(&hex[0], NULL, 16);
    mac[1] = strtol(&hex[3], NULL, 16);
    mac[2] = strtol(&hex[6], NULL, 16);
    mac[3] = strtol(&hex[9], NULL, 16);
    mac[4] = strtol(&hex[12], NULL, 16);
    mac[5] = strtol(&hex[15], NULL, 16);
    return 1;
}

static int nl_cb(struct nl_msg *msg, void *arg) {
    struct nlmsghdr *ret_hdr = nlmsg_hdr(msg);
    struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];

    if (ret_hdr->nlmsg_type != nl80211State.nl80211_id) return NL_STOP;

    struct genlmsghdr *gnlh = (struct genlmsghdr *) nlmsg_data(ret_hdr);

    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);


    /*
    *  <------- NLA_HDRLEN ------> <-- NLA_ALIGN(payload)-->
    * +---------------------+- - -+- - - - - - - - - -+- - -+
    * |        Header       | Pad |     Payload       | Pad |
    * |   (struct nlattr)   | ing |                   | ing |
    * +---------------------+- - -+- - - - - - - - - -+- - -+
    *  <-------------- nlattr->nla_len -------------->
    */
    for (int i = 0; i < NL80211_ATTR_MAX; i++) {
        if (tb_msg[i] == NULL) continue;
        fprintf(stderr, "nla_type: %d nla_len: %d\n", tb_msg[i]->nla_type, tb_msg[i]->nla_len);
    }
    return 0;
}

/* Returns true if 'prefix' is a not empty prefix of 'string'. */
static bool matches(const char *prefix, const char *string) {
    if (!*prefix)
        return false;
    while (*string && *prefix == *string) {
        prefix++;
        string++;
    }
    return !*prefix;
}

static void incomplete_command(void) {
    fprintf(stderr, "Command line is not complete. Try option \"help\"\n");
    exit(-1);
}

static int nl80211_cmd_del_station(const char *dev, const char *mac) {
    int ret; /* to store returning values */
    /* nl_socket_alloc(), genl_connect() replacement */
    struct nl_sock sk = {
            .s_fd = -1,
            .s_cb = nl_cb_alloc(NL_CB_DEFAULT), /* callback */
            .s_local.nl_family = AF_NETLINK,
            .s_peer.nl_family = AF_NETLINK,
            .s_seq_expect = time(NULL),
            .s_seq_next = time(NULL),

            /* the port is 0 (unspecified), meaning NL_OWN_PORT */
            .s_flags = NL_OWN_PORT,
    };

    sk.s_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC);
    nl80211State.nl_sock = &sk;


    //find the nl80211 driver ID
    nl80211State.nl80211_id = genl_ctrl_resolve(&sk, "nl80211");

    //attach a callback
    nl_socket_modify_cb(&sk, NL_CB_VALID, NL_CB_CUSTOM, nl_cb, NULL);


    //allocate a message
    struct nl_msg *msg = nlmsg_alloc();


    int if_index = if_nametoindex(dev);
    if (if_index == 0) if_index = -1;


    uint8_t mac_addr[ETH_ALEN];
    if (!mac_addr_atoi((uint8_t *) &mac_addr, mac)) {
        fprintf(stderr, "invalid mac address\n");
        return 2;
    }

    enum nl80211_commands cmd = NL80211_CMD_DEL_STATION;
    int flags = 0;

    // setup the message
    genlmsg_put(msg, 0, 0, nl80211State.nl80211_id, 0, flags, cmd, 0);

    //add message attributes
    NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, if_index);

    NLA_PUT(msg, NL80211_ATTR_MAC, ETH_ALEN, mac_addr);

    //send the message (this frees it)
    ret = nl_send_auto_complete(&sk, msg);
    if (ret < 0) {
        /* -ret bacause nl commands returns negative error code if false */
        fprintf(stderr, "nl_send_auto_complete: %d %s\n", ret, strerror(-ret));
        return (ret);
    }

    //block for message to return
    ret = nl_recvmsgs_default(&sk);

    if (ret < 0) {
        /* -ret bacause nl commands returns negative error code if false */
        fprintf(stderr, "nl_send_auto_complete: %d %s\n", ret, strerror(-ret));
    }

    return (ret);

    nla_put_failure: /* this tag is used in NLA_PUT macros */
    nlmsg_free(msg);
    return -ENOBUFS;
}


int main(int argc, char **argv) {
    int ret;
    char *dev = NULL, *mac = NULL;
    /* cli arguments parse */
    argv0 = *argv; /* first arg is program name */
    while (argc > 1) {
        NEXT_ARG();
        if (matches(*argv, "dev")) {
            NEXT_ARG();
            dev = *argv; /* device interface name e.g. wlan0 */
        } else if (matches(*argv, "mac")) {
            NEXT_ARG();
            mac = *argv; /* mac address e.g. aa:bb:cc:dd:ee:ff */
        } else if (matches(*argv, "help")) {
            usage();
        } else {
            usage();
        }
    }

    if (dev == NULL || mac == NULL) {
        incomplete_command();
    }
    ret = nl80211_cmd_del_station(dev, mac);
    return -ret;
}

