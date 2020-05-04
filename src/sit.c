#include <netlink/route/link/sit.h>
#include <netlink/route/addr.h>
#include <netlink/route/route.h>
#include <netlink/version.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include "sit.h"
#include "log.h"
#include "types.h"

#if LIBNL_VER_NUM < LIBNL_VER(3, 5)
extern int rtnl_link_is_sit(struct rtnl_link *link);
#endif

int sit_configure(struct nl_sock *sk, const sit_tunnel_t *tunnel, const sit_route_t *route) {
    struct nl_cache *cache = NULL;
    struct rtnl_link *sit_link;
    in_addr_t laddr, raddr;
    struct nl_addr* local_addr = NULL;
    struct rtnl_addr* rtnl_addr = NULL;
    const sit_route_t *route_ptr = route;
    int err, ifindex;

    /* create sit tunnel */

    err = inet_pton(AF_INET, tunnel->local, &laddr);
    if (err != 1) {
        err = 1;
        log_fatal("inet_pton(): bad local address.\n");
        goto end;
    }

    err = inet_pton(AF_INET, tunnel->remote, &raddr);
    if (err != 1) {
        err = 1;
        log_fatal("inet_pton(): bad remote address.\n");
        goto end;
    }

    err = rtnl_link_alloc_cache(sk, AF_UNSPEC, &cache);
    if (err < 0) {
        log_fatal("rtnl_link_alloc_cache(): %s.\n", nl_geterror(err));
        goto end;
    }

    sit_link = rtnl_link_sit_alloc();
    if (sit_link == NULL) {
        err = 1;
        log_fatal("rtnl_link_sit_alloc(): can't allocate.\n");
        goto end;
    }

    rtnl_link_set_name(sit_link, tunnel->name);
    rtnl_link_sit_set_local(sit_link, laddr);
    rtnl_link_sit_set_remote(sit_link, raddr);
    rtnl_link_sit_set_ttl(sit_link, 255);
    rtnl_link_set_flags(sit_link, IFF_UP);

    if (tunnel->mtu != 0) {
        rtnl_link_set_mtu(sit_link, tunnel->mtu);
    }

    err = rtnl_link_add(sk, sit_link, NLM_F_CREATE);
    if (err < 0) {
        log_fatal("rtnl_link_add(): %s.\n", nl_geterror(err));
        goto end;
    }

    rtnl_link_put(sit_link);

    /* configure tunnel address */

    err = nl_addr_parse(tunnel->address, AF_INET6, &local_addr);
    if (err < 0) {
        err = 1;
        log_fatal("nl_addr_parse(): %s.\n", nl_geterror(err));
        goto end;
    }

    sit_link = NULL;
    err = sit_get(sk, tunnel->name, &sit_link);
    if (err < 0 || sit_link == NULL) {
        err = 1;
        log_fatal("sit_get(): can't get interface.\n");
        goto end;
    }

    ifindex = rtnl_link_get_ifindex(sit_link);
    if (ifindex == 0) {
        log_fatal("rtnl_link_get_ifindex(): can't get interface index.\n");
        err = 1;
        goto end;
    }

    rtnl_addr = rtnl_addr_alloc();
    rtnl_addr_set_ifindex(rtnl_addr, ifindex);
    rtnl_addr_set_local(rtnl_addr, local_addr);
    
    err = rtnl_addr_add(sk, rtnl_addr, NLM_F_REPLACE);
    if (err < 0) {
        log_fatal("rtnl_addr_add(): %s.\n", nl_geterror(err));
        goto end;
    }

    /* configure routing */

    while (route_ptr != NULL) {
        struct rtnl_route *rtnl_route = rtnl_route_alloc();
        struct rtnl_nexthop *nexthop = rtnl_route_nh_alloc();
        struct nl_addr* address = NULL;

        if (rtnl_route == NULL) {
            err = 1;
            log_fatal("rtnl_route_alloc(): can't alloc.\n");
            goto route_end;
        }

        rtnl_route_set_family(rtnl_route, AF_INET6);
        err = nl_addr_parse(route_ptr->prefix, AF_INET6, &address);
        if (err < 0) {
            log_fatal("nl_addr_parse(): %s.\n", nl_geterror(err));
            goto route_end;
        }

        rtnl_route_set_dst(rtnl_route, address);
        nl_addr_put(address);
        address = NULL;

        if (nexthop == NULL) {
            err = 1;
            log_fatal("rtnl_route_nh_alloc(): can't alloc.\n");
            goto route_end;
        }

        rtnl_route_nh_set_ifindex(nexthop, ifindex);

        err = nl_addr_parse(route_ptr->nexthop, AF_INET6, &address);
        if (err < 0) {
            log_fatal("nl_addr_parse(): %s.\n", nl_geterror(err));
            goto route_end;
        }

        rtnl_route_nh_set_gateway(nexthop, address);
        nl_addr_put(address);
        address = NULL;
        rtnl_route_add_nexthop(rtnl_route, nexthop);

        err = rtnl_route_add(sk, rtnl_route, NLM_F_CREATE | NLM_F_REPLACE);
        if (err < 0) {
            log_fatal("rtnl_route_add(): %s.\n", nl_geterror(err));
            goto route_end;
        }

route_end:
        if (address != NULL) nl_addr_put(address);
        if (rtnl_route != NULL) rtnl_route_put(rtnl_route);
        // FIXME: free nexthop?
        route_ptr = route_ptr->next;
        if (err != 0) goto end;
    }

end:
    if (local_addr != NULL) nl_addr_put(local_addr);
    if (rtnl_addr != NULL) rtnl_addr_put(rtnl_addr);
    if (cache != NULL) nl_cache_free(cache);
    if (sit_link != NULL) rtnl_link_put(sit_link);

    return err;
}

int sit_destroy(struct nl_sock *sk, const char *name) {
    int err;
    struct rtnl_link *sit_link;

    err = sit_get(sk, name, &sit_link);
    if (err == 1) {
        err = 0; // non fatal
        goto end;
    } else if (err < 0) goto end;

    err = rtnl_link_delete(sk, sit_link);
    if (err < 0) {
        log_fatal("rtnl_link_delete(): %s.\n", nl_geterror(err));
        goto end;
    }

end:
    rtnl_link_put(sit_link);
    return err;
}

int sit_get(struct nl_sock *sk, const char *name, struct rtnl_link **link) {
    struct nl_cache *cache;
    int err;

    *link = NULL;

    err = rtnl_link_alloc_cache(sk, AF_UNSPEC, &cache);
    if (err < 0) {
        log_fatal("rtnl_link_alloc_cache(): %s.\n", nl_geterror(err));
        goto end;
    }

    struct rtnl_link *sit_link = rtnl_link_get_by_name(cache, name);
    if (sit_link == NULL) {
        log_error("rtnl_link_get_by_name(): can't find interface %s.\n", name);
        err = 1;
        goto end;
    }

    if (!rtnl_link_is_sit(sit_link)) {
        err = -1;
        log_fatal("rtnl_link_is_sit(): link is not sit.\n");
        goto end;
    }

    *link = sit_link;

end:
    nl_cache_free(cache);
    return err;
}