#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <string.h>
#include <memory.h>
#include "api.h"
#include "log.h"
#define RECV_BUFFER_SZ 0xffff

typedef struct handler_table {
    const char* url_format;
    api_handler_t handler;
    struct handler_table *next;
} handler_table_t;

static handler_table_t *handlers = NULL;
static handler_table_t *handlers_tail = NULL;
static struct MHD_Daemon *api_server = NULL;

void api_register_handler(const char* url_format, api_handler_t handler) {
    if (handlers == NULL) {
        handlers = (handler_table_t *) malloc(sizeof(handler_table_t));
        handlers_tail = handlers;
    }
    handlers_tail->handler = handler;
    handlers_tail->url_format = url_format;
    handlers_tail->next = malloc(sizeof(handler_table_t));
    handlers_tail = handlers_tail->next;
    handlers_tail->next = NULL;
}

void api_clear_handlers() {
    if (handlers == NULL) return;

    handler_table_t *handler = handlers, *next = NULL;

    while (handler != NULL) {
        if (handler != NULL) next = handler->next;
        free(handler);
        handler = next;
    }
}

static void free_args (char **args, size_t url_args_count) {
    if (args == NULL) return;
    for (int i = 0; i < url_args_count; i++) free(args[i]);
    free(args);
}

static int try_invole_handler(struct MHD_Connection *conn, const char *method, const char *url, const json_t *body) {
    handler_table_t *table_ptr = handlers;

    while (table_ptr != handlers_tail) {
        const char *handler_url_ptr = table_ptr->url_format;
        const char *url_ptr = url;

        char **url_args = NULL;
        size_t url_args_count = 0;
        int mismatch = 0;

        while (*handler_url_ptr != 0 && *url_ptr != 0) {
            while (*handler_url_ptr == *url_ptr && *url_ptr != 0) {
                ++handler_url_ptr;
                ++url_ptr;
            }

            if (*handler_url_ptr == 0 || *url_ptr == 0) break;

            if (*handler_url_ptr == ':') { // variable
                while (*handler_url_ptr != 0 && *handler_url_ptr != '/') handler_url_ptr++;

                const char *var_begin_ptr = url_ptr;
                while (*url_ptr != 0 && *url_ptr != '/') url_ptr++;
                size_t arg_sz = url_ptr - var_begin_ptr + 1;
                
                if (url_args == NULL) {
                    url_args = (char **) malloc(sizeof(char *));
                    url_args[0] = (char *) malloc(arg_sz);
                    memcpy(url_args[0], var_begin_ptr, arg_sz);
                    url_args[0][arg_sz - 1] = 0;
                    url_args_count = 1;
                } else {
                    url_args = (char **) realloc(url_args, (url_args_count + 1) * sizeof(char *));

                    if (url_args == NULL) {
                        log_fatal("realloc() failed.\n");
                        return -1;
                    }

                    url_args[url_args_count] = (char *) malloc(arg_sz);
                    memcpy(url_args[url_args_count], var_begin_ptr, arg_sz);
                    url_args[url_args_count][arg_sz - 1] = 0;
                    ++url_args_count;
                }

            }

            
            if (*handler_url_ptr == *url_ptr) continue;

            mismatch = 1;
            free_args(url_args, url_args_count);
            url_args = NULL;
            break;
        }

        if (!mismatch && *handler_url_ptr == *url_ptr && *url_ptr == 0) {
            int res = table_ptr->handler(conn, method, url_args_count, (const char**) url_args, body);
            free_args(url_args, url_args_count);
            return res;
        }


        table_ptr = table_ptr->next;
    }
    
    return -1;
}

static int router (
    void *cls, struct MHD_Connection *connection,
    const char *url,
    const char *method, const char *version,
    const char *upload_data,
    size_t *upload_data_size, void **con_cls
) {
    static int old_conn_marker;
    static char buffer[RECV_BUFFER_SZ];
    static char* buffer_ptr;
    static size_t buffer_size;

    if (*con_cls == NULL) {
        buffer_size = 0;
        buffer_ptr = buffer;
        *con_cls = &old_conn_marker;
        return MHD_YES;
    }

    if (*upload_data_size != 0) {
        if (buffer_size + *upload_data_size > RECV_BUFFER_SZ) {
            log_error("client request body too big.\n");
            return MHD_NO;
        }

        memcpy(buffer_ptr, upload_data, *upload_data_size);
        buffer_ptr += *upload_data_size;
        buffer_size += *upload_data_size;

        *upload_data_size = 0;
        return MHD_YES;        
    }

    json_error_t err;
    json_t *body = buffer_size > 0 ? json_loadb(buffer, buffer_size, 0, &err) : NULL;

    if (body == NULL && buffer_size > 0) {
        log_error("json_loadb(): (%d, %d) %s\n", err.line, err.position, err.text);
        return MHD_NO;
    }

    int res = try_invole_handler(connection, method, url, body);

    json_decref(body);
    if (res != MHD_YES) {
        log_error("try_invole_handler(): %s %s: error finding/running handler.\n", method, url);
        return MHD_NO;
    }

    return MHD_YES;
}

int api_respond(struct MHD_Connection *connection, uint32_t http_code, const json_t *respond_body) {
    if (respond_body == NULL) {
        log_error("respond body null.\n");
        return MHD_NO;
    }

    int r;
    char *payload = json_dumps(respond_body, JSON_COMPACT);

    struct MHD_Response *response = MHD_create_response_from_buffer (strlen (payload), (void*) payload, MHD_RESPMEM_MUST_COPY);
    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "Server", "sitd");
    r = MHD_queue_response (connection, http_code, response);
    MHD_destroy_response (response);
    free(payload);

    return r;
}

int api_start(uint16_t port) {
    api_server = MHD_start_daemon(
        MHD_USE_DUAL_STACK | MHD_USE_EPOLL_INTERNALLY, 
        port, NULL, NULL, &router, NULL, 
        MHD_OPTION_CONNECTION_TIMEOUT, (uint32_t) 10, MHD_OPTION_END);
    
    if (api_server == NULL) {
        log_fatal("can't start api server.\n");
        return 1;
    }
    
    return 0;
}

int api_stop() {
    if (api_server == NULL) {
        log_fatal("api not running.\n");
        return 1;
    }

    MHD_stop_daemon(api_server);
    return 0;
}