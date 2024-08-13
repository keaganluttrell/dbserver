#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

int send_hello(int fd) {
    printf("send_hello fd: %d\n", fd);

    char buf[4096] = {0};

    dbproto_hdr_t *hdr = (dbproto_hdr_t *)buf;

    hdr->type = MSG_HELLO_REQ;
    hdr->len = 1;

    dbproto_hello_req *hello = (dbproto_hello_req *)&hdr[1];
    hello->proto = PROTO_VER;

    hdr->type = htonl(hdr->type);
    hdr->len = htons(hdr->len);
    hello->proto = htons(hello->proto);

    write(fd, buf, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_req));

    read(fd, buf, sizeof(buf));

    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);

    if (hdr->type == MSG_ERROR) {
        printf("Protocol mismatch.\n");
        close(fd);
        return STATUS_ERROR;
    }

    printf("Server connected, protocol v1.\n");

    return STATUS_SUCCESS;
}

int send_employee(int fd, char *e) {

    char buf[4096] = {0};
    dbproto_hdr_t *hdr = (dbproto_hdr_t *)buf;
    hdr->type = MSG_EMPLOYEE_ADD_REQ;
    hdr->len = 1;

    dbproto_employee_add_req *employee = (dbproto_employee_add_req *)&hdr[1];
    strncpy(&employee->data, e, sizeof(employee->data));

    hdr->type = htonl(hdr->type);
    hdr->len = htons(hdr->len);

    write(fd, buf, sizeof(dbproto_hdr_t) + sizeof(dbproto_employee_add_req));

    read(fd, buf, sizeof(buf));

    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);

    if (hdr->type == MSG_ERROR) {
        printf("Err: Bad format for employee string\n");
        close(fd);
        return STATUS_ERROR;
    }

    if (hdr->type == MSG_EMPLOYEE_ADD_RES) {
        printf("Employee added.\n");
    }

    return STATUS_SUCCESS;
}

int main(int argc, char *argv[]) {

    char *add = NULL;
    char *host = NULL;
    unsigned short port = 0;

    int c;
    while ((c = getopt(argc, argv, "a:h:p:")) != -1) {
        switch (c) {
        case 'a':
            add = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'h':
            host = optarg;
            break;
        case '?':
            printf("UKNWN  OPTION: -%c\n", c);
            break;
        defualt:
            return -1;
        }
    }

    if (port == 0) {
        printf("bad port arg: -p %s\n", optarg);
        return -1;
    }
    if (host == NULL) {
        printf("You must specify host with -h \n");
        return -1;
    }

    struct sockaddr_in serverinfo = {0};

    serverinfo.sin_family = AF_INET;
    serverinfo.sin_addr.s_addr = inet_addr(host);
    serverinfo.sin_port = htons(port);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&serverinfo, sizeof(serverinfo)) == -1) {
        perror("connect");
        close(fd);
        return 0;
    }

    if (send_hello(fd) != STATUS_SUCCESS) {
        close(fd);
        return STATUS_ERROR;
    }

    if (add) {
        send_employee(fd, add);
    }

    close(fd);
    return 0;
}
