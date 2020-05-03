#include <netlink/netlink.h>
#include "sit.h"
#include "log.h"
#include "db.h"

int main () {
    struct nl_sock *sk;
    int err;
    sit_tunnel_t *tunnels, *tunnel;
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

    db_open("test.db");
    db_get_tunnels(&tunnels);

    tunnel = tunnels;

    while (tunnel != NULL) {
        sit_destroy(sk, tunnels->name);
        sit_configure(sk, tunnel, NULL);
        log_debug("%d name: %s, l: %s, r: %s, a: %s, m: %d, s: %d\n", tunnel->id, tunnel->name, tunnel->remote, tunnel->local, tunnel->address, tunnel->mtu, tunnel->state);
        tunnel = tunnel->next;
    }

    db_free_result_tunnels(tunnels);
    db_close();

end:
    if (sk != NULL) {
        nl_close(sk);
        nl_socket_free(sk);
        return err;
    }
}