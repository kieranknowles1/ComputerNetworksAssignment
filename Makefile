CC=gcc
LDFLAGS=-pthread -lcurses -lncurses
LIBS=libnet.o console_safe.o
CFLAGS=-Wall -DREMOVE_DUMMY_HEADER
DEBUG_CFLAGS=-g -O0
SOURCES=libnet.c console_safe.c controller.c

help:
	@echo "make <target> where target is one of"
	@echo "             all:   everything"
	@echo "             run:   make and run 'control'"
	@echo "           debug:   make and run 'control' in GDB"
	@echo "      controller:   build the contoller program"
	@echo "debug_controller:   build 'controller' with debugging symbols"
	@echo "            tags:   build the tags file with 'ctags'"
	@echo "                    useful for navigating code in vim"
	@echo "           clean:   delete files that can be rebuilt"
	@echo "          pretty:   reformat the code with 'indent -kr'"
	@echo "            help:   show this help"
	@echo "     consoledocs:   show the help man page for the console library"
	@echo "         netdocs:   show the help man page for the libnet library"

all: $(LIBS) controller

run: controller
	./controller 65200 65250

debug: debug_controller
	gdb -ex run --args debug_controller 65200 65250

# controller: controller.c $(LIBS)
controller:
	$(CC) $(CFLAGS)   controller.c $(LIBS)   -o controller $(LDFLAGS)

debug_controller: $(LIBS)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) controller.c $(LIBS) -o debug_controller $(LDFLAGS)

.PHONY: consoledocs netdocs
consoledocs:
	groff -man -Tutf8 console.3 | less
netdocs:
	groff -man -Tutf8 libnet.3 | less

tags: $(SOURCES)
	ctags $?

.PHONY: clean pretty

clean:
	rm -f $(LIBS) controller debug_controller

pretty: $(SOURCES)
	indent -kr $?

