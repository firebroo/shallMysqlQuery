#ifndef MYSQL_PARSE_H
#define MYSQL_PARSE_H

#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MYSQL_CTOS_PROTOCOL_SIZE  5

#define PACK_LEN(x) (x & 0x00FFFFFF)
#define PACK_NUM(x) ((x & (~0x00FFFFFF)) >> 24)
#define CSTRINGLEN(string) strlen(string)+1
#define MAX(a, b) ((a) > (b)? (a): (b))

#define CLIENT_LONG_PASSWORD	1	/* new more secure passwords */
#define CLIENT_FOUND_ROWS	2	/* Found instead of affected rows */
#define CLIENT_LONG_FLAG	4	/* Get all column flags */
#define CLIENT_CONNECT_WITH_DB	8	/* One can specify db on connect */
#define CLIENT_NO_SCHEMA	16	/* Don't allow database.table.column */
#define CLIENT_COMPRESS		32	/* Can use compression protocol */
#define CLIENT_ODBC		64	/* Odbc client */
#define CLIENT_LOCAL_FILES	128	/* Can use LOAD DATA LOCAL */
#define CLIENT_IGNORE_SPACE	256	/* Ignore spaces before '(' */
#define CLIENT_PROTOCOL_41	512	/* New 4.1 protocol */
#define CLIENT_INTERACTIVE	1024	/* This is an interactive client */
#define CLIENT_SSL              2048	/* Switch to SSL after handshake */
#define CLIENT_IGNORE_SIGPIPE   4096    /* IGNORE sigpipes */
#define CLIENT_TRANSACTIONS	8192	/* Client knows about transactions */
#define CLIENT_RESERVED         16384   /* Old flag for 4.1 protocol  */
#define CLIENT_SECURE_CONNECTION 32768  /* New 4.1 authentication */
#define CLIENT_MULTI_STATEMENTS (1UL << 16) /* Enable/disable multi-stmt support */
#define CLIENT_MULTI_RESULTS    (1UL << 17) /* Enable/disable multi-results */
#define CLIENT_PS_MULTI_RESULTS (1UL << 18) /* Multi-results in PS-protocol */

#define CLIENT_PLUGIN_AUTH  (1UL << 19) /* Client supports plugin authentication */

#define CLIENT_SSL_VERIFY_SERVER_CERT (1UL << 30)
#define CLIENT_REMEMBER_OPTIONS (1UL << 31)



//extern unsigned short  port;

struct stmt_prepare_response {
    char   status;
    int    statement_id;
    short  num_columns;
    short  num_params;
    char   reserved_1;
    short  warning_count;
} __attribute__((packed));

struct handshake {
    int    connection_id;
    char   auth_plugin_data_part[8];
    char   filler;
    short  capability_flag_1;
    //char   character_set;
    //short  status_flags;
    //short  capability_flags_2;
    //char   auth_plugin_data_len;
} __attribute__((packed));

enum enum_server_command
{
    COM_SLEEP,                 /*0*/
    COM_QUIT,                  
    COM_INIT_DB,
    COM_QUERY,
    COM_FIELD_LIST,
    COM_CREATE_DB,            /*5*/
    COM_DROP_DB,
    COM_REFRESH,
    COM_SHUTDOWN,
    COM_STATISTICS,
    COM_PROCESS_INFO,        /*10*/
    COM_CONNECT,
    COM_PROCESS_KILL,
    COM_DEBUG,
    COM_PING,
    COM_TIME,                /*15*/
    COM_DELAYED_INSERT,
    COM_CHANGE_USER,
    COM_BINLOG_DUMP,
    COM_TABLE_DUMP,
    COM_CONNECT_OUT,        /*20*/
    COM_REGISTER_SLAVE,
    COM_STMT_PREPARE,
    COM_STMT_EXECUTE,
    COM_STMT_SEND_LONG_DATA,
    COM_STMT_CLOSE,        /*25*/
    COM_STMT_RESET,
    COM_SET_OPTION,
    COM_STMT_FETCH,
    COM_DAEMON,
    COM_END               /*30*/
};

enum method {
    RESQUEST,
    RESPONSE
};

