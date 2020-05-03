#ifndef SITD_DB_H
#define SITD_DB_H
#include "types.h"

int db_open(const char *file);
int db_close();

int db_get_tunnels(sit_tunnel_t **tunnels);
int db_get_tunnel(const char* name, sit_tunnel_t **tunnel);

int db_get_routes(const sit_tunnel_t *tunnel, sit_route_t **routes);
int db_get_route(const char* prefix, const sit_tunnel_t *tunnel, sit_route_t **route);

int db_create_tunnel(const sit_tunnel_t *tunnel);
int db_create_route(const sit_route_t *route);

int db_update_tunnel(const sit_tunnel_t *tunnel);
int db_update_route(const sit_route_t *route);

void db_free_result_tunnels(sit_tunnel_t *tunnels);
void db_free_result_routes(sit_route_t *routes);

#endif // SITD_DB_H