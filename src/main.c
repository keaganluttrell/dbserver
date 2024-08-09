#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <stdlib.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char* argv[]) {
	printf("Usage: %s -f <filename.db> \n", argv[0]);
	printf("\t -f                            -- (required) path to datbase file\n");
	printf("\t -n                            -- create new db file\n");
	printf("\t -l                            -- list employees\n");
	printf("\t -a '<name>,<address>,<hours>' -- add new employee\n");
	printf("\t -A <filename>                 -- add new employees from a file\n");
	printf("\t -r '<name>'                   -- remove employee by name\n");
	printf("\t -u '<name>,<hours(int)>'      -- update an employees hours by name\n");

}

int main(int argc, char* argv[]) {

	bool newFile = false;
	bool list = false;
	char* filepath = NULL;
	char* addstring = NULL;
	char* addfile = NULL;
	char* removename = NULL;
	char* updatestring = NULL;

	int db_fd = -1;

	struct dbheader_t* dbhdr = NULL;
	struct employee_t* employees = NULL;

	int c;

	while ((c = getopt(argc, argv, "a:A:f:r:u:nlh")) != -1) {
		switch (c) {
		case 'a':
			addstring = optarg;
			break;
		case 'A':
			addfile = optarg;
			break;
		case 'f':
			filepath = optarg;
			break;
		case 'r':
			removename = optarg;
			break;
		case 'u':
			updatestring = optarg;
			break;
		case 'n':
			newFile = true;
			break;
		case 'l':
			list = true;
			break;
		case 'h':
			print_usage(argv);
			return 0;
		case '?':
			printf("Unknown option -%c\n", c);
			break;
		default:
			return -1;
		}
	}

	if (filepath == NULL) {
		print_usage(argv);
		return 0;
	}

	if (newFile) {
		db_fd = create_db_file(filepath);
		if (db_fd == STATUS_ERROR) {
			printf("Unable to create database file\n");
			return STATUS_ERROR;
		}

		if (create_db_header(db_fd, &dbhdr) == STATUS_ERROR) {
			printf("Unable to create database header\n");
			return STATUS_ERROR;
		}
	}
	else {
		db_fd = open_db_file(filepath);
		if (db_fd == STATUS_ERROR) {
			printf("Unable to open database file\n");
			return STATUS_ERROR;
		}

		if (validate_db_header(db_fd, &dbhdr) == STATUS_ERROR) {
			printf("Unable to validate database header\n");
			return STATUS_ERROR;
		}
	}

	if (read_employees(db_fd, dbhdr, &employees) != STATUS_SUCCESS) {
		printf("Failed to read employees");
		return STATUS_ERROR;
	}

	if (addstring) {
		dbhdr->count++;
		employees = realloc(employees, dbhdr->count * (sizeof(struct employee_t)));
		add_employee(dbhdr, employees, addstring);
	}

	if (addfile) {
		if (add_employees_from_file(dbhdr, &employees, addfile) != STATUS_SUCCESS) {
			printf("Failed to add employees from file\n");
			return STATUS_ERROR;
		}
	}

	if (removename) {
		if (remove_employee_by_name(dbhdr, employees, removename) != STATUS_SUCCESS) {
			printf("Unable to remove employee\n");
			return STATUS_ERROR;
		}
		if (ftruncate(db_fd, dbhdr->filesize - sizeof(struct employee_t)) != 0) {
			printf("Failed to truncate file\n");
			return STATUS_ERROR;
		}
	}

	if (updatestring) {
		if (update_employee_hours_by_name(dbhdr, employees, updatestring) != STATUS_SUCCESS) {
			printf("Unable to update employee hours\n");
			return STATUS_ERROR;
		}
	}

	if (list) {
		list_employees(dbhdr, employees);
	}

	output_file(db_fd, dbhdr, employees);

	return STATUS_SUCCESS;
}
