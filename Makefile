TARGET = bin/dbview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))

run: setup clean default
	./$(TARGET) -f ./mynewdb.db -n

default: $(TARGET)

setup:
	mkdir -p obj bin

test: clean default
	#### Create DB-------------------------
	./$(TARGET) -f ./mynewdb.db -n
	#### Add employee ---------------------
	./$(TARGET) -f ./mynewdb.db -a 'joe,10 addr ln,85'
	#### List employees -------------------
	./$(TARGET) -f ./mynewdb.db -l
	#### Update employee hours
	./$(TARGET) -f ./mynewdb.db -u 'joe,100' -l 
	#### Remove employee ------------------
	./$(TARGET) -f ./mynewdb.db -r 'joe' -l
	#### Add Employees with a file --------
	./$(TARGET) -f ./mynewdb.db -A manifest.txt -l


clean:
	rm -f obj/*.o
	rm -f bin/*
	rm -f *.db

$(TARGET): $(OBJ)
	gcc -o $@ $?

obj/%.o : src/%.c
	gcc -c $< -o $@ -Iinclude -g
