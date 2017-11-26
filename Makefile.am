ACLOCAL_AMFLAGS = -I m4 --install

bin_PROGRAMS = src/coincerd src/coincer

src_coincerd_SOURCES = \
	src/coincerd.c \
	src/p2p.c src/p2p.h \
	src/neighbours.c src/neighbours.h

src_coincerd_CFLAGS = $(AM_CFLAGS) $(JANSSON_CFLAGS) $(LIBEVENT_CFLAGS) $(LIBSODIUM_CFLAGS)
src_coincerd_LDADD  = $(AM_LDADD) $(JANSSON_LIBS) $(LIBEVENT_LIBS) $(LIBSODIUM_LIBS)

src_coincer_SOURCES = \
	src/coincer.c
