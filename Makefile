CC=		gcc
CFLAGS=		-g  -Wall -Werror -std=gnu99 -Iinclude  #took out -Wall
LD=		gcc
LDFLAGS=	-L.
AR=		ar
ARFLAGS=	rcs
TARGETS=	bin/spidey

all:		$(TARGETS)

clean:
	@echo Cleaning...
	@rm -f $(TARGETS) lib/*.a src/*.o *.log *.input

.PHONY:		all test clean

# TODO: Add rules for bin/spidey, lib/libspidey.a, and any intermediate objects

src/forking.o: src/forking.c
	$(CC) $(CFLAGS) -c -o $@ $^

src/utils.o: src/utils.c
	$(CC) $(CFLAGS) -c -o $@ $^

src/handler.o: src/handler.c
	$(CC) $(CFLAGS) -c -o $@ $^

src/request.o: src/request.c
	$(CC) $(CFLAGS) -c -o $@ $^

src/single.o: src/single.c
	$(CC) $(CFLAGS) -c -o $@ $^

src/socket.o: src/socket.c
	$(CC) $(CFLAGS) -c -o $@ $^

src/spidey.o: src/spidey.c
	$(CC) $(CFLAGS) -c -o $@ $^

lib/libspidey.a: src/forking.o src/handler.o src/request.o src/single.o src/socket.o src/utils.o src/spidey.o
	$(AR) $(ARFLAGS) $@ $^

bin/spidey: src/spidey.o lib/libspidey.a
	$(LD) $(LDFLAGS) -o $@ $^
