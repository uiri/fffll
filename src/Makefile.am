AM_CFLAGS=-Wall -g -pedantic

bin_PROGRAMS = fffll
fffll_SOURCES = fffll.c fffll.l.c value.c tree.c
fffll_LDADD = libfffllstd.la libffflllist.la
fffll_CFLAGS = $(AM_CFLAGS)
fffll_LDFLAGS=-L. -lm -lcurl -lpcre

pkglib_LTLIBRARIES = libfffllstd.la libffflllist.la
pkginclude_HEADERS = list.h builtin.h
noinst_HEADERS = value.h evaluate.h tree.h array.h fffll.h
libfffllstd_la_SOURCES = builtin.c evaluate.c tree.c
libfffllstd_la_LDFLAGS = -module
libffflllist_la_SOURCES = list.c array.c
libffflllist_la_LDFLAGS = -module
