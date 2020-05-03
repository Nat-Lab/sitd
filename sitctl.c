#include <netlink/netlink.h>
#include "sit.h"
#include "log.h"

int main () {
    struct nl_sock *sk;
    int err;
    sk = nl_socket_alloc();

    if (sk == NULL) {
        log_fatal("nl_socket_alloc() returned null.\n");
        goto end;
    }

    err = nl_connect(sk, NETLINK_ROUTE);
    if (err < 0) {
        log_fatal("nl_connect(): %s.\n", nl_geterror(err));
        goto end;
    }

    sit_config_t config;
    config.local = "172.17.0.14";
    config.remote = "172.17.0.1";
    config.name = "sit2";
    config.address = "fd00::1/64";
    config.mtu = 0;

    sit_route_t route;
    route.prefix = "2602:feda:333::/48";
    route.nexthop = "fd00::2";

    sit_route_t route2;
    route2.prefix = "2602:feda:444::/48";
    route2.nexthop = "fd00::3";
    route2.next = NULL;
    route.next = &route2;

    config.routes = &route;

    sit_destroy(sk, "sit2");
    sit_create(sk, &config);

end:
    if (sk != NULL) {
        nl_close(sk);
        nl_socket_free(sk);
        return err;
    }
}