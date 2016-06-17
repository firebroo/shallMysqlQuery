#include "mysql_pack_shall.h"	


void 
process_packet(unsigned char* buffer)
{
    //Get the IP Header part of this packet
    struct iphdr *iph = (struct iphdr*)buffer;
    switch (iph->protocol) //Check the Protocol and do accordingly...
    {
        case 1:  //ICMP Protocol
        case 2:  //IGMP Protocol
            break;

        case 6:  //TCP Protocol
            handle_tcp_packet(buffer);
            break;

        case 17: //UDP Protocol
            break;

        default: //Some Other Protocol like ARP etc.
            //PrintData(buffer, size);
            break;
    }
}

void 
handle_tcp_packet(unsigned char* buffer)
{
    struct tcphdr   *tcph;
    struct iphdr    *iph;
    unsigned short  iphdrlen;
    char            *body, mysql_body[BUFFER_SIZE];
    int             packet, packet_len, packet_num;

    iph    = (struct iphdr*)buffer;
    iphdrlen = iph->ihl*4;

    tcph = (struct tcphdr*)(buffer + iphdrlen);
    if (ntohs (tcph->dest) == port && ntohs(tcph->source) != port) {
        body = buffer + iphdrlen + tcph->doff*4;
        packet = ((int*)body)[0];
        packet_len = PACK_LEN(packet);
        packet_num = PACK_NUM(packet);
        if (packet_len < 0 || packet_len > BUFFER_SIZE) {
            return;
        }
        memset(mysql_body, '\0', BUFFER_SIZE);
        strncpy (mysql_body, buffer + iphdrlen + tcph->doff*4 + MYSQL_CTOS_PROTOCOL_SIZE, packet_len - 1);
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
                printf("%-36s%s\n","mysql select db:", mysql_body);
                break;
            case COM_QUERY:
                printf("%-36s%s\n", "mysql query:", mysql_body);
                break;
            case COM_FIELD_LIST:
                break;
            case COM_CREATE_DB:
                printf("%-36s%s\n", "mysql create db:", mysql_body);
                break;
            case COM_DROP_DB:
                printf("%-36s%s\n","mysql delete db:", mysql_body);
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
            case COM_STMT_PREPARE:
                printf("%-36s%s\n","mysql prepare statement:", mysql_body);
                break;
            case COM_STMT_EXECUTE:
                handle_exec_statement(buffer + iphdrlen + tcph->doff*4 + MYSQL_CTOS_PROTOCOL_SIZE);
                break;
            case COM_STMT_SEND_LONG_DATA:
                break;
            case COM_STMT_CLOSE:
                printf("%-36s\n", "mysql close statement");
                break;
            case COM_STMT_RESET:
                break;
            case COM_SET_OPTION:
                printf("%-36s\n", "set option");
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

unsigned short
validate_port(char *p){
    int port = atoi(p);

    if(port <= 0 || port > 0xffff){
        printf("Invalid port %d\n", port);
        exit(-1);
    }

    /* return port number if its valid */
    return (unsigned short)port;
}

int 
check_argv(int argc, char *argv[]) {
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

void
handle_exec_statement(unsigned char *body) {
    printf("%-36s\n", "mysql execute statement");
    printf("statement id:\t%d\n", ((int *)body)[0]);
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
    assert( ((int *)(body + 5))[0] == 0x01);
}

int 
main(int argc, char *argv[])
{
    int             sockfd, data_size;
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
        /*Receive a packet*/
        data_size = recvfrom(sock_raw , buffer , BUFFER_SIZE , 0 , &saddr , &saddr_size);
        if(data_size < 0)
        {
            printf("Recvfrom error , failed to get packets\n");
            return 1;
        }
        if(data_size > 0) {
            if ((sockfd = fork()) < 0) {
                perror("fork");
                exit(-1);
            }
            if (sockfd == 0) {
                process_packet(buffer);
            }
            process_packet(buffer);
        }
    }

    close(sock_raw);
    printf("Finished");
    return 0;
}
