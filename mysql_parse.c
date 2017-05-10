#include "config.h"
#include "common.h"
#include "hashtable.h"
#include "mysql_parse.h"

/*
 * mysql protocol
 * read from http://dev.mysql.com/doc/internals/en/client-server-protocol.html
 * */

static char source[20], dest[20];
static unsigned short s_port, d_port;
static int capabilities;
static struct stmt_prepare_response res;
static enum method method;
extern short port;
static HashTable *hashtable;
static struct ether_header  *ehr = NULL;

struct stmt {
    bool   status;
    short  num_params;
};

static void __handle_exec_statement__(unsigned char *body, int pack_len, char *data);
static void __handle_long_data__(unsigned char *body, int pack_len, char *data);
static char* __handle_tcp_packet__(unsigned char* buffer, int size);
static char* __format_json_data__(char *query);
static void __handle_process_info__(unsigned char *body, int packet_len, char *data);
static void __handle_handshake_response__(unsigned char *buffer, char *data);
static void __handle_ok_packet__(unsigned char *buffer, char *data);
static void __handle_stmt_prepare__(unsigned char* buffer, int packet_len, char *data);

void 
hashtable_init(void)
{
    hashtable = malloc (sizeof (HashTable));
    hash_table_init(hashtable);
    ehr = (struct ether_header *)malloc(sizeof(struct ether_header));
}

char *
__format_json_data__(char *query)
{
    char *data = (char *)malloc(strlen(query) + BUFFER_SIZE); 
    char *tim = current_time();
    sprintf(data, "{\"time\": \"%s\","
                  "\"method\": %d,"
                  "\"s_mac\": \"%02x:%02x:%02x:%02x:%02x:%02x\","
                  "\"s_ip\": \"%s\", \"s_port\": \"%d\","
                  "\"d_mac\": \"%02x:%02x:%02x:%02x:%02x:%02x\","
                  "\"d_ip\": \"%s\", \"d_port\": \"%d\","
                  "\"query\": \"%s\"}",
                  tim, 
                  method,
                  ehr->ether_dhost[0],
                  ehr->ether_dhost[1],
                  ehr->ether_dhost[2],
                  ehr->ether_dhost[3],
                  ehr->ether_dhost[4],
                  ehr->ether_dhost[5],
                  source, s_port, 
                  ehr->ether_shost[0],
                  ehr->ether_shost[1],
                  ehr->ether_shost[2],
                  ehr->ether_shost[3],
                  ehr->ether_shost[4],
                  ehr->ether_shost[5],
                  dest, d_port,
                  query);
    free(tim);
    free(query);

    return data;
}

char*
mysql_process_packet(unsigned char* buffer, int size)
{
    char                 *data = NULL;

    memcpy(ehr, buffer, sizeof(struct ether_header));
    buffer += ETH_HLEN;     /*数据链路层*/

    struct iphdr *iph = (struct iphdr*)buffer;
    switch (iph->protocol) {

    case IPPROTO_ICMP:
    case IPPROTO_IGMP:
        break;
    case IPPROTO_TCP:
        data = __handle_tcp_packet__(buffer, size-ETH_HLEN);
        break;
    case IPPROTO_UDP:
        break;
    default:
        break;
    }
    if (data) {
    	return  __format_json_data__(data);
    }
    return NULL;
}

void
handle_prepare_response_pack(unsigned char* buffer, int size, struct stmt *s)
{
    struct stmt_prepare_response {
        char   status;
        int    statement_id;
        short  num_columns;
        short  num_params;
        char   reserved_1;
        short  warning_count;
    } __attribute__((packed));

    struct stmt_prepare_response* res = (struct stmt_prepare_response*)buffer;
    s->num_params = res->num_params;
}

void*
__is_stmt_repsonse_pack__(void)
{
    char  stream[BUFFER_SIZE];

    sprintf(stream, "%s%d%s%d", dest, d_port, source, s_port);
    HashNode *node = hash_table_lookup (hashtable, stream);
    if (node) {
        struct stmt* ret = (struct stmt*)(zVoidValue(node));
        return ret->status == true? ret: NULL;
    }
    return NULL;
}


