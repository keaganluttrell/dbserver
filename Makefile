TARGET_SRV = bin/dbserver
TARGET_CLI = bin/dbcli

SRC_SRV = $(wildcard src/srv/*.c)
OBJ_SRV = $(SRC_SRV:src/srv/%.c=obj/srv/%.o)

SRC_CLI = $(wildcard src/cli/*.c)
OBJ_CLI = $(SRC_CLI:src/cli/%.c=obj/cli/%.o)

DIRS = bin obj obj/srv obj/cli

run: clean default
	./$(TARGET_SRV) -f ./mynewdb.db -n -p 8080 -A manifest.txt &
	./$(TARGET_CLI) -h 127.0.0.1 -p 8080 -a "tom,1 main st,85"
	./$(TARGET_CLI) -h 127.0.0.1 -p 8080 -a "tim,2 state st,87"
	./$(TARGET_CLI) -h 127.0.0.1 -p 8080 -l
	
default: dirs $(TARGET_SRV) $(TARGET_CLI)

clean:
	rm -rf obj/srv/*.o
	rm -rf bin/*
	rm -rf *.db

dirs:
	mkdir -p $(DIRS)

$(TARGET_SRV): $(OBJ_SRV)
	gcc -g -o $@ $?

$(OBJ_SRV): obj/srv/%.o: src/srv/%.c
	gcc -g -c $< -o $@ -Iinclude

$(TARGET_CLI): $(OBJ_CLI)
	gcc -g -o $@ $?

$(OBJ_CLI): obj/cli/%.o: src/cli/%.c
	gcc -g -c $< -o $@ -Iinclude
