
CC       ?= gcc
CFLAGS   ?= -Wall -Wextra -O2 -DAUTOEXECLIST_NO_MAIN
LDFLAGS  ?=

TARGET   := init
SRCDIR   := .
ROOTFS   ?= ../rootfs
SOURCES  := $(shell find $(SRCDIR) -type f -name '*.c')
OBJECTS  := $(SOURCES:.c=.o)

.PHONY: all install clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(TARGET)
	install -D -m 0755 $(TARGET) $(ROOTFS)/sbin/init

clean:
	rm -f $(OBJECTS) $(TARGET)
