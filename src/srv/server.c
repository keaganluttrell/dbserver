#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "parse.h"
#include "server.h"

void init_clients(client_state_t *states) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        states[i].fd = -1;
        states[i].state = STATE_NEW;
        memset(&states[i].buffer, '\0', BUFF_SZ);
    }
    return;
}

int find_first_free_slot(client_state_t *states) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (states[i].fd == -1) {
            return i;
        }
    }
    return -1;
}

int find_slot_by_fd(client_state_t *states, int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (states[i].fd == fd) {
            return i;
        }
    }
    return -1;
}

void fsm_reply_err(client_state_t *c, dbproto_hdr_t *hdr) {

    hdr->type = htonl(MSG_ERROR);
    hdr->len = htons(0);

    write(c->fd, hdr, sizeof(dbproto_hdr_t));
}

void fsm_reply_add(client_state_t *c, dbproto_hdr_t *hdr) {
    hdr->type = htonl(MSG_EMPLOYEE_ADD_RES);
    hdr->len = htons(0);

    write(c->fd, hdr, sizeof(dbproto_hdr_t));

    return;
}

void fsm_reply_hello(client_state_t *c, dbproto_hdr_t *hdr) {
    hdr->type = htonl(MSG_HELLO_RESP);
    hdr->len = htons(1);
    dbproto_hello_resp *hello = (dbproto_hello_resp *)&hdr[1];
    hello->proto = htons(PROTO_VER);

    write(c->fd, hdr, sizeof(dbproto_hdr_t) + sizeof(dbproto_hello_resp));
}

void send_employees(struct dbheader_t *dbhdr, struct employee_t **employeesptr,
                    client_state_t *c) {

    dbproto_hdr_t *hdr = (dbproto_hdr_t *)c->buffer;
    hdr->type = htonl(MSG_EMPLOYEE_LIST_RESP);
    hdr->len = htons(dbhdr->count);

    write(c->fd, hdr, sizeof(dbproto_hdr_t));

    dbproto_employee_list_resp *emp = (dbproto_employee_list_resp *)&hdr[1];

    struct employee_t *employees = *employeesptr;

    for (int i = 0; i < dbhdr->count; i++) {
        strncpy((char *)&emp->name, employees[i].name, sizeof(emp->name));
        strncpy((char *)&emp->address, employees[i].address,
                sizeof(emp->address));
        emp->hours = htonl(employees[i].hours);
        write(c->fd, emp, sizeof(dbproto_employee_list_resp));
    }

    return;
}

void fsm_del_reply(client_state_t *c, dbproto_hdr_t *hdr) {
    hdr->type = htonl(MSG_EMPLOYEE_DEL_RES);
    hdr->len = htons(0);

    write(c->fd, hdr, sizeof(dbproto_hdr_t));
    return;
}

int handle_client_fsm(struct dbheader_t *dbhdr, struct employee_t **employees,
                      client_state_t *c, int dbfd) {

    dbproto_hdr_t *hdr = (dbproto_hdr_t *)c->buffer;
    hdr->type = ntohl(hdr->type);
    hdr->len = ntohs(hdr->len);

    if (c->state == STATE_HELLO) {
        if (hdr->type != MSG_HELLO_REQ || hdr->len != 1) {
            printf("Didn't get MSG_HELLO in HELLO state\n");
            fsm_reply_err(c, hdr);
            return STATUS_ERROR;
        }

        dbproto_hello_req *hello = (dbproto_hello_req *)&hdr[1];
        hello->proto = ntohs(hello->proto);
        if (hello->proto != PROTO_VER) {
            printf("Proto version mismatch\n");
            fsm_reply_err(c, hdr);
            return STATUS_ERROR;
        }
        fsm_reply_hello(c, hdr);
        c->state = STATE_MSG;
        printf("client state updated to STATE_MSG\n");
    }

    if (c->state == STATE_MSG) {
        if (hdr->type == MSG_EMPLOYEE_ADD_REQ) {
            dbproto_employee_add_req *employee =
                (dbproto_employee_add_req *)&hdr[1];

            printf("Adding employee: %s\n", employee->data);

            if (add_employee(dbhdr, employees, (char *)employee->data) !=
                STATUS_SUCCESS) {
                fsm_reply_err(c, hdr);
                return STATUS_ERROR;
            } else {
                fsm_reply_add(c, hdr);
                output_file(dbfd, dbhdr, *employees);
            }
        }

        if (hdr->type == MSG_EMPLOYEE_LIST_REQ) {
            printf("Client: Listing Employees...\n");
            send_employees(dbhdr, employees, c);
        }

        if (hdr->type == MSG_EMPLOYEE_DEL_REQ) {
            dbproto_employee_del_req *employee =
                (dbproto_employee_del_req *)&hdr[1];
            printf("removing employee: %s\n", (char *)employee->name);

            if (remove_employee_by_name(dbhdr, *employees,
                                        (char *)employee->name) !=
                STATUS_SUCCESS) {
                printf("unable to remove employee: %s", employee->name);
                fsm_reply_err(c, hdr);
            } else {
                fsm_del_reply(c, hdr);
                output_file(dbfd, dbhdr, *employees);
            }
        }
    }

    return STATUS_SUCCESS;
}

client_state_t client_states[MAX_CLIENTS] = {0};

int open_select(unsigned short port, struct dbheader_t *dbhdr,
                struct employee_t *employees) {
    printf("hello from open_select on port: %d\n", port);
    return 0;
}

int open_poll(unsigned short port, struct dbheader_t *dbhdr,
              struct employee_t **employees, int dbfd) {
    int conn_fd, first_free_slot;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;
    int opt = 1;

    init_clients((client_state_t *)&client_states);

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, 10) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server using Poll listening on port: %d\n", port);

    memset(fds, 0, sizeof(fds));
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds = 1;

    while (1) {
        int j = 1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_states[i].fd != -1) {
                fds[j].fd = client_states[i].fd;
                fds[j].events = POLLIN;
                j++;
            }
        }

        int n_events = poll(fds, nfds, -1); // -1 means no timeout
        if (n_events == -1) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        if (fds[0].revents & POLLIN) {
            conn_fd =
                accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
            if (conn_fd == -1) {
                perror("accept");
                continue;
            }

            printf("New Connection from %s:%d\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));

            first_free_slot =
                find_first_free_slot((client_state_t *)&client_states);
            if (first_free_slot == -1) {
                printf("Server full: closing new connections\n");
                close(conn_fd);
            } else {
                client_states[first_free_slot].fd = conn_fd;
                client_states[first_free_slot].state = STATE_HELLO;
                nfds++;
            }

            n_events--;
        }

        for (int i = 1; i <= nfds && n_events > 0; i++) {
            if (fds[i].revents & POLLIN) {
                n_events--;

                int fd = fds[i].fd;
                int slot =
                    find_slot_by_fd((client_state_t *)&client_states, fd);
                ssize_t bytes_read = read(fd, &client_states[slot].buffer,
                                          sizeof(client_states[slot].buffer));
                if (bytes_read <= 0) {
                    close(fd);
                    if (slot != -1) {
                        client_states[slot].fd = -1;
                        client_states[slot].state = STATE_DISCONNECTED;
                        printf("Client disconnected\n");
                        nfds--;
                    }
                } else {
                    handle_client_fsm(dbhdr, employees, &client_states[slot],
                                      dbfd);
                }
            }
        }
    }
}
