#ifndef SITD_API_H
#define SITD_API_H
#include <microhttpd.h>
#include <jansson.h>
#include <stdint.h>

typedef int (*api_handler_t)(struct MHD_Connection *connection, const char *method, size_t arg_count, const char **url_args, const json_t *request_body);

int api_start(uint16_t port);
int api_stop();

void api_register_handler(const char* url_format, api_handler_t handler);
void api_clear_handlers();
void api_respond(struct MHD_Connection *connection, uint32_t http_code, const json_t *respond_body);

#endif // SITD_API_H