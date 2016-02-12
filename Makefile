
CC=gcc
CFLAGS=-c -Wall -g
LDFLAGS=-Wall -g -lusb-1.0 -lpthread
SOURCES=client.c ptp.c ptp-pima.c ptp-sony.c dynbuf.c timer.c usb.c
OBJECTS=$(SOURCES:.c=.o)
EXEC=ptpclient

all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

ptp.c: ptp.h
client.c: ptp.h

clean:
	rm -f $(EXEC) $(OBJECTS)

.PHONY: all clean
