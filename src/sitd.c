#include <netlink/netlink.h>
#include "sit.h"
#include "log.h"
#include "db.h"
#include "api.h"

int tunnel_api_handler (struct MHD_Connection *conn, const char *method, size_t argc, const char **argv, const json_t *req) {
    log_debug("tunnel_api: %s, args: \n", method);
    for (int i = 0; i < argc; i++) {
        log_debug("arg %d: %s\n", i, argv[i]);
    }

    // echo
    return api_respond(conn, 200, req);

}

int route_api_handler (struct MHD_Connection *conn, const char *method, size_t argc, const char **argv, const json_t *req) {
    log_debug("route_api: %s, args: \n", method);
    for (int i = 0; i < argc; i++) {
        log_debug("arg %d: %s\n", i, argv[i]);
    }

    // echo
    return api_respond(conn, 200, req);
}

int main () {
    /*struct nl_sock *sk;
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
        sit_destroy(sk, tunnel->name);
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
    }*/

    api_register_handler("/api/v1/tunnel/:tunnel_name", &tunnel_api_handler);
    api_register_handler("/api/v1/tunnel/", &tunnel_api_handler);
    api_register_handler("/api/v1/tunnel/:tunnel_name/route/:route", &route_api_handler);
    api_register_handler("/api/v1/tunnel/:tunnel_name/route/", &route_api_handler);
    api_start(8123);
    getchar();
    api_stop();
    api_clear_handlers();

    return 0;
}