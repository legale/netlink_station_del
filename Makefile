#name of output file
NAME = station_del
#build dir
BD = ./build

#Linker flags
LDLIBS +=
LDDIRS += -L$(BD)

#Compiler flags
CFLAGS += -Wall -O2
CFLAGS += -I/usr/include/libnl-tiny
CFLAGS += -I/usr/local/include/libnl-tiny
CFLAGS += -I/usr/include

CFLAGS += -L/usr/local/lib
LDFLAGS += -lnl-tiny

#Compiler
CC = gcc
AR = ar

#SRC=$(wildcard *.c)
SRC_BIN = main.c station_del.c syslog.c
SRC = $(SRC_BIN)

all: $(NAME)

$(NAME): $(SRC)
	
	$(CC) $(CFLAGS)  $^ -o build/$(NAME) $(LDFLAGS)

clean:
	rm -rf $(BD)/*
