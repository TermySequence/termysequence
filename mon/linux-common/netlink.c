// Copyright © 2018 TermySequence LLC
//
// SPDX-License-Identifier: GPL-2.0-only

#include "common.h"
#include "linux-common/mon-linux.h"
#include "lib/attr.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <unistd.h>

struct areq {
    struct nlmsghdr nlh;
    struct ifinfomsg ifm;
};

typedef void (*callback)(struct nlmsghdr *h, struct addr_table *tab);

static int
rescan_send(int nd, int type, int family)
{
    struct areq req = {
        .nlh.nlmsg_len = sizeof(req),
        .nlh.nlmsg_type = type,
        .nlh.nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST,
        .ifm.ifi_family = family
    };

    int rc = send(nd, &req, sizeof(req), 0);
    return (rc == -1) ? -errno : 0;
}

static void
route_handle(struct nlmsghdr *h, struct addr_table *tab)
{
    struct addr_rec rec;
    struct rtmsg *r = NLMSG_DATA(h);
    struct rtattr *a[RTA_MAX + 1];
    int len;

    if (r->rtm_type != RTN_UNICAST && r->rtm_type != RTN_LOCAL)
        return;

    len = h->nlmsg_len - NLMSG_LENGTH(sizeof(*r));
    memset(a, 0, sizeof(a));

    for (struct rtattr *p = RTM_RTA(r); RTA_OK(p, len); p = RTA_NEXT(p, len))
        if (p->rta_type <= RTA_MAX)
            a[p->rta_type] = p;

    if (a[RTA_OIF]) {
        rec.iface = *(uint32_t *)RTA_DATA(a[RTA_OIF]);
        rec.len = len = (r->rtm_family == AF_INET) ? 4 : 16;
        rec.prio = (r->rtm_scope << 8) | (a[RTA_DST] || a[RTA_SRC]);

        if (a[RTA_PREFSRC]) {
            rec.stem = len * 8;
            memcpy(rec.addr, RTA_DATA(a[RTA_PREFSRC]), len);
            addrUpdate(tab, &rec);
        }
        else if (a[RTA_DST]) {
            rec.stem = r->rtm_dst_len;
            memcpy(rec.addr, RTA_DATA(a[RTA_DST]), len);
            addrUpdate(tab, &rec);
        }
    }
}

static void
addr_handle(struct nlmsghdr *h, struct addr_table *tab)
{
    struct addr_rec rec;
    struct ifaddrmsg *r = NLMSG_DATA(h);
    struct rtattr *a[IFA_MAX + 1] = {};
    int len = h->nlmsg_len - NLMSG_LENGTH(sizeof(*r));

    for (struct rtattr *p = IFA_RTA(r); RTA_OK(p, len); p = RTA_NEXT(p, len))
        if (p->rta_type <= IFA_MAX)
            a[p->rta_type] = p;

    if (!a[IFA_ADDRESS])
        a[IFA_ADDRESS] = a[IFA_LOCAL];
    if (!a[IFA_ADDRESS])
        return;

    switch (r->ifa_family) {
    case AF_INET:
        len = 4;
        break;
    case AF_INET6:
        len = 16;
        break;
    default:
        return;
    }

    rec.iface = r->ifa_index;
    rec.len = len;
    rec.prio = ~0;
    rec.stem = r->ifa_prefixlen;
    memcpy(rec.addr, RTA_DATA(a[IFA_ADDRESS]), len);

    addrInsert(tab, &rec);
}

static int
rescan_recv(int nd, struct addr_table *tab, callback func)
{
    struct sockaddr_nl addr;
    char buf[32768];
    struct iovec iov = { buf };
    struct msghdr msg = {
        .msg_name = &addr,
        .msg_namelen = sizeof(addr),
        .msg_iov = &iov,
        .msg_iovlen = 1
    };
    struct nlmsghdr *h;
    int rc;

    while (1) {
        iov.iov_len = sizeof(buf);
        rc = recvmsg(nd, &msg, 0);
        if (rc == -1)
            return -errno;

        h = (struct nlmsghdr *)buf;
        for (; NLMSG_OK(h, rc); h = NLMSG_NEXT(h, rc))
            if (addr.nl_pid == 0)
                switch (h->nlmsg_type) {
                case NLMSG_DONE:
                case NLMSG_ERROR:
                    return 0;
                default:
                    (*func)(h, tab);
                }
    }
}

int
netlinkProps(int nd, struct addr_table *tab, struct mon_string *str)
{
    char buf[64];
    unsigned i;

    int rc = rescan_send(nd, RTM_GETADDR, AF_UNSPEC);
    if (rc < 0)
        return rc;
    rc = rescan_recv(nd, tab, &addr_handle);
    if (rc < 0)
        return rc;
    rc = rescan_send(nd, RTM_GETROUTE, AF_INET);
    if (rc < 0)
        return rc;
    rc = rescan_recv(nd, tab, &route_handle);
    if (rc < 0)
        return rc;
    rc = rescan_send(nd, RTM_GETROUTE, AF_INET6);
    if (rc < 0)
        return rc;
    rc = rescan_recv(nd, tab, &route_handle);
    if (rc < 0)
        return rc;

    addrAdjust(tab);
    qsort(tab->buf, tab->tabsize, sizeof(struct addr_rec), &addrCompare);

    // Set name to top address
    if (tab->tabsize > 0) {
        const struct addr_rec *rec = tab->buf;
        int af = (rec->len == 4) ? AF_INET : AF_INET6;
        inet_ntop(af, rec->addr, buf, sizeof(buf));
        stringInsert(str, TSQ_ATTR_NAME, buf);
    }

    // Find first ipv4 address
    for (i = 0; i < tab->tabsize; ++i) {
        const struct addr_rec *rec = tab->buf + i;
        if (rec->len == 4) {
            inet_ntop(AF_INET, rec->addr, buf, sizeof(buf));
            stringInsert(str, TSQ_ATTR_NET_IP4_ADDR, buf);
            goto next;
        }
    }
    stringRemove(str, TSQ_ATTR_NET_IP4_ADDR);
next:
    // Find first ipv6 address
    for (i = 0; i < tab->tabsize; ++i) {
        const struct addr_rec *rec = tab->buf + i;
        if (rec->len == 16) {
            inet_ntop(AF_INET6, rec->addr, buf, sizeof(buf));
            stringInsert(str, TSQ_ATTR_NET_IP6_ADDR, buf);
            goto out;
        }
    }
    stringRemove(str, TSQ_ATTR_NET_IP6_ADDR);
out:
    return 0;
}

int
netlinkFlush(int nd)
{
    while (1) {
        struct msghdr msg = {};
        switch (recvmsg(nd, &msg, MSG_TRUNC|MSG_DONTWAIT)) {
        case 0:
            errno = ECONNABORTED;
            // fallthru
        case -1:
            return (errno == EAGAIN || errno == EINTR) ? 0 : -errno;
        }
    }
}

int
netlinkOpen(int notifier)
{
    int nd, rc;
    struct sockaddr_nl addr = {
        .nl_family = AF_NETLINK,
        .nl_groups = notifier ? 0x551 : 0
    };

    nd = socket(AF_NETLINK, SOCK_RAW|SOCK_CLOEXEC, NETLINK_ROUTE);
    if (nd < 0)
        return -errno;

    rc = bind(nd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc < 0) {
        rc = -errno;
        close(nd);
        return rc;
    }

    return nd;
}
