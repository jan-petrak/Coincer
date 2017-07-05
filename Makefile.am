ACLOCAL_AMFLAGS = -I m4 --install

bin_PROGRAMS = src/coincerd src/coincer

src_coincerd_SOURCES = \
	src/coincerd.c

src_coincerd_CFLAGS = $(AM_CFLAGS) $(LIBEVENT_CFLAGS)
src_coincerd_LDADD  = $(AM_LDADD) $(LIBEVENT_LIBS)

src_coincer_SOURCES = \
	src/coincer.c