char *
__handle_tcp_packet__(unsigned char* buffer, int size)
{
    struct tcphdr    *tcph;
    struct iphdr     *iph;
    unsigned short   iphdrlen;
    unsigned char    *body, mysql_body[BUFFER_SIZE] = {'\0'};
    int              packet, packet_len, packet_num;
    static char      last_is_pare = 0;
    unsigned char    *body_protol_start_ptr;
    char             *data = NULL;
    char              stream[BUFFER_SIZE];

    iph    = (struct iphdr*)buffer;
    iphdrlen = iph->ihl*4;

    tcph = (struct tcphdr*)(buffer + iphdrlen);
    long2ip(ntohl(iph->saddr), source);
    long2ip(ntohl(iph->daddr), dest);
    //if (!strcmp(source, "127.0.0.1")) {   /*本地连接*/
    //    return NULL;
    //}
    tcph = (struct tcphdr*)(buffer + iphdrlen);
    s_port = ntohs(tcph->source);
    d_port = ntohs(tcph->dest);
    body = buffer+iphdrlen+tcph->doff*4;
    if (size == iphdrlen+tcph->doff*4) {            /*it's a TCP packet*/
        return NULL;
    }
    if (s_port == port || d_port == port) {
        struct stmt      *s;
        if ((s = __is_stmt_repsonse_pack__()) != NULL && s_port == port) {   
            handle_prepare_response_pack(buffer+iphdrlen+tcph->doff*4+4, packet_len, s);
            return NULL;
        }
        packet = *(int*)body;
        packet_len = PACK_LEN(packet);
        packet_num = PACK_NUM(packet);
        if (packet_len <= 0 || packet_len > PKTBUFSIZE 
            || packet_len != (size-iphdrlen-tcph->doff*4-4)) { /* 不是Mysql协议包 */
            return NULL; 
        }
        data = (char *)malloc(packet_len+BUFFER_SIZE);
        //printf("packet: %d\tdata: %d\n", packet_len, size - iphdrlen - tcph->doff*4 - 4);
        body_protol_start_ptr = buffer+iphdrlen+tcph->doff*4+MYSQL_CTOS_PROTOCOL_SIZE;
        enum enum_server_command s_cmd = *(body + sizeof(int));
        switch (s_cmd) {

        case 0xFF: 
            method = 1;
            sprintf(data, "query unknown error");
            break;
        case COM_SLEEP: 
        case 0XFC:
        case 0XFD:
        case 0xFE:
            method = 1;
            __handle_ok_packet__(body_protol_start_ptr-1, data);
            break;
        case COM_QUIT:
            method = 0;
            if (packet_len == 1 && packet_num == 0) {
                sprintf(data, "%s", "mysql close");
            } else {
                sprintf(data, "%s", "unparse protocol");
            }
            break;
        case COM_INIT_DB:
            method = 0;
            strncpy (mysql_body, body_protol_start_ptr, packet_len-1);
            sprintf(data, "mysql select db: %s", mysql_body);
            break;
        case COM_QUERY:
            method = 0;
            strncpy (mysql_body, body_protol_start_ptr, packet_len-1);
            sprintf(data, "mysql query: %s", mysql_body);
            break;
        case COM_CREATE_DB:
            method = 0;
            strncpy (mysql_body, body_protol_start_ptr, packet_len-1);
            sprintf(data, "mysql create db: %s", mysql_body);
            break;
        case COM_DROP_DB:
            method = 0;
            strncpy (mysql_body, body_protol_start_ptr, packet_len-1);
            sprintf(data, "mysql delete db: %s", mysql_body);
            break;
        case COM_STMT_PREPARE:
            method = 0;
            __handle_stmt_prepare__(body_protol_start_ptr, packet_len-1, data);
            break;
        case COM_FIELD_LIST:
            method = 0;
            strncpy (mysql_body, body_protol_start_ptr, packet_len-1);
            sprintf(data, "mysql field list : %s", mysql_body);
            break;
        case COM_REFRESH:
        case COM_SHUTDOWN:
        case COM_STATISTICS:
            method = 0;
            sprintf(data, "%s", "unparse protocol");
            break;
        case COM_PROCESS_INFO:
            method = 0;
            __handle_process_info__(body, packet_len-1, data);
            break;
        case COM_CONNECT:
            method = 0;
            sprintf(data, "%s", "unparse protocol");
            break;
        case COM_PROCESS_KILL:
            method = 0;
            sprintf(data, "%s", "unparse protocol");
            break;
        case COM_DEBUG:
            method = 0;
            sprintf(data, "%s", "mysql debug");
            break;
        case COM_PING:
            method = 0;
            sprintf(data, "%s", "mysql ping");
            break;
        case COM_TIME:
        case COM_DELAYED_INSERT:
        case COM_CHANGE_USER:
        case COM_BINLOG_DUMP:
        case COM_TABLE_DUMP:
        case COM_CONNECT_OUT:
        case COM_REGISTER_SLAVE:
            method = 0;
            sprintf(data, "%s", "mysql processlist");
            break;
        case COM_STMT_EXECUTE:
            method = 0;
            __handle_exec_statement__(body_protol_start_ptr, packet_len-1, data);
            last_is_pare = 0;
            break;
        case COM_STMT_SEND_LONG_DATA:
            method = 0;
            __handle_long_data__(body_protol_start_ptr, packet_len-1, data);
            break;
        case COM_STMT_CLOSE:
            method = 0;
            sprintf(data, "mysql close statement: statement id = %d",
                    *(int*)body_protol_start_ptr);
            break;
        case COM_STMT_RESET:
            method = 0;
            sprintf(data, "mysql reset statement: statement id = %d",
                    *(int*)body_protol_start_ptr);
            break;
        case COM_SET_OPTION:
            method = 0;
            sprintf(data, "mysql set option: multi_statement(%s)",
                    *(short*)body_protol_start_ptr? "OFF": "ON");
            break;
        case COM_STMT_FETCH:
        case COM_DAEMON:
        case COM_END:
            method = 0;
            sprintf(data, "%s", "mysql processlist");
            break;
        default:
            method = 1;
            __handle_handshake_response__(body_protol_start_ptr-1, data);
            break;
        }

        if (!strcmp(data, "unparse protocol")) {
            free(data);
            return NULL;
        }
    }
    return data;
}

