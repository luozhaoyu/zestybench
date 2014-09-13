CC=cc
CFLAGS=-c -Wall -Wextra -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_REENTRANT $(INCLUDE)
INCLUDE=
LDFLAGS=
LIBS=-lrt

SOURCES=$(wildcard *.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))
VENDOR_OBJS=vendor/vendor.o
EXECUTABLE=measure

all: clean $(SOURCES) $(EXECUTABLE)
	./measure

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

$(OBJECTS):
	$(CC) $(CFLAGS) $(SOURCES)

.PHONY: clean all
clean:
	rm -rf $(EXECUTABLE)
	rm -rf $(OBJECTS)
