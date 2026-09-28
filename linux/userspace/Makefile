CC ?= cc
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Werror -Wpedantic

.PHONY: all clean install

all: bms-gateway

bms-gateway: bms_gateway.c
	$(CC) $(CFLAGS) -o $@ $<

install: bms-gateway
	install -D -m 0755 bms-gateway $(DESTDIR)/usr/bin/bms-gateway

clean:
	rm -f bms-gateway