void
__handle_stmt_prepare__(unsigned char* buffer, int packet_len, char *data)
{
    char  mysql_body[BUFFER_SIZE];
    char  stream[BUFFER_SIZE];

    sprintf(stream, "%s%d%s%d", source, s_port, dest, d_port);
    struct stmt *s = (struct stmt*)malloc(sizeof(struct stmt));
    s->status = true;
    hash_table_insert_struct(hashtable, stream, s);
    strncpy (mysql_body, buffer, packet_len);
    sprintf(data, "mysql prepare stattement : %s", mysql_body);
}

/**
 * https://dev.mysql.com/doc/internals/en/packet-OK_Packet.html
 */
void
__handle_ok_packet__(unsigned char *buffer, char *data)
{
    int lenenc;
    char header = buffer[0];
    if (header  < 0xfb) {
        lenenc = 1;
    } else if (header == 0xfc) {
        lenenc = 2;
    } else if (header == 0xfd) {
        lenenc = 3;
    } else if (header == 0xfe) {
        lenenc = 8;
    }
    
    long *affected_rows = (long *)calloc(1, sizeof(long));
    long *last_insert_id = (long *)calloc(1, sizeof(long));
    int i;
    buffer += 1;
    for (i = 0; i < lenenc; i++){
        *((char *)affected_rows + i) = buffer[i];
    }
    buffer += lenenc;
    for (i = 0; i < lenenc; i++){
        *((char *)last_insert_id + i) = buffer[i];
    }

    sprintf(data, "affected_rows: %ld, last_insert_id: %ld", *affected_rows, *last_insert_id);
    free(affected_rows);
    free(last_insert_id);
}

/**
 * 4              capability flags, CLIENT_PROTOCOL_41 always set
   4              max-packet size
   1              character set
   string[23]     reserved (all [0])
   string[NUL]    username
     if capabilities & CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA {
   lenenc-int     length of auth-response
   string[n]      auth-response
     } else if capabilities & CLIENT_SECURE_CONNECTION {
   1              length of auth-response
   string[n]      auth-response
     } else {
   string[NUL]    auth-response
     }
     if capabilities & CLIENT_CONNECT_WITH_DB {
   string[NUL]    database
     }
     if capabilities & CLIENT_PLUGIN_AUTH {
   string[NUL]    auth plugin name
     }
     if capabilities & CLIENT_CONNECT_ATTRS {
   lenenc-int     length of all key-values
   lenenc-str     key
   lenenc-str     value
      if-more data in 'length of all key-values', more keys and value pairs
  }
 */
