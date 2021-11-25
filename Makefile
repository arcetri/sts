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

# HomeBrew on macOS overrides (or lack of override is not found)
#
ifeq ($(homebrew_fftw),)
homebrew_fftw=$(shell echo $$(brew --cellar fftw 2>/dev/null)/$$(brew list --versions fftw 2>/dev/null| tr ' ' '\n' | tail -1))
endif
# if the above fails, it will set to /, so we turn / into empty string
ifeq ($(homebrew_fftw),/)
homebrew_fftw=
endif

ifeq ($(homebrew_fftw),)
LIBS= -lm -L/usr/local/lib -lfftw3 -pthread
LOC_INC= -I /usr/include -I /usr/global/include -I /usr/local/include
else
LIBS= -lm -L${homebrew_fftw}/lib -lfftw3 -pthread
LOC_INC= -I /usr/include -I ${homebrew_fftw}/include
endif

# make variables we pass down to override make variables set in the lower level makefile
#
PASSDOWN_VARS= LIBS='${LIBS}' homebrew_fftw='${homebrew_fftw}' LOC_INC='${LOC_INC}'

SRC= src

# default rules
#
all: sts

# debug_make allows you to deterlime how HomeBrew (or lack) impacts compilation
#
debug_make:
	@echo DEBUG: top level makefile debug:
	@echo
	@echo DEBUG: homebrew_fftw: ${homebrew_fftw}
	@echo DEBUG: LIBS: ${LIBS}
	@echo DEBUG: LOC_INC: ${LOC_INC}
	@echo DEBUG: PASSDOWN_VARS: ${PASSDOWN_VARS}
	@echo
	@echo DEBUG: calling lower level makefile debug:
	@echo
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}
	@echo
	@echo DEBUG: end of top level makefile debug

legacy:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

sts: ${SRC}/sts
	cd ${SRC}; $(MAKE) ../sts ${PASSDOWN_VARS}

sts_legacy_fft: ${SRC}/sts_legacy_fft
	cd ${SRC}; $(MAKE) ../sts_legacy_fft ${PASSDOWN_VARS}

${SRC}/sts:
	cd ${SRC}; $(MAKE) all ${PASSDOWN_VARS}

${SRC}/sts_legacy_fft:
	cd ${SRC}; $(MAKE) sts_legacy_fft ${PASSDOWN_VARS}

# utility rules
#
clean:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

clobber:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

tags:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

ctags:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

picky:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

indent:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

valgrind:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

valgrindeach:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

minivalgrind:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

osxtest:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

mimiosxtest:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

rebuild:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

depend:
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}

install: all
	cd ${SRC}; $(MAKE) $@ ${PASSDOWN_VARS}
