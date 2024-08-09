# Basic File DB

This utility is just a simple file based database implementation. It does base crud functionality for adding an employee.

This is an introductory app to get my feet wet in C programming. More to come!

This program is based from the low level academy [c programming course](https://lowlevel.academy/courses/zero2hero)

## Quick Start

### Run the build tool
```sh
make
```

### Test
```sh
make test
```

## Commands

`-f`: filename is required

```sh
./bin/dbview -f file.db
```

`-n`: flag to create new database file from `-f` argument

```sh
./bin/dbview -f file.db -n
```
`-l`: flag will list the employees in the database

```sh
./bin/dbview -f file.db -l
```

`-a`: adds employee entry to database

```sh
./bin/dbview -f file.db -a "name, address, hours(int)"
```

`-A`: adds a file with a list of employee entries

```sh
./bin/dbview -f file.db -A manifest.txt
```

`-u`: updates an employee's hours by name

```sh
./bin/dbview -f file.db -u "name,newhours(int)"
```

`-r`: removes an employee by name


```sh
./bin/dbview -f somefile.db -r "name"

```

## Order

Commands and Flags can be used and combined in any order.
```sh
./bin/dbview -a 'joe,10 addr ln,85' -n -l -f ./mynewdb.db
```