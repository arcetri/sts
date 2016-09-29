#!/bin/make
#
# sts - Stastical Test Suite

# This code has been heavily modified by Landon Curt Noll (chongo at cisco dot com) and Tom Gilgan (thgilgan at cisco dot com).
# See the initial comment in assess.c and the file README.txt for more information.
#
# TOM GILGAN AND LANDON CURT NOLL DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
# EVENT SHALL TOM GILGAN NOR LANDON CURT NOLL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
# USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#
# chongo (Landon Curt Noll, http://www.isthe.com/chongo/index.html) /\oo/\
#
# Share and enjoy! :-)

SHELL= /bin/bash
MAKE= make

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

picky:
	cd ${SRC}; $(MAKE) $@

indent:
	cd ${SRC}; $(MAKE) $@

valgrind:
	cd ${SRC}; $(MAKE) $@

valgrindeach:
	cd ${SRC}; $(MAKE) $@

minivalgrind:
	cd ${SRC}; $(MAKE) $@

osxtest:
	cd ${SRC}; $(MAKE) $@

mimiosxtest:
	cd ${SRC}; $(MAKE) $@

rebuild:
	cd ${SRC}; $(MAKE) $@

depend:
	cd ${SRC}; $(MAKE) $@

install: all
	cd ${SRC}; $(MAKE) $@