enum cursor_flag {
    CURSOR_TYPE_NO_CURSOR,
    CURSOR_TYPE_READ_ONLY,
    CURSOR_TYPE_FOR_UPDATE,
    CURSOR_TYPE_SCROLLABLE
};

enum enum_field_types {
    MYSQL_TYPE_DECIMAL,
    MYSQL_TYPE_TINY,
    MYSQL_TYPE_SHORT,
    MYSQL_TYPE_LONG,
    MYSQL_TYPE_FLOAT,
    MYSQL_TYPE_DOUBLE,
    MYSQL_TYPE_NULL,
    MYSQL_TYPE_TIMESTAMP,
    MYSQL_TYPE_LONGLONG,
    MYSQL_TYPE_INT24,
    MYSQL_TYPE_DATE,
    MYSQL_TYPE_TIME,
    MYSQL_TYPE_DATETIME,
    MYSQL_TYPE_YEAR,
    MYSQL_TYPE_NEWDATE,
    MYSQL_TYPE_VARCHAR,
    MYSQL_TYPE_BIT,
    MYSQL_TYPE_NEWDECIMAL=246,
    MYSQL_TYPE_ENUM=247,
    MYSQL_TYPE_SET=248,
    MYSQL_TYPE_TINY_BLOB=249,
    MYSQL_TYPE_MEDIUM_BLOB=250,
    MYSQL_TYPE_LONG_BLOB=251,
    MYSQL_TYPE_BLOB=252,
    MYSQL_TYPE_VAR_STRING=253,
    MYSQL_TYPE_STRING=254,
    MYSQL_TYPE_GEOMETRY=255
};

/* For backward compatibility */
#define CLIENT_MULTI_QUERIES    CLIENT_MULTI_STATEMENTS
#define FIELD_TYPE_DECIMAL     MYSQL_TYPE_DECIMAL
#define FIELD_TYPE_NEWDECIMAL  MYSQL_TYPE_NEWDECIMAL
#define FIELD_TYPE_TINY        MYSQL_TYPE_TINY
#define FIELD_TYPE_SHORT       MYSQL_TYPE_SHORT
#define FIELD_TYPE_LONG        MYSQL_TYPE_LONG
#define FIELD_TYPE_FLOAT       MYSQL_TYPE_FLOAT
#define FIELD_TYPE_DOUBLE      MYSQL_TYPE_DOUBLE
#define FIELD_TYPE_NULL        MYSQL_TYPE_NULL
#define FIELD_TYPE_TIMESTAMP   MYSQL_TYPE_TIMESTAMP
#define FIELD_TYPE_LONGLONG    MYSQL_TYPE_LONGLONG
#define FIELD_TYPE_INT24       MYSQL_TYPE_INT24
#define FIELD_TYPE_DATE        MYSQL_TYPE_DATE
#define FIELD_TYPE_TIME        MYSQL_TYPE_TIME
#define FIELD_TYPE_DATETIME    MYSQL_TYPE_DATETIME
#define FIELD_TYPE_YEAR        MYSQL_TYPE_YEAR
#define FIELD_TYPE_NEWDATE     MYSQL_TYPE_NEWDATE
#define FIELD_TYPE_ENUM        MYSQL_TYPE_ENUM
#define FIELD_TYPE_SET         MYSQL_TYPE_SET
#define FIELD_TYPE_TINY_BLOB   MYSQL_TYPE_TINY_BLOB
#define FIELD_TYPE_MEDIUM_BLOB MYSQL_TYPE_MEDIUM_BLOB
#define FIELD_TYPE_LONG_BLOB   MYSQL_TYPE_LONG_BLOB
#define FIELD_TYPE_BLOB        MYSQL_TYPE_BLOB
#define FIELD_TYPE_VAR_STRING  MYSQL_TYPE_VAR_STRING
#define FIELD_TYPE_STRING      MYSQL_TYPE_STRING
#define FIELD_TYPE_CHAR        MYSQL_TYPE_TINY
#define FIELD_TYPE_INTERVAL    MYSQL_TYPE_ENUM
#define FIELD_TYPE_GEOMETRY    MYSQL_TYPE_GEOMETRY
#define FIELD_TYPE_BIT         MYSQL_TYPE_BIT


void hashtable_init(void);
char* mysql_process_packet(unsigned char* buffer, int size);

#endif /* MYSQL_PARSE */
