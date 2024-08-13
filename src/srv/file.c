#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "file.h"
#include "common.h"


int create_db_file(char* filename) {
    int fd = open(filename, O_RDWR);
    if (fd != STATUS_ERROR) {
        close(fd);
        printf("File already exists\n");
        return STATUS_ERROR;
    }

    fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (fd == STATUS_ERROR) {
        perror("open");
        return STATUS_ERROR;
    }

    printf("file created\n");
    return fd;
}

int open_db_file(char* filename) {
    int fd = open(filename, O_RDWR, 0644);
    if (fd == STATUS_ERROR) {
        perror("open");
        return STATUS_ERROR;
    }

    return fd;
}

