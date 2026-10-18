
# Cookie Init — main build

CC       ?= gcc
AR       ?= ar
CFLAGS   ?= -Wall -Wextra -O2 -static -DAUTOEXECLIST_NO_MAIN
CPPFLAGS ?=
LDFLAGS  ?=
LDLIBS   ?=

TARGET      := init
LIB_DIR     := lib
LIB_ARCHIVE := $(LIB_DIR)/libinit.a
LIB_MAKE    := $(MAKE) -C $(LIB_DIR) -f Makefile.libinit-a

# Discover every C source recursively, excluding lib/.
SOURCES := $(shell find . -path './lib' -prune -o -type f -name '*.c' -print)
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean install libinit

all: $(TARGET)

libinit:
	$(LIB_MAKE) CC="$(CC)" AR="$(AR)" \
		CFLAGS="$(CFLAGS)" CPPFLAGS="$(CPPFLAGS)"

$(TARGET): $(OBJECTS) libinit
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) $(LIB_ARCHIVE) $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)
	$(LIB_MAKE) clean

install: $(TARGET)
	install -D -m 0755 $(TARGET) "$(DESTDIR)/sbin/init"
