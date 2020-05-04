#ifndef SITD_SIT_H
#define SITD_SIT_H
#include <stdint.h>
#include <netlink/route/link.h>
#include "types.h"

#define SIT_OK 0
#define SIT_NOT_EXIST 1 
#define SIT_ERROR 2
#define SIT_FATAL 3

int sit_get(struct nl_sock *sk, const char *name, struct rtnl_link **link);
int sit_configure(struct nl_sock *sk, const sit_tunnel_t *tunnel, const sit_route_t *route);
int sit_destroy(struct nl_sock *sk, const char *name);

#endif // SITD_SIT_H