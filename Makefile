rwildcard=$(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

CC=gcc
CFLAGS=-std=c11 -c -Wall -Wno-unused-function -g
LDFLAGS=
SOURCES=$(call rwildcard, , *.c)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=lang

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean

clean:
	rm -f $(EXECUTABLE) *.o
