#the information part
CC = gcc

SOURCES = conf.c file.c gen_cli.c getopt_clone.c iolet.c llist.c useful.c 

#  test-check_failure.c

BUILDDATE = $(strip $(shell date +%d%m%Y))

CFLAGS_IN = -g -DCONF_TESTING -DDEBUG -Wall -W -I. -I.. $(CFLAGS) -DBUILDDATE=$(BUILDDATE)
LDFLAGS = 

OBJECTS = $(subst .c,.o,$(SOURCES))
COMPILE = $(CC) $(CFLAGS_IN) 
LINK = $(CC) $(LDFLAGS)

#test

CF_PROGRAM = test-check_failure

CONF_SRC = conf2.c 
CONF_PRG = test-conf2

#compiling
all: $(OBJECTS) $(CF_PROGRAM)

$(CF_PROGRAM): $(OBJECTS) 
	$(LINK) -o $(CF_PROGRAM) $(OBJECTS)

$(CONF_PRG): $(OBJECTS)
	$(LINK) -o $(CONF_PRG) $(OBJECTS)

#%.o: %.c
#	$(COMPILE) -c $<

.c.o: 
	$(COMPILE) -c $?

clean:
	rm -f $(OBJECTS) $(CF_PROGRAM) core *~
