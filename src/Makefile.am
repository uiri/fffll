AM_CFLAGS=-Wall -g -pedantic

bin_PROGRAMS = fffll
fffll_SOURCES = fffll.c fffll.l.c value.c tree.c
fffll_LDADD = libfffllstd.la libffflllist.la -lcurl
fffll_CFLAGS = $(AM_CFLAGS)

pkglib_LTLIBRARIES = libfffllstd.la libffflllist.la
noinst_HEADERS = value.h evaluate.h tree.h array.h fffll.h list.h builtin.h
libfffllstd_la_SOURCES = builtin.c evaluate.c tree.c
libfffllstd_la_LDFLAGS = -module -version-info 0:0:0
libfffllstd_la_LIBADD = -lcurl -lpcre -lm
libffflllist_la_SOURCES = list.c array.c
libffflllist_la_LDFLAGS = -module -version-info 0:0:0
