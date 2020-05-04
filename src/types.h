#ifndef SITD_TYPES_H
#define SITD_TYPES_H
#include <stdbool.h>
#include <stdint.h>
#include <jansson.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <netinet/in.h>

typedef enum tunnel_state {
    STATE_RUNNING,
    STETE_STOPPED
} tunnel_state_t;

#define field(type, name) type name; bool name##_isset
#define array_field(type, len, name) type name[len]; bool name##_isset

typedef struct sit_route {
    field(uint32_t, id);
    field(uint32_t, tunnel_id);
    array_field(char, INET6_ADDRSTRLEN + 4, prefix);
    array_field(char, INET6_ADDRSTRLEN, nexthop);
    struct sit_route *next;
} sit_route_t;

typedef struct sit_tunnel {
    field(uint32_t, id);
    field(tunnel_state_t, state);
    array_field(char, IFNAMSIZ, name);
    array_field(char, INET_ADDRSTRLEN, local);
    array_field(char, INET_ADDRSTRLEN, remote);
    array_field(char, INET6_ADDRSTRLEN + 4, address);
    field(uint32_t, mtu);
    struct sit_tunnel *next;
} sit_tunnel_t;

#define set_val_numeric(obj_path, value) {obj_path = value; obj_path##_isset = true;}
#define set_val_string(obj_path, src, length) {strncpy(obj_path, src, length); obj_path##_isset = true;}
#define isset(obj_path) ( obj_path##_isset )

int sit_tunnel_to_json(const sit_tunnel_t *tunnel, json_t **json);
int sit_route_to_json(const sit_route_t *route, json_t **json);
int json_to_sit_route(const json_t *json, sit_route_t **route);
int json_to_sit_tunnel(const json_t *json, sit_route_t **route);

#endif // SITD_TYPES_H