void 
__handle_handshake_response__(unsigned char *buffer, char *data)
{
    struct handleshake_response {
        int   capability_flags;
        int   max_packet_size;
        char  character_set;
        char  reserved[23];
    } __attribute__((packed));
    char  username[BUFFER_SIZE];

    struct handleshake_response *h_res =  (struct handleshake_response*)(buffer);
    strcpy(username, buffer + sizeof(struct handleshake_response));
    sprintf(data, "username: %s", username);
}

/**
 * 1              [0a] protocol version
   string[NUL]    server version
   4              connection id
   string[8]      auth-plugin-data-part-1
   1              [00] filler
   2              capability flags (lower 2 bytes)
     if more data in the packet:
   1              character set
   2              status flags
   2              capability flags (upper 2 bytes)
     if capabilities & CLIENT_PLUGIN_AUTH {
   1              length of auth-plugin-data
     } else {
   1              [00]
     }
   string[10]     reserved (all [00])
     if capabilities & CLIENT_SECURE_CONNECTION {
   string[$len]   auth-plugin-data-part-2 ($len=MAX(13, length of auth-plugin-data - 8))
     if capabilities & CLIENT_PLUGIN_AUTH {
   string[NUL]    auth-plugin name
     }
 */
void __handle_process_info__(unsigned char *body, int pack_len, char *data)
{
    char               server_name[BUFFER_SIZE];
    char              *body_protol_start_ptr;
    struct handshake  *hs;

    struct base_data {
        int   connection_id;
        char  auth_plugin_data_part_1[8];
        char  filler_1;
        short capability_flag_1;
    } __attribute__((packed));

    struct more_data {
        char  character_set;
        short status_flags;
        short capability_flags_2;
    } __attribute__((packed));
   
    body_protol_start_ptr = body + MYSQL_CTOS_PROTOCOL_SIZE;
    strcpy(server_name, body_protol_start_ptr);
    struct base_data *b_data = (struct base_data*)(body_protol_start_ptr + CSTRINGLEN(server_name));
    if ((pack_len - CSTRINGLEN(server_name) - sizeof(struct base_data)) > 0) {
        struct more_data *m_data = (struct more_data*)(body_protol_start_ptr + CSTRINGLEN(server_name) + sizeof(struct base_data));
        capabilities = (m_data->capability_flags_2 << 16) | b_data->capability_flag_1;
        int auth_plugin_data_len;
        if (capabilities & CLIENT_PLUGIN_AUTH) {
            auth_plugin_data_len = *(body_protol_start_ptr + CSTRINGLEN(server_name) + sizeof(struct base_data) + sizeof(struct more_data));
            //printf("auth_len: %d\n", auth_plugin_data_len);
        } else {
            auth_plugin_data_len = 0x00;
        }
        if (auth_plugin_data_len) {
            int i;
            char reserved[10];
            memcpy(reserved, body_protol_start_ptr + CSTRINGLEN(server_name) + sizeof(struct base_data) + sizeof(struct more_data) + 1, 10);
            for(i = 0; i < 10; i++) {
                assert(reserved[i] == '\0');
            }
            if (capabilities & CLIENT_SECURE_CONNECTION) {
                int len =  MAX(13, auth_plugin_data_len - 8);
                char *auth_plugin_data_part_2 = (char *)malloc(len);
                memcpy(auth_plugin_data_part_2, body_protol_start_ptr + CSTRINGLEN(server_name) + sizeof(struct base_data) + sizeof(struct more_data) + 1 + 10, len);
                if (capabilities & CLIENT_PLUGIN_AUTH) {
                    char auth_plugin_name[BUFFER_SIZE];
                    strcpy(auth_plugin_name, body_protol_start_ptr + CSTRINGLEN(server_name) + sizeof(struct base_data) + sizeof(struct more_data) + 1 + 10 + len);
                    sprintf(data, "mysql processlist: server version[%s], auth_pulgin_name[%s], auth-plugin-data[%s%s]", server_name, auth_plugin_name, b_data->auth_plugin_data_part_1, auth_plugin_data_part_2);
                }
                free(auth_plugin_data_part_2);
            } 
        }
    }
}

