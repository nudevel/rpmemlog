noinst_PROGRAMS = test
test_SOURCES = test.c rpmemlog.c
test_LDADD = -lpthread
test_CFLAGS = $(AM_CFLAGS) -Wall

lib_LTLIBRARIES = librpmemlog.la
librpmemlog_la_SOURCES = rpmemlog.c
librpmemlog_la_LDFLAGS = -version-info 0:0:0
librpmemlog_la_CFLAGS = -Wall
include_HEADERS = rpmemlog.h
