#!/bin/make
# @(#)Makefile	1.2 04 May 1995 02:06:57
#
# sts - Stastical Test Suite

SHELL= /bin/bash
RM= rm
MAKE= make

# the version we build and install
#
VERSION= sts-2.1.2-1

SRC= src

# default rules
#
all: assess

assess: ${SRC}/assess
	cd ${SRC}; $(MAKE) ../assess

${SRC}/assess:
	cd ${SRC}; $(MAKE) all

# utility rules
#
clean:
	cd ${SRC}; $(MAKE) $@

clobber:
	cd ${SRC}; $(MAKE) $@

tags:
	cd ${SRC}; $(MAKE) $@

ctags:
	cd ${SRC}; $(MAKE) $@

rebuild:
	cd ${SRC}; $(MAKE) $@

depend:
	cd ${SRC}; $(MAKE) $@

install: all
	cd ${SRC}; $(MAKE) $@
