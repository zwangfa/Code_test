
prog = test-webserver

CC=gcc
CFLAGS = -I.

OBJS = cia.o \
	base64.o \
	fa_log.o \
	cJSON.o \
	http.o \
	util.o \
	test-webserver.o


test-webserver: $(OBJS)
	$(CC) -Wall -Werror -g -I. -o $@ $^ -lcivetweb  -L. -lcrypto -ldl -lpthread

.PHONY : clean
clean:
	@rm *.o test-webserver