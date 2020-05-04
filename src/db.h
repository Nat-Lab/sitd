#ifndef SITD_DB_H
#define SITD_DB_H
#include "types.h"

#define SIT_DB_OK 0
#define SIT_DB_NOT_EXIST 1 
#define SIT_DB_ALREADY_EXIST 2
#define SIT_DB_FAIL 3

int db_open(const char *file);
int db_close();

int db_get_tunnels(sit_tunnel_t **tunnels);
int db_get_tunnel(const char* name, sit_tunnel_t **tunnel);

int db_get_routes(uint32_t tunnel_id, sit_route_t **routes);
int db_get_route(const char* prefix, uint32_t tunnel_id, sit_route_t **route);

int db_create_tunnel(const sit_tunnel_t *tunnel);
int db_create_route(const sit_route_t *route);

int db_update_tunnel(const sit_tunnel_t *tunnel);
int db_update_route(const sit_route_t *route);

void db_free_result_tunnels(sit_tunnel_t *tunnels);
void db_free_result_routes(sit_route_t *routes);

#endif // SITD_DB_H