#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "parse.h"

int update_employee_hours_by_name(struct dbheader_t* dbhdr, struct employee_t* employees, char* updatestring) {
    char* name = strtok(updatestring, ",");
    char* hours_str = strtok(NULL, ",");

    if (name == NULL || hours_str == NULL) {
        printf("Invalid update string format\n");
        return STATUS_ERROR;
    }

    for (int i = 0; i < dbhdr->count; i++) {
        if (strcmp(name, employees[i].name) == 0) {
            employees[i].hours = atoi(hours_str);
            return STATUS_SUCCESS;
        }
    }

    printf("EMPLOYEE: %s Not Found\n", name);
    return STATUS_ERROR;
}

int remove_employee_by_name(struct dbheader_t* dbhdr, struct employee_t* employees, char* name) {
    int index = -1;
    for (int i = 0; i < dbhdr->count; i++) {
        if (strcmp(name, employees[i].name) == 0) {
            index = i;
        }
        if (index != -1 && index != dbhdr->count - 1) {
            employees[i] = employees[i + 1];
        }
    }

    if (index == -1) {
        printf("EMPLOYEE: %s Not Found\n", name);
        return STATUS_ERROR;
    }

    dbhdr->count--;
    struct employee_t* temp = realloc(employees, dbhdr->count * sizeof(struct employee_t));
    if (temp == NULL && dbhdr->count > 0) {
        perror("Memory reallocation failed");
        return STATUS_ERROR;
    }

    employees = temp;

    return STATUS_SUCCESS;
}

int add_employees_from_file(struct dbheader_t* dbhdr, struct employee_t** employees, char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (file == NULL) {
        perror("Could not open file to add employees");
        return STATUS_ERROR;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        dbhdr->count++;
        struct employee_t* temp = realloc(*employees, dbhdr->count * sizeof(struct employee_t));
        if (temp == NULL) {
            perror("Memory allocation failed");
            fclose(file);
            return STATUS_ERROR;
        }
        *employees = temp;
        add_employee(dbhdr, *employees, line);
    }

    fclose(file);
    return STATUS_SUCCESS;
}

void list_employees(struct dbheader_t* dbhdr, struct employee_t* employees) {

    if (dbhdr->count == 0) {
        printf("no employees in database\n");
        return;
    }

    for (int i = 0; i < dbhdr->count; i++) {
        printf("EMPLOYEE %d\n", i);
        printf("\tNAME : %s\n", employees[i].name);
        printf("\tHOURS: %d\n", employees[i].hours);
        printf("\tADDR : %s\n", employees[i].address);
    }

    return;
}

int add_employee(struct dbheader_t* dbhdr, struct employee_t* employees, char* addstring) {

    char* name = strtok(addstring, ",");
    char* addr = strtok(NULL, ",");
    char* hours = strtok(NULL, ",");

    struct employee_t* e = &employees[dbhdr->count - 1];

    strncpy(e->name, name, sizeof(e->name));
    strncpy(e->address, addr, sizeof(e->address));
    e->hours = atoi(hours);

    return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t* dbhdr, struct employee_t** employeesOut) {
    if (fd < 0) {
        printf("Bad file descriptor\n");
        return STATUS_ERROR;
    }

    int count = dbhdr->count;

    struct employee_t* employees = calloc(count, sizeof(struct employee_t));
    if (employees == NULL) {
        printf("Malloc failed\n");
        return STATUS_ERROR;
    }

    read(fd, employees, count * sizeof(struct employee_t));

    for (int i = 0; i < count; i++) {
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;

    return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t* dbhdr, struct employee_t* employees) {
    if (fd < 0) {
        printf("Bad file descriptor\n");
        return STATUS_ERROR;
    }

    int realcount = dbhdr->count;

    dbhdr->magic = htonl(dbhdr->magic);
    dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount));
    dbhdr->count = htons(dbhdr->count);
    dbhdr->version = htons(dbhdr->version);

    if (lseek(fd, 0, SEEK_SET) == STATUS_ERROR) {
        perror("lseek");
        return STATUS_ERROR;
    }

    write(fd, dbhdr, sizeof(struct dbheader_t));

    for (int i = 0; i < realcount; i++) {
        employees[i].hours = htonl(employees[i].hours);
        write(fd, &employees[i], sizeof(struct employee_t));
    }

    return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t** headerOut) {
    if (fd < 0) {
        printf("Bad file descriptor\n");
        return STATUS_ERROR;
    }
    struct dbheader_t* header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        printf("Malloc failed to validate db header\n");
        return STATUS_ERROR;
    }

    if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
        perror("read");
        free(header);
        return STATUS_ERROR;
    }

    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->magic = ntohl(header->magic);
    header->filesize = ntohl(header->filesize);

    if (header->magic != HEADER_MAGIC) {
        printf("Header file is not parseable\n");
        free(header);
        return STATUS_ERROR;
    }

    if (header->version != 1) {
        printf("Header Version is incorrect");
    }

    struct stat dbstat = { 0 };
    if (fstat(fd, &dbstat) == STATUS_ERROR) {
        perror("fstat");
        free(header);
        return STATUS_ERROR;
    }

    if (header->filesize != dbstat.st_size) {
        printf("Corrupted database file\n");
        free(header);
        return STATUS_ERROR;
    }

    *headerOut = header;

    return STATUS_SUCCESS;
}

int create_db_header(int fd, struct dbheader_t** headerOut) {
    struct dbheader_t* header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        printf("Malloc failed to create db header\n");
        return STATUS_ERROR;
    }

    header->version = 0x1;
    header->count = 0;
    header->magic = HEADER_MAGIC;
    header->filesize = sizeof(struct dbheader_t);

    *headerOut = header;

    return STATUS_SUCCESS;
}
