#!/bin/make
#
# sts - Stastical Test Suite

# This code has been heavily modified by the following people:
#
# 	Landon Curt Noll
# 	Tom Gilgan
#	Riccardo Paccagnella
#
# See the README.md and the initial comment in sts.c for more information.
#
# WE (THOSE LISTED ABOVE WHO HEAVILY MODIFIED THIS CODE) DISCLAIM ALL
# WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL WE (THOSE LISTED ABOVE
# WHO HEAVILY MODIFIED THIS CODE) BE LIABLE FOR ANY SPECIAL, INDIRECT OR
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
all: sts

legacy:
	cd ${SRC}; $(MAKE) $@

sts: ${SRC}/sts
	cd ${SRC}; $(MAKE) ../sts

sts_legacy_fft: ${SRC}/sts_legacy_fft
	cd ${SRC}; $(MAKE) ../sts_legacy_fft

${SRC}/sts:
	cd ${SRC}; $(MAKE) all

${SRC}/sts_legacy_fft:
	cd ${SRC}; $(MAKE) sts_legacy_fft

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
