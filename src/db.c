#include <sqlite3.h>
#include <stdlib.h>
#include "log.h"

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

int db_init();

int db_open(const char *file) {
    if (db != NULL) {
        log_fatal("database is already opened.\n");
        return -1;
    }

    int err = sqlite3_open_v2(file, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

    if (err == SQLITE_OK) return db_init();

    log_fatal("sqlite3_open_v2(): %s.\n", sqlite3_errmsg(db));
    return -1;
}

int db_init() {
    int err;
    char *errmsg;

    if (db == NULL) {
        log_fatal("database not yet opened.\n");
        return -1;
    }

    static const char create_tables[] = 
        "PRAGMA foreign_keys=ON;"
        "CREATE TABLE IF NOT EXISTS `tunnels` ("
            "`id`       INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT UNIQUE,"
            "`name`     TEXT NOT NULL,"
            "`local`    TEXT NOT NULL,"
            "`remote`   TEXT NOT NULL,"
            "`address`  TEXT NOT NULL"
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
        log_fatal("sqlite3_exec(): %s.\n", errmsg);
        goto end;
    }

    err = sqlite3_prepare_v2(db, "select * from tunnels", -1, &stmt_get_tunnels, NULL);
    err += sqlite3_prepare_v2(db, "select * from tunnels where `name` = ?", -1, &stmt_get_tunnel, NULL);
    err += sqlite3_prepare_v2(db, "insert into tunnels (`name`, `local`, `remote`, `address`, `mtu`) values (?, ?, ?, ?, ?)", -1, &stmt_insert_tunnel, NULL);
    err += sqlite3_prepare_v2(db, "update tunnels set (`name`, `local`, `remote`, `address`, `mtu`) = (?, ?, ?, ?, ?) where `id` = ?", -1, &stmt_update_tunnel, NULL);

    err += sqlite3_prepare_v2(db, "select * from routes where `tunnel_id` = ?", -1, &stmt_get_routes, NULL);
    err += sqlite3_prepare_v2(db, "select * from routes where `tunnel_id` = ? and `route` = ?", -1, &stmt_get_route, NULL);
    err += sqlite3_prepare_v2(db, "insert into routes (`route`, `nexthop`, `tunnel_id`) values (?, ?, ?)", -1, &stmt_insert_route, NULL);
    err += sqlite3_prepare_v2(db, "update routes set (`route`, `nexthop`) = (?, ?) where `tunnel_id` = ?", -1, &stmt_update_route, NULL);

    err += sqlite3_prepare_v2(db, "delete from tunnels where `id` = ?", -1, &stmt_del_tunnel, NULL);
    err += sqlite3_prepare_v2(db, "delete from routes where `tunnel_id` = ? and `route` = ?", -1, &stmt_del_route, NULL);

    err += sqlite3_prepare_v2(db, "select last_insert_rowid()", -1, &stmt_last_id, NULL);

    if (err != SQLITE_OK) {
        log_fatal("sqlite3_prepare_v2(): %s.\n", sqlite3_errmsg(db));
        goto end;
    }
    
end:
    sqlite3_free(errmsg);
    return err;
}

int db_close() {
    if (db == NULL) {
        log_fatal("database not yet opened.\n");
        return -1;
    }

    int err = sqlite3_close_v2(db);
    if (err != SQLITE_OK) log_fatal("sqlite3_close_v2(): %s.\n", sqlite3_errmsg(db));
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

    if (err != SQLITE_OK) log_fatal("sqlite3_finalize(): %s.\n", sqlite3_errmsg(db));

    stmt_get_routes = stmt_get_tunnel = stmt_insert_route = 
        stmt_insert_tunnel = stmt_update_route = stmt_update_tunnel = NULL;

    return err;
}

int main () {
    db_open("test.db");
    db_close();
    return 0;
}