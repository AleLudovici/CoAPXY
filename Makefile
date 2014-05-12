LIBS=-L./coap -lcoap -lm -lpthread -ldl -L/opt/db-5.2.28/lib -ldb -lsqlite3 -lssl
CFLAGS=-D_LARGEFILE64_SOURCE -DNDEBUG -g -O2

#CC=/opt/backfire/staging_dir/toolchain-armeb_v5te_gcc-4.4.2_uClibc-0.9.30.3/usr/bin/armeb-openwrt-linux-gcc
#CFLAGS=-Os -pipe -march=armv5te -mtune=xscale -funit-at-a-time -DNDEBUG
#CFLAGS+=-I/opt/backfire/staging_dir/toolchain-armeb_v5te_gcc-4.4.2_uClibc-0.9.30.3/usr/include -L/home/polmoreno/Documentos/PFC/gateway/backfire/staging_dir/toolchain-armeb_v5te_gcc-4.4.2_uClibc-0.9.30.3/usr/lib

COAPOBJS=coap/coap_pdu.o coap/coap_cache.o coap/coap_debug.o coap/coap_option.o coap/coap_list.o coap/coap_net.o coap/coap_context.o coap/coap_resource.o coap/coap_client.o coap/coap_server.o coap/coap_rd.o coap/coap_md5.o
PROXYOBJS=proxy/sockset.o  proxy/websocket.o proxy/fcgi/fcgiapp.o proxy/fcgi/fcgi_stdio.o proxy/fcgi/os_unix.o proxy/connection.o proxy/resource.o
SUBDIRS=examples proxy proxy/fcgi coap

#export C_INCLUDE_PATH=/opt/db-5.2.28/include
#export CSOURCES= /opt/db-5.2.28/include

#export C_INCLUDE_PATH = /opt/db_gateway/include:/opt/sqlite_gateway/include
#export CSOURCES= /opt/db_gateway/include:/opt/sqlite_gateway/include

all: coap_proxy coap_server coap_client
.PHONY: all

coap_client: examples/coap_client.o $(COAPOBJS) libcoap.a
	$(LINK.c) -o $@ examples/coap_client.o $(LIBS)

coap_server: examples/coap_server.o $(COAPOBJS) libcoap.a
	$(LINK.c) -o $@ examples/coap_server.o $(LIBS)

coap_proxy: proxy/main.o $(PROXYOBJS) libcoap.a
	$(LINK.c) -o $@ proxy/main.o $(PROXYOBJS) $(LIBS)

libcoap.a: $(COAPOBJS)
	ar rcv coap/libcoap.a $(COAPOBJS)
	ranlib coap/libcoap.a

.PHONY: clean
clean:
	@dir=`pwd`; \
	if test -n $$dir/src; then \
		cd $$dir/src; \
		for i in $(SUBDIRS); do \
			cd $$i; \
			echo "Entering $$i "; \
			echo "rm -f *.o *.a"; \
			rm -f *.o *.a; \
			cd $$dir; \
		done; \
		rm -f *.o *.a; \
	fi;
	rm -f *% *~ *.o *.a *.lo *.slo *.la core;
	@echo "Cleaning done!";

