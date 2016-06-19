#include "mysql_pack_shall.h"

/*
 * mysql protocol
 * read from http://dev.mysql.com/doc/internals/en/client-server-protocol.html
 * */

static short num_of_param = -1;

void
process_packet(unsigned char* buffer)
{
    //Get the IP Header part of this packet
    struct iphdr *iph = (struct iphdr*)buffer;
    switch (iph->protocol) //Check the Protocol and do accordingly...
    {
        case IPPROTO_ICMP:  //ICMP Protocol
        case IPPROTO_IGMP:  //IGMP Protocol
            break;

        case IPPROTO_TCP:  //TCP Protocol
            handle_tcp_packet(buffer);
            break;

        case IPPROTO_UDP: //UDP Protocol
            break;

        default: //Some Other Protocol like ARP etc.
            //PrintData(buffer, size);
            break;
    }
}

short big2littles(short num)
{
    return ((num & 0xFF00) >> 8)
         | ((num & 0x00FF) << 8);
}

int big2littlei(int num)
{
    return ((num & 0xFF000000) >> 24)
         | ((num & 0x000000FF) << 24)
         | ((num & 0x00FF0000) >> 8)
         | ((num & 0x0000FF00) << 8);
}

long big2littlel(long num)
{
    return ((num & 0xFF00000000000000) >> 56)
         | ((num & 0x00000000000000FF) << 56)
         | ((num & 0x00FF000000000000) >> 40)
         | ((num & 0x000000000000FF00) << 40)
         | ((num & 0x0000FF0000000000) >> 24)
         | ((num & 0x0000000000FF0000) << 24)
         | ((num & 0x000000FF00000000) >> 8)
         | ((num & 0x00000000FF000000) << 8);
}

char *
revstr(char *str, size_t len)
{
    char    *start = str;
    char    *end = str + len - 1;
    char    ch;

    if (str != NULL){
        while (start < end){
            ch = *start;
            *start++ = *end;
            *end-- = ch;
        }
    }
    return str;
}

void
handle_prepare_repose_pack(unsigned char* buffer)
{
    int     packet_len, packet_num, packet, statement_id;
    short   num_of_fileds;

    packet = ((int*)buffer)[0];
    packet_len = PACK_LEN(packet);
    packet_num = PACK_NUM(packet);

    statement_id = ((int*)(buffer+5))[0];


    num_of_fileds = ((short*)(buffer+9))[0];
    num_of_param = ((short*)(buffer+9))[1];

    /*
    printf("packet len:%d\n"
            "packet num: %d\n"
            "statement_id: %d\n"
            "num_of_fileds: %d\n"
            "num_of_param: %d\n",
            packet_len,
            packet_num,
            statement_id,
            num_of_fileds,
            num_of_param);
            */
}


