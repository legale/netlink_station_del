#include <stdio.h> /* printf() */
#include <stdlib.h> /* exit() */
#include <stdbool.h> /* bool, true, false macros */
#include <unistd.h> /* close() */


/* to replace include socket.h possible without struct ucred define it here
#include <sys/socket.h>  //to define struct ucred which is used in libnl-tiny headers
*/
#ifndef _struct_ucred
#define _struct_ucred
struct ucred {
    pid_t pid;
    uid_t uid;
    gid_t gid;
};
#endif /* _struct_ucred */

#include <errno.h> /* printf */
#include <string.h>
#include <time.h>
#include <arpa/inet.h> /* inet_ntop() */
#include <linux/nl80211.h> /* 802.11 netlink interface */
#include <linux/netlink.h> /*netlink macros and structures */
#include <net/if.h>


/* libnl-3 libnl-tiny */
#include <netlink/netlink.h>
#include <netlink/attr.h>
#include <netlink/msg.h>

/*libnl-gen-3 libnl-tiny */
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include "station_del.h"


/* used macros */
#define NL_OWN_PORT (1<<2)
#define ETH_ALEN 6



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
#if UINTPTR_MAX == UINT64_MAX
        printf("error: wrong len: %lu expected: %lu\n", strlen(hex), sizeof("FF:FF:FF:FF:FF:FF") - 1);
#else
        printf("error: wrong len: %u expected: %u\n", strlen(hex), sizeof("FF:FF:FF:FF:FF:FF") - 1);
#endif
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

int nl80211_cmd_del_station(const char *dev, const char *mac) {
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

