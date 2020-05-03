#ifndef SITD_SIT_H
#define SITD_SIT_H
#include <stdint.h>
#include <netlink/route/link.h>

typedef struct sit_route {
    const char *prefix;
    const char *nexthop;
    struct sit_route *next;
} sit_route_t;

typedef struct sit_config {
    const char *name;
    const char *local;
    const char *remote;
    const char *address;
    uint32_t mtu;
    sit_route_t *routes;
} sit_config_t;

int sit_get(struct nl_sock *sk, const char *name, struct rtnl_link **link);
int sit_create(struct nl_sock *sk, const sit_config_t *config);
int sit_destroy(struct nl_sock *sk, const char *name);

#endif // SITD_SIT_H