void
handle_tcp_packet(unsigned char* buffer)
{
    struct tcphdr   *tcph;
    struct iphdr    *iph;
    unsigned short  iphdrlen;
    unsigned char   *body, mysql_body[BUFFER_SIZE];
    int             packet, packet_len, packet_num;
    static char     last_is_pare = 0;
    unsigned char   *body_protol_start_ptr;

    iph    = (struct iphdr*)buffer;
    iphdrlen = iph->ihl*4;

    tcph = (struct tcphdr*)(buffer + iphdrlen);
    if (last_is_pare == 1 && ntohs(tcph->source) == 3306) {
        handle_prepare_repose_pack(buffer + iphdrlen + tcph->doff*4);
        last_is_pare = 0;
    }
    if (ntohs (tcph->dest) == port && ntohs(tcph->source) != port) {
        body = buffer + iphdrlen + tcph->doff*4;
        packet = ((int*)body)[0];
        packet_len = PACK_LEN(packet);
        packet_num = PACK_NUM(packet);
        if (packet_len < 0 || packet_len > BUFFER_SIZE) {
            return;
        }
        memset(mysql_body,'\0', BUFFER_SIZE);

        body_protol_start_ptr = buffer + iphdrlen + tcph->doff*4 + MYSQL_CTOS_PROTOCOL_SIZE;
        //printf("%d\n", packet_len);
        switch (body[4]) {
            case COM_SLEEP:
                break;
            case COM_QUIT:
                if (packet_len == 1 && packet_num == 0) {
                    printf("%-36s\n","mysql close");
                }
                break;
            case COM_INIT_DB:
                strncpy (mysql_body, body_protol_start_ptr, packet_len - 1);
                printf("%-36s%s\n","mysql select db:", mysql_body);
                break;
            case COM_QUERY:
                strncpy (mysql_body, body_protol_start_ptr, packet_len - 1);
                printf("%-36s%s\n", "mysql query:", mysql_body);
                break;
            case COM_CREATE_DB:
                strncpy (mysql_body, body_protol_start_ptr, packet_len - 1);
                printf("%-36s%s\n", "mysql create db:", mysql_body);
                break;
            case COM_DROP_DB:
                strncpy (mysql_body, body_protol_start_ptr, packet_len - 1);
                printf("%-36s%s","mysql delete db:", mysql_body);
                break;
            case COM_STMT_PREPARE:
                last_is_pare = 1;
                printf("%-36s","mysql prepare statement:");
                strncpy (mysql_body, body_protol_start_ptr, packet_len - 1);
                printf("%s\n", mysql_body);
                break;
            case COM_FIELD_LIST:
                break;
            case COM_REFRESH:
                break;
            case COM_SHUTDOWN:
                break;
            case COM_STATISTICS:
                break;
            case COM_PROCESS_INFO:
                printf("%-36s\n", "mysql processlist");
                break;
            case COM_CONNECT:
                break;
            case COM_PROCESS_KILL:
                break;
            case COM_DEBUG:
                printf("%-36s\n", "mysql debug");
                break;
            case COM_PING:
                printf("%-36s\n", "mysql ping");
                break;
            case COM_TIME:
                break;
            case COM_DELAYED_INSERT:
                break;
            case COM_CHANGE_USER:
                break;
            case COM_BINLOG_DUMP:
                break;
            case COM_TABLE_DUMP:
                break;
            case COM_CONNECT_OUT:
                break;
            case COM_REGISTER_SLAVE:
                break;
            case COM_STMT_EXECUTE:
                handle_exec_statement(body_protol_start_ptr, packet_len - 1);
                break;
            case COM_STMT_SEND_LONG_DATA:
                handle_long_data(body_protol_start_ptr, packet_len - 1);
                break;
            case COM_STMT_CLOSE:
                printf("%-36sstatement id = %d\n",
                        "mysql close statement:",
                        ((int*)(body_protol_start_ptr))[0]);
                break;
            case COM_STMT_RESET:
                printf("%-36sstatement id = %d\n",
                        "mysql reset statement:",
                        ((int*)(body_protol_start_ptr))[0]);
                break;
            case COM_SET_OPTION:
                printf("%-36smulti_statement(%s)\n",
                        "mysql set option: ",
                        ((short*)body_protol_start_ptr)[0]? "OFF": "ON");
                break;
            case COM_STMT_FETCH:
                break;
            case COM_DAEMON:
                break;
            case COM_END:
                break;
            default:
                //printf("unkonw command\n");
                break;
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
handle_long_data(unsigned char *body, int pack_len)
{
    int    i;
    int    statement_id;
    short  param_id;

    statement_id = ((int*)body)[0];
    param_id = ((short*)(body + 4))[0];
    for (i = 0; i < pack_len - 6; i++) {
        printf("%02x ", (body + 6)[i]);
    }
    printf("%-36s", "mysql send long data:");
}

unsigned short
validate_port(char *p)
{
    int     port = atoi(p);

    if(port <= 0 || port > 0xffff){
        printf("Invalid port %d\n", port);
        exit(-1);
    }

    /* return port number if its valid */
    return (unsigned short)port;
}

int
check_argv(int argc, char *argv[])
{
    int    opt;

    if (argc > 2) {
        printf("Usage: ./mysql_pack_shall [-p port]\n");
        return 0;
    }
    while ( (opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                port = validate_port(optarg);
                break;
            default:
                printf("Usage: ./mysql_pack_shall [-p port]\n");
                return 0;
        }
    }
    return 1;
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

void
handle_exec_statement(unsigned char *body, int pack_len)
{
    int             nullBit_len;
    char            new_params_bound_flag;
    int             type_param_len;
    unsigned char   param_type, is_signed;
    int             i, j;
    unsigned char*  param_value_ptr,  *param_type_ptr;
    int             all_param_len;
    char            buffer_reserve[BUFFER_SIZE], *buffer_reserve_ptr;
    unsigned char   *param_type_last_ptr;
    char            string_buffer[BUFFER_SIZE], *string;
    int             counter = 0;

    printf("%-36s", "mysql execute statement:");
    printf("statement id = %d\t", ((int *)body)[0]);
    switch (body[4]) {
        case CURSOR_TYPE_NO_CURSOR:
            printf("CURSOR_TYPE_NO_CURSOR\n");
            break;
        case CURSOR_TYPE_READ_ONLY:
            printf("CURSOR_TYPE_READ_ONLY\n");
            break;
        case CURSOR_TYPE_FOR_UPDATE:
            printf("CURSOR_TYPE_FOR_UPDATE\n");
            break;
        case CURSOR_TYPE_SCROLLABLE:
            printf("CURSOR_TYPE_SCROLLABLE\n");
            break;
        default:
            break;
    }
    /*The iteration-count is always 1.*/
    assert( ((int *)(body + 5))[0] == 0x01);
    if (num_of_param > 0){
        nullBit_len = (num_of_param + 7) / 8;
    }
    else {
        nullBit_len = 0;
    }
    new_params_bound_flag = (body + 9 + nullBit_len)[0];
    if (new_params_bound_flag == 1) {
        type_param_len = num_of_param * 2;
    }else {
        type_param_len = 0;
    }

    param_type_ptr = body + 9 + nullBit_len + 1;
    param_value_ptr = param_type_ptr + type_param_len;
    param_type_last_ptr = param_value_ptr - 2;

    all_param_len = pack_len - (9 + nullBit_len + 1 + type_param_len);

    for(j = 1; j <= all_param_len; j++) {
        buffer_reserve[all_param_len - j] = *(param_value_ptr++);
    }
    buffer_reserve_ptr = buffer_reserve;
    /*reverse read param */
    for(i = 1; i <= num_of_param; i++) {
        param_type = (param_type_last_ptr)[0];
        is_signed = (param_type_last_ptr)[1];
        if (is_signed) {
            printf("%-36s[%d] => (signed ", "",  num_of_param-i);
        }else {
            printf("%-36s[%d] => (unsigned ", "",  num_of_param-i);
        }
        switch(param_type) {
            case FIELD_TYPE_LONGLONG:
                printf("long value = %ld)\n", big2littlel((((long *)buffer_reserve_ptr)[0])));
                buffer_reserve_ptr += 8;
                break;
            case FIELD_TYPE_VAR_STRING:
                printf("var_string = ");
                while (*buffer_reserve_ptr != ETX) {
                    //another end of text;
                    if (*buffer_reserve_ptr == 0x08) {
                        break;
                    }
                    string_buffer[counter++] = *(buffer_reserve_ptr++);
                }
                string_buffer[counter] = '\0';
                string = revstr(string_buffer, strlen(string_buffer));
                printf("%s)\n", string);
                buffer_reserve_ptr++;
                break;
            default:
                printf("unkonw type: %d\n", param_type);
        }
        param_type_last_ptr -= 2;
    }

    num_of_param = -1;
}

int
main(int argc, char *argv[])
{
    int             data_size;
    unsigned int    saddr_size;
    struct sockaddr saddr;
    unsigned char   *buffer;

    if (!check_argv(argc, argv)) {
        exit(-1);
    }

    buffer = (unsigned char *)malloc(BUFFER_SIZE); //Its Big!

    /*Create a raw socket that shall sniff*/
    sock_raw = socket(AF_INET , SOCK_RAW , IPPROTO_TCP);
    if(sock_raw < 0)
    {
        printf("Socket Error\n");
        return 1;
    }

    saddr_size = sizeof(saddr);
    for( ; ; )
    {
        memset(buffer, '\0', BUFFER_SIZE);

        /*Receive a packet*/
        data_size = recvfrom(sock_raw , buffer , BUFFER_SIZE , 0 , &saddr , &saddr_size);
        if(data_size < 0)
        {
            printf("Recvfrom error , failed to get packets\n");
            return 1;
        }
        if(data_size > 0) {
            //if ((sockfd = fork()) < 0) {
            //    perror("fork");
            //    exit(-1);
            //}
            //if (sockfd == 0) {
            //    process_packet(buffer);
            //}
            process_packet(buffer);
        }
    }

    close(sock_raw);
    printf("Finished");
    return 0;
}
