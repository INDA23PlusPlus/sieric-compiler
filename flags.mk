CC=gcc

CPPFLAGS?=
CPPFLAGS+=-Iinclude -D_POSIX_C_SOURCE=2

CFLAGS?=-O2 -g
CFLAGS+=-Wall -Wextra -MD -std=c99

LDFLAGS?=
LDFLAGS+=
