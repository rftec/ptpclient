
CC=gcc
CFLAGS=-c -Wall -g
LDFLAGS=-Wall -g -lusb-1.0 -lpthread
PYLDFLAGS=-lpython2.7
SOURCES=client.c ptp.c ptp-pima.c ptp-sony.c dynbuf.c timer.c usb.c
PYSOURCES=pyptp.c
OBJECTS=$(SOURCES:.c=.o)
PYOBJECTS=$(PYSOURCES:.c=.o)
EXEC=ptpclient
PYTARGET=pyptp
PYMOD=$(PYTARGET).so

all: $(EXEC) $(PYTARGET)

$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(PYTARGET): $(PYMOD)

$(PYMOD): $(OBJECTS) $(PYOBJECTS)
	$(CC) $(OBJECTS) $(PYOBJECTS) $(LDFLAGS) $(PYLDFLAGS) -o $@
	
.c.o:
	$(CC) $(CFLAGS) $< -o $@

ptp.c: ptp.h
client.c: ptp.h

clean:
	rm -f $(EXEC) $(PYMOD) $(OBJECTS) $(PYOBJECTS)

.PHONY: all clean $(PYTARGET)
