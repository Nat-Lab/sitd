#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "db.h"

static sqlite3 *db = NULL;

static sqlite3_stmt *stmt_get_tunnels = NULL;
static sqlite3_stmt *stmt_get_tunnel = NULL;
static sqlite3_stmt *stmt_insert_tunnel = NULL;
static sqlite3_stmt *stmt_update_tunnel = NULL;

static sqlite3_stmt *stmt_get_routes = NULL;
static sqlite3_stmt *stmt_get_route = NULL;
static sqlite3_stmt *stmt_insert_route = NULL;
static sqlite3_stmt *stmt_update_route = NULL;

static sqlite3_stmt *stmt_del_tunnel = NULL;
static sqlite3_stmt *stmt_del_route = NULL;

static sqlite3_stmt *stmt_last_id = NULL;

static int db_init();

int db_open(const char *file) {
    if (db != NULL) {
        log_fatal("database is already opened.\n");
        return SIT_DB_FATAL;
    }

    int err = sqlite3_open_v2(file, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (err == SQLITE_OK) return db_init();

    log_fatal("sqlite3_open_v2(): %s.\n", sqlite3_errmsg(db));
    return SIT_DB_FATAL;
}

static int db_init() {
    int err;
    char *errmsg;

    if (db == NULL) {
        log_fatal("database not yet opened.\n");
        return SIT_DB_FATAL;
    }

    static const char create_tables[] = 
        "PRAGMA foreign_keys=ON;"
        "CREATE TABLE IF NOT EXISTS `tunnels` ("
            "`id`       INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,"
            "`state`      INTEGER NOT NULL,"
            "`name`     TEXT NOT NULL UNIQUE,"
            "`local`    TEXT NOT NULL,"
            "`remote`   TEXT NOT NULL UNIQUE,"
            "`address`  TEXT NOT NULL UNIQUE,"
            "`mtu`      INTEGER NOT NULL DEFAULT 0"
        ");"
        "CREATE TABLE IF NOT EXISTS `routes` ("
            "`id`         INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,"
            "`route`      TEXT NOT NULL UNIQUE,"
            "`nexthop`    TEXT NOT NULL,"
            "`tunnel_id`  INTEGER NOT NULL,"
            "CONSTRAINT   `tunnel_id`"
                "FOREIGN KEY(`tunnel_id`)"
                "REFERENCES tunnels ( id )"
                "ON DELETE CASCADE"
        ");";

    
    err = sqlite3_exec(db, create_tables, NULL, NULL, &errmsg);

    if (err != SQLITE_OK) {
        err = SIT_DB_FATAL;
        log_fatal("sqlite3_exec(): %s.\n", errmsg);
        goto end;
    }

    err = sqlite3_prepare_v2(db, "select * from tunnels", -1, &stmt_get_tunnels, NULL);
    err += sqlite3_prepare_v2(db, "select * from tunnels where `name` = ?", -1, &stmt_get_tunnel, NULL);
    err += sqlite3_prepare_v2(db, "insert into tunnels (`state`, `name`, `local`, `remote`, `address`, `mtu`) values (?, ?, ?, ?, ?, ?)", -1, &stmt_insert_tunnel, NULL);
    err += sqlite3_prepare_v2(db, "update tunnels set (`state`, `name`, `local`, `remote`, `address`, `mtu`) = (?, ?, ?, ?, ?, ?) where `id` = ?", -1, &stmt_update_tunnel, NULL);

    err += sqlite3_prepare_v2(db, "select * from routes where `tunnel_id` = ?", -1, &stmt_get_routes, NULL);
    err += sqlite3_prepare_v2(db, "select * from routes where `tunnel_id` = ? and `route` = ?", -1, &stmt_get_route, NULL);
    err += sqlite3_prepare_v2(db, "insert into routes (`route`, `nexthop`, `tunnel_id`) values (?, ?, ?)", -1, &stmt_insert_route, NULL);
    err += sqlite3_prepare_v2(db, "update routes set (`route`, `nexthop`) = (?, ?) where `tunnel_id` = ?", -1, &stmt_update_route, NULL);

    err += sqlite3_prepare_v2(db, "delete from tunnels where `id` = ?", -1, &stmt_del_tunnel, NULL);
    err += sqlite3_prepare_v2(db, "delete from routes where `tunnel_id` = ? and `route` = ?", -1, &stmt_del_route, NULL);

    err += sqlite3_prepare_v2(db, "select last_insert_rowid()", -1, &stmt_last_id, NULL);

    if (err != SQLITE_OK) {
        err = SIT_DB_FATAL;
        log_fatal("sqlite3_prepare_v2(): %s.\n", sqlite3_errmsg(db));
        goto end;
    }
    
end:
    sqlite3_free(errmsg);
    return err;
}

int db_close() {
    if (db == NULL) {
        log_error("database not yet opened.\n");
        return SIT_DB_ERROR;
    }

    int err = sqlite3_close_v2(db);
    if (err != SQLITE_OK) {
        log_fatal("sqlite3_close_v2(): %s.\n", sqlite3_errmsg(db));
        err = SIT_DB_FATAL;
    }
    else db = NULL;

    err = sqlite3_finalize(stmt_get_routes);
    err += sqlite3_finalize(stmt_get_route);
    err += sqlite3_finalize(stmt_get_tunnels);
    err += sqlite3_finalize(stmt_get_tunnel);
    err += sqlite3_finalize(stmt_insert_route);
    err += sqlite3_finalize(stmt_insert_tunnel);
    err += sqlite3_finalize(stmt_update_route);
    err += sqlite3_finalize(stmt_update_tunnel);
    err += sqlite3_finalize(stmt_del_route);
    err += sqlite3_finalize(stmt_del_tunnel);
    err += sqlite3_finalize(stmt_last_id);

    if (err != SQLITE_OK) {
        log_fatal("sqlite3_finalize(): %s.\n", sqlite3_errmsg(db));
        err = SIT_DB_FATAL;
    }

    stmt_get_routes = stmt_get_tunnel = stmt_insert_route = 
        stmt_insert_tunnel = stmt_update_route = stmt_update_tunnel = NULL;

    return err;
}

int db_get_tunnels(sit_tunnel_t **tunnels) {
    int err;
    size_t n = 0;
    sit_tunnel_t *current = *tunnels, *prev = NULL;

    *tunnels = (sit_tunnel_t *) malloc(sizeof(sit_tunnel_t));
    if (*tunnels == NULL) {
        err = SIT_DB_FATAL;
        log_fatal("malloc() failed.\n");
        goto end;
    }

    memset(current, 0, sizeof(sit_tunnel_t));

    err = sqlite3_reset(stmt_get_tunnels);
    if (err != SQLITE_OK) {
        err = SIT_DB_FATAL;
        log_error("sqlite3_reset(): %s.\n", sqlite3_errmsg(db));
        goto end;
    }

    do {
        err = sqlite3_step(stmt_get_tunnels);
        if (err == SQLITE_DONE) break;
        ++n;

        if (err != SQLITE_ROW) {
            err = SIT_DB_ERROR;
            log_error("sqlite3_step(): %s (%d).\n", sqlite3_errmsg(db), err);
            goto end;
        }

        set_val_numeric(current->id, sqlite3_column_int(stmt_get_tunnels, 0));
        set_val_numeric(current->state, sqlite3_column_int(stmt_get_tunnels, 1));
        set_val_string(current->name, (char *) sqlite3_column_text(stmt_get_tunnels, 2), IFNAMSIZ);
        set_val_string(current->name, (char *) sqlite3_column_text(stmt_get_tunnels, 2), IFNAMSIZ);
        set_val_string(current->local, (char *) sqlite3_column_text(stmt_get_tunnels, 3), INET_ADDRSTRLEN);
        set_val_string(current->remote, (char *) sqlite3_column_text(stmt_get_tunnels, 4), INET_ADDRSTRLEN);
        set_val_string(current->address, (char *) sqlite3_column_text(stmt_get_tunnels, 5), INET6_ADDRSTRLEN + 4);
        set_val_numeric(current->mtu, sqlite3_column_int(stmt_get_tunnels, 6));
        
        prev = current;
        current->next = (sit_tunnel_t *) malloc(sizeof(sit_tunnel_t));
        if (current->next == NULL) {
            log_fatal("malloc() failed.\n");
            goto end;
        }
        current = current->next;
        memset(current, 0, sizeof(sit_tunnel_t));

    } while (err != SQLITE_DONE);

    err = prev == NULL ? SIT_DB_NOT_EXIST : SIT_DB_OK;

end:
    if (prev == NULL) {
        if (*tunnels != NULL) free(*tunnels);
        *tunnels = NULL;
    } else {
        if (current != NULL) free(current);
        prev->next = NULL;
    }
    return err;
}

int db_get_tunnel(const char* name, sit_tunnel_t **tunnel) {
    *tunnel = NULL;
    int err;

    err = sqlite3_reset(stmt_get_tunnel);
    if (err != SQLITE_OK) {
        err = SIT_DB_FATAL;
        log_error("sqlite3_reset(): %s.\n", sqlite3_errmsg(db));
        goto end;
    }

    err = sqlite3_clear_bindings(stmt_get_tunnel);
    if (err != SQLITE_OK) {
        err = SIT_DB_FATAL;
        log_error("sqlite3_clear_bindings(): %s.\n", sqlite3_errmsg(db));
        goto end;
    }

    err = sqlite3_bind_text(stmt_get_tunnel, 1, name, -1, NULL);
    if (err != SQLITE_OK) {
        err = SIT_DB_ERROR;
        log_error("sqlite3_bind_text(): %s.\n", sqlite3_errmsg(db));
        goto end;
    }

    err = sqlite3_step(stmt_get_tunnel);

    if (err == SQLITE_DONE) {
        err = SIT_DB_NOT_EXIST;
        goto end;
    }

    if (err == SQLITE_ROW) {
        *tunnel = (sit_tunnel_t *) malloc(sizeof(sit_tunnel_t));
        sit_tunnel_t *current = *tunnel;
        set_val_numeric(current->id, sqlite3_column_int(stmt_get_tunnel, 0));
        set_val_numeric(current->state, sqlite3_column_int(stmt_get_tunnel, 1));
        set_val_string(current->name, (char *) sqlite3_column_text(stmt_get_tunnel, 2), IFNAMSIZ);
        set_val_string(current->name, (char *) sqlite3_column_text(stmt_get_tunnel, 2), IFNAMSIZ);
        set_val_string(current->local, (char *) sqlite3_column_text(stmt_get_tunnel, 3), INET_ADDRSTRLEN);
        set_val_string(current->remote, (char *) sqlite3_column_text(stmt_get_tunnel, 4), INET_ADDRSTRLEN);
        set_val_string(current->address, (char *) sqlite3_column_text(stmt_get_tunnel, 5), INET6_ADDRSTRLEN + 4);
        set_val_numeric(current->mtu, sqlite3_column_int(stmt_get_tunnel, 6));
        current->next = NULL;
    } else {
        log_error("sqlite3_step(): %s\n", sqlite3_errmsg(db));
        db = SIT_DB_ERROR;
    }

    err = sqlite3_step(stmt_get_tunnel);
    if (err != SQLITE_DONE) {
        err = SIT_DB_ERROR;
        log_error("sqlite3_step(): expected SQLITE_DONE, but saw %d.\n", err);
        goto end;
    }
    
    err = SIT_DB_OK;

end:
    if (err != SIT_DB_OK && *tunnel != NULL) {
        free(*tunnel);
        *tunnel = NULL;
    }

    return err;
}

void db_free_result_tunnels(sit_tunnel_t *tunnels) {
    sit_tunnel_t *tunnel = tunnels, *next;
    while (tunnel != NULL) {
        next = tunnel->next;
        free(tunnel);
        tunnel = next;
    }
}