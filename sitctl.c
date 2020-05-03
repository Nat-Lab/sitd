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
    config.address = "fd00::1";
    config.address_mask = 64;
    config.mtu = 0;

    sit_destroy(sk, "sit2");
    sit_create(sk, &config);

end:
    if (sk != NULL) {
        nl_close(sk);
        nl_socket_free(sk);
        return err;
    }
}