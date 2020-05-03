#ifndef SITD_TYPES_H
#define SITD_TYPES_H
#include <stdint.h>
#include <linux/if.h>
#include <netinet/in.h>

typedef struct sit_route {
    uint32_t id;
    uint32_t tunnel_id;
    char prefix[INET6_ADDRSTRLEN + 4];
    char nexthop[INET6_ADDRSTRLEN];
    struct sit_route *next;
} sit_route_t;

typedef struct sit_tunnel {
    uint32_t id;
    char name[IFNAMSIZ];
    char local[INET_ADDRSTRLEN];
    char remote[INET_ADDRSTRLEN];
    char address[INET6_ADDRSTRLEN + 4];
    uint32_t mtu;
    struct sit_tunnel *next;
} sit_tunnel_t;

#endif // SITD_TYPES_H