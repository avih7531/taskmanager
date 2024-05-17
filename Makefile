CC=gcc
CFLAGS=-Wall -Wextra -std=c99 -pedantic -O2
LDFLAGS=-lsqlite3 -llinenoise

SRCS=Prod.c
TARGET=Prod

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
