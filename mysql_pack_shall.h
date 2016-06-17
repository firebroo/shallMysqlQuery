#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define DEA_PORT 3306
#define BUFFER_SIZE 65536
#define MYSQL_CTOS_PROTOCOL_SIZE 5

#define PACK_LEN(x) (x & 0x00FFFFFF)
#define PACK_NUM(x) ((x & (~0x00FFFFFF)) >> 24)

int sock_raw;
struct sockaddr_in source,dest;
unsigned short port = DEA_PORT;

enum enum_server_command
{
  COM_SLEEP, 
  COM_QUIT, 
  COM_INIT_DB, 
  COM_QUERY, 
  COM_FIELD_LIST,
  COM_CREATE_DB, 
  COM_DROP_DB, 
  COM_REFRESH, 
  COM_SHUTDOWN, 
  COM_STATISTICS,
  COM_PROCESS_INFO, 
  COM_CONNECT, 
  COM_PROCESS_KILL, 
  COM_DEBUG, 
  COM_PING,
  COM_TIME, 
  COM_DELAYED_INSERT, 
  COM_CHANGE_USER, 
  COM_BINLOG_DUMP,
  COM_TABLE_DUMP,
  COM_CONNECT_OUT, 
  COM_REGISTER_SLAVE,
  COM_STMT_PREPARE, 
  COM_STMT_EXECUTE, 
  COM_STMT_SEND_LONG_DATA, 
  COM_STMT_CLOSE,
  COM_STMT_RESET, 
  COM_SET_OPTION, 
  COM_STMT_FETCH, 
  COM_DAEMON,
  COM_END
};

int check_argv(int argc, char *argv[]);

unsigned short validate_port(char *p);

void handle_tcp_packet(unsigned char* Buffer);

void process_packet(unsigned char* buffer);