/*
 * COM_STMT_SEND_LONG_DATA
 *   direction: client -> server
 *   response: none
 *
 *    payload:
 *      1              [18] COM_STMT_SEND_LONG_DATA
 *      4              statement-id
 *      2              param-id
 *      n              data
 */
void
__handle_long_data__(unsigned char *body, int pack_len, char *data)
{
    int    i;
    int    statement_id;
    short  param_id;

    statement_id = *(int*)body;
    param_id = *(short*)(body + 4);
    sprintf(data, "%s", "mysql send long data: ");
    for (i = 0; i < pack_len - 6; i++) {
        sprintf(data, "%s%02x ", data, (body + 6)[i]);
    }
}


/*
 * COM_STMT_EXECUTE
 * execute a prepared statement
 *
 * direction: client -> server
 * esponse: COM_STMT_EXECUTE Response
 *
 * payload:
 *     1              [17] COM_STMT_EXECUTE
 *     4              stmt-id
 *     1              flags
 *     4              iteration-count
 *     if num-params > 0:
 *     n              NULL-bitmap, length: (num-params+7)/8
 *     1              new-params-bound-flag
 *     if new-params-bound-flag == 1:
 *     n              type of each parameter, length: num-params * 2
 *     n              value of each parameter)]
 */

int
__lookup_stmt_num_params__(void)
{
    char  stream[BUFFER_SIZE];
    int   num_params = 0;

    sprintf(stream, "%s%d%s%d", source, s_port, dest, d_port);
    HashNode *node = hash_table_lookup (hashtable, stream);
    if (node) {
        struct stmt* s = (struct stmt*)(zVoidValue(node));
        num_params = s->num_params;
        hash_table_remove(hashtable, stream);   /*删除TCP链接STMT Session*/
    }

    return num_params;
}

void
__handle_exec_statement__(unsigned char *body, int pack_len, char *data)
{
    int             nullBit_len;
    char            new_params_bound_flag;
    int             type_param_len;
    unsigned char   param_type, is_signed;
    int             i, j;
    unsigned char*  param_value_ptr,  *param_type_ptr;
    char           *cursor;
    
    switch (body[4]) {

    case CURSOR_TYPE_NO_CURSOR:
        cursor = "CURSOR_TYPE_NO_CURSOR";
        break;
    case CURSOR_TYPE_READ_ONLY:
        cursor = "CURSOR_TYPE_READ_ONLY";
        break;
    case CURSOR_TYPE_FOR_UPDATE:
        cursor = "CURSOR_TYPE_FOR_UPDATE";
        break;
    case CURSOR_TYPE_SCROLLABLE:
        cursor = "CURSOR_TYPE_SCROLLABLE";
        break;
    default:
        break;
    }
    sprintf(data, "mysql execute statement: statement id = %d(%s) ", *(int *)body, cursor);
    /*The iteration-count is always 1.*/
    assert(*((int*)(body+5)) == 0x01);
    int num_params = __lookup_stmt_num_params__();
    if (num_params > 0){
        nullBit_len = (num_params + 7) / 8;
        new_params_bound_flag = (body+9+nullBit_len)[0];
        if (new_params_bound_flag == 1) {
            type_param_len = num_params*2;
            param_type_ptr = body+9+nullBit_len+1;
            param_value_ptr = param_type_ptr+type_param_len;
            for(i = 0; i < num_params; i++) {
                param_type = param_type_ptr[0];
                is_signed = param_type_ptr[1];
                if (is_signed) {
                    sprintf(data, "%s [%d] => (signed ", data,  i);
                }else {
                    sprintf(data, "%s [%d] => (unsigned ", data,  i);
                }
                switch(param_type) {

                case FIELD_TYPE_LONGLONG:
                    sprintf(data, "%s long value = %ld)", data, *(long *)param_value_ptr);
                    param_value_ptr += 8;
                    break;
                case FIELD_TYPE_VAR_STRING:
                    sprintf(data, "%s var_string = ", data);
                    strncat(data, param_value_ptr+1, *param_value_ptr);
                    param_value_ptr += *param_value_ptr + 1;
                    break;
                default:
                    fprintf(stderr, "num_params[%d], pack_len[%d], data, %s, unkonw type: %d\n", num_params, pack_len,data, param_type);
                    break;
                }
                param_type_ptr += 2;
            }
        } else {
            type_param_len = 0;
        }
    } else {
        nullBit_len = 0;
    }
}

