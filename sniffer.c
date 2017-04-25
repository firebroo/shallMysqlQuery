#include "config.h"
#include "common.h"
#include "mysql_parse.h"

unsigned short  port = DEA_PORT;

int
check_argv(int argc, char *argv[])
{
    int    opt;

    if (argc > 2) {
        printf("Usage: ./sniffer [-p port]\n");
        return 0;
    }
    while ( (opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                port = validate_port(optarg);
                break;
            default:
                printf("Usage: ./sniffer [-p port]\n");
                return 0;
        }
    }
    return 1;
}

int
main(int argc, char *argv[])
{
    int             sock_raw;
    int             data_size;
    unsigned int    saddr_size;
    struct sockaddr saddr;
    unsigned char   *buffer;

    if (!check_argv(argc, argv)) {
        return -1;
    }

    buffer = (unsigned char *)malloc(PKTBUFSIZE);

    /*接受所有的ip数据帧*/
    sock_raw = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP));
    if(sock_raw < 0) {
        printf("Socket Error\n");
        return -1;
    }

    saddr_size = sizeof(saddr);
    hashtable_init();
    for( ; ; ) {
        memset(buffer, '\0', BUFFER_SIZE);

        /*Receive a packet*/
        data_size = recvfrom(sock_raw, buffer, BUFFER_SIZE, 0 , &saddr, &saddr_size);
        if(data_size < 0)
        {
            printf("Recvfrom error , failed to get packets\n");
            return -1;
        }
        if(data_size > 0) {
            char *data = mysql_process_packet(buffer, data_size);
            if (data != NULL) {
                printf("%s\n", data);
                free(data);
            }
        }
    }

    close(sock_raw);
    printf("Finished");
    return 0;
}
