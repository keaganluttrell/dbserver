#ifndef SERVER_H
#define SERVER_H
#define MAX_CLIENTS 256
#define BUFF_SZ 4096

#include "parse.h"

typedef enum {
    STATE_NEW,
    STATE_CONNECTED,
    STATE_DISCONNECTED,
    STATE_HELLO,
    STATE_MSG,
    STATE_GOODBYE,
} state_e;

typedef struct {
    int fd;
    state_e state;
    char buffer[BUFF_SZ];
} client_state_t;

int open_select(unsigned short port, struct dbheader_t *dbhdr,
                struct employee_t *employees);
int open_poll(unsigned short port, struct dbheader_t *dbhdr,
              struct employee_t **employees, int dbfd);
int handle_client_fsm(struct dbheader_t *dbhdr, struct employee_t **employees,
                      client_state_t *c, int dbfd);
#endif
