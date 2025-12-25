# Operating Systems Project - myshell
# 编译方法: make
# 清理: make clean

CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = myshell
SOURCES = myshell.c utility.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

myshell.o: myshell.c myshell.h
	$(CC) $(CFLAGS) -c myshell.c

utility.o: utility.c myshell.h
	$(CC) $(CFLAGS) -c utility.c

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean