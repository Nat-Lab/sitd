#include <netlink/route/link/sit.h>
#include <netlink/route/addr.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include "sit.h"
#include "log.h"

int sit_create(struct nl_sock *sk, const sit_config_t *config) {
    struct nl_cache *cache = NULL;
	struct rtnl_link *sit_link;
	in_addr_t laddr, raddr;
    struct in6_addr addr6;
    struct nl_addr* local_addr = NULL;
    struct rtnl_addr* rtnl_addr = NULL;
	int err, ifindex;

    err = inet_pton(AF_INET, config->local, &laddr);
    if (err != 1) {
        err = 1;
        log_fatal("inet_pton(): bad local address.\n");
        goto end;
    }

	err = inet_pton(AF_INET, config->remote, &raddr);
    if (err != 1) {
        err = 1;
        log_fatal("inet_pton(): bad remote address.\n");
        goto end;
    }

    err = inet_pton(AF_INET6, config->address, &addr6);
    if (err != 1) {
        err = 1;
        log_fatal("inet_pton(): bad interface address.\n");
        goto end;
    }

    err = rtnl_link_alloc_cache(sk, AF_UNSPEC, &cache);
    if (err < 0) {
        log_fatal("rtnl_link_alloc_cache(): %s.\n", nl_geterror(err));
        goto end;
    }

    sit_link = rtnl_link_sit_alloc();
    if (sit_link == NULL) {
        log_fatal("rtnl_link_sit_alloc(): %s.\n", nl_geterror(err));
        goto end;
    }

    rtnl_link_set_name(sit_link, config->name);
	rtnl_link_sit_set_local(sit_link, laddr);
	rtnl_link_sit_set_remote(sit_link, raddr);
    rtnl_link_sit_set_ttl(sit_link, 255);
    rtnl_link_set_flags(sit_link, IFF_UP);

    if (config->mtu != 0) {
        rtnl_link_set_mtu(sit_link, config->mtu);
    }

    err = rtnl_link_add(sk, sit_link, NLM_F_CREATE);
    if (err < 0) {
        log_fatal("rtnl_link_add(): %s.\n", nl_geterror(err));
        goto end;
    }

    rtnl_link_put(sit_link);

    local_addr = nl_addr_build(AF_INET6, &addr6, sizeof(struct in6_addr));
    if (local_addr == NULL) {
        log_fatal("nl_addr_build(): %s.\n", nl_geterror(err));
        goto end;
    }
    nl_addr_set_prefixlen(local_addr, config->address_mask);

    sit_link = NULL;
    err = sit_get(sk, config->name, &sit_link);
    if (err < 0 || sit_link == NULL) {
        log_fatal("sit_get(): can't get interface.\n");
        goto end;
    }

    ifindex = rtnl_link_get_ifindex(sit_link);
    rtnl_link_put(sit_link);
    if (ifindex == 0) {
        log_fatal("rtnl_link_get_ifindex(): can't get interface index.\n");
        err = 1;
        goto end;
    }

    rtnl_addr = rtnl_addr_alloc();
    rtnl_addr_set_ifindex(rtnl_addr, ifindex);
    rtnl_addr_set_local(rtnl_addr, local_addr);
    
    
    err = rtnl_addr_add(sk, rtnl_addr, 0);
    if (err < 0) {
        log_fatal("rtnl_addr_add(): %s.\n", nl_geterror(err));
        goto end;
    }

end:
    if (local_addr != NULL) nl_addr_put(local_addr);
    if (rtnl_addr != NULL) rtnl_addr_put(rtnl_addr);
    if (cache != NULL) nl_cache_free(cache);

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