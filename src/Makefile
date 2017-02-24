#!/usr/bin/make
#
# sts - statistical test suite
#
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

# utilities
#
SHELL= bash
CC= cc
INSTALL= install
UNCOMPRESS= uncompress
RM= rm
CP= cp
MV= mv
CMP= cmp
CTAGS= ctags
MKDIR= mkdir
MAKEDEPEND= makedepend
EGREP= egrep
FGREP= fgrep
VALGRIND= valgrind
VALGRIND_OPTIONS= --leak-check=full --show-reachable=yes --tool=memcheck -v
OSX_MALLOC_ENV= MallocGuardEdges=1 MallocStackLogging=1 MallocStackLoggingNoCompact=1 \
    MallocScribble=1 MallocCheckHeapAbort=1 MallocErrorAbort=1 MallocCorruptionAbort=1
TEE= tee
INDENT= indent
DIFF= diff

# how to compile
#
LEGACY_LIBS= -lm -pthread
LIBS= -lm -L/usr/local/lib -lfftw3 -pthread
#OPT=
OPT= -O3
#DEBUG=
DEBUG= -g3
LOC_INC= -I /usr/include -I /usr/global/include -I /usr/local/include
#CFLAGS= -std=c99 -D_ISOC99_SOURCE -Wall -Werror -pedantic -DDEBUG_LINT ${LOC_INC} ${OPT} ${DEBUG}
#CFLAGS= -std=c99 -D_ISOC99_SOURCE -Wall -Werror -pedantic ${LOC_INC} ${OPT} ${DEBUG}
CFLAGS= -std=c99 -D_ISOC99_SOURCE -Wall -pedantic ${LOC_INC} ${OPT} ${DEBUG}

# locations
#
ROOTDIR= ..
DESTBINDIR= /usr/local/bin

# source
#
CSRC= sts.c tests/frequency.c tests/blockFrequency.c \
	tests/cusum.c tests/runs.c tests/longestRunOfOnes.c \
	tests/serial.c tests/rank.c tests/discreteFourierTransform.c \
	tests/nonOverlappingTemplateMatchings.c \
	tests/overlappingTemplateMatchings.c tests/universal.c \
	tests/approximateEntropy.c tests/randomExcursions.c \
	tests/randomExcursionsVariant.c tests/linearComplexity.c \
	utils/dfft.c utils/cephes.c utils/matrix.c \
	utils/utilities.c utils/generators.c utils/genutils.c \
	utils/parse_args.c utils/debug.c utils/dyn_alloc.c utils/driver.c

HSRC= utils/cephes.h utils/config.h utils/defs.h \
	utils/dfft.h utils/externs.h utils/generators.h utils/genutils.h \
	utils/matrix.h utils/stat_fncs.h utils/utilities.h utils/debug.h \
	utils/dyn_alloc.h

SRCS= ${CSRC} ${HSRC}

LEGACY_ONLY_OBJ= utils/genutils_legacy.o utils/dyn_alloc_legacy.o \
      utils/dfft.o tests/discreteFourierTransform_legacy.o \
      sts_legacy.o tests/frequency_legacy.o tests/blockFrequency_legacy.o \
      tests/cusum_legacy.o tests/runs_legacy.o tests/longestRunOfOnes_legacy.o \
      tests/serial_legacy.o tests/rank_legacy.o \
      tests/nonOverlappingTemplateMatchings_legacy.o \
      tests/overlappingTemplateMatchings_legacy.o tests/universal_legacy.o \
      tests/approximateEntropy_legacy.o tests/randomExcursions_legacy.o \
      tests/randomExcursionsVariant_legacy.o tests/linearComplexity_legacy.o \
      utils/cephes_legacy.o utils/matrix_legacy.o \
      utils/utilities_legacy.o utils/generators_legacy.o \
      utils/parse_args_legacy.o utils/debug_legacy.o utils/driver_legacy.o

MODERN_ONLY_OBJ= utils/genutils.o utils/dyn_alloc.o \
      sts.o tests/frequency.o tests/blockFrequency.o \
      tests/cusum.o tests/runs.o tests/longestRunOfOnes.o \
      tests/serial.o tests/rank.o tests/discreteFourierTransform.o \
      tests/nonOverlappingTemplateMatchings.o \
      tests/overlappingTemplateMatchings.o tests/universal.o \
      tests/approximateEntropy.o tests/randomExcursions.o \
      tests/randomExcursionsVariant.o tests/linearComplexity.o \
      utils/cephes.o utils/matrix.o \
      utils/utilities.o utils/generators.o \
      utils/parse_args.o utils/debug.o utils/driver.o

OBJ_LEGACY= ${LEGACY_ONLY_OBJ}

OBJ= ${MODERN_ONLY_OBJ}

# Code that is mentioned in source comments that is not strictly part of the
# sts code base.  And while only mkapertemplate has a rule to compile itself
# in this Makefile, that rule is not invoked when compiling sts.
#
# We only mention this code because it is referenced in the source code
# comments and we want to be sure it is kept around.
#
TOOLS_SRC= ../tools/mkapertemplate.c ../tools/pi_term.nb ../tools/pi_term.txt \
	../tools/runs_table.cal

LEGACY_TARGETS= sts_legacy_fft ../sts_legacy_fft

TARGETS= sts ../sts

# default rule
#
all: ${TARGETS} ${TOOLS_SRC}

legacy: ${LEGACY_TARGETS}

sts: ${OBJ}
	${RM} -f $@
	${CC} ${CFLAGS} ${OBJ} ${LIBS} -o $@

sts_legacy_fft: ${OBJ_LEGACY}
	${RM} -f $@
	${CC} ${CFLAGS} ${OBJ_LEGACY} ${LEGACY_LIBS} -o $@

../sts: sts
	${RM} -f $@
	${CP} -f -p sts $@

../sts_legacy_fft: sts_legacy_fft
	${RM} -f $@
	${CP} -f -p sts_legacy_fft $@

# object dependencies and rules
#
sts.o: sts.c
	${CC} -c -o $@ ${CFLAGS} sts.c

sts_legacy.o: sts.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT sts.c

utils/parse_args.o: utils/parse_args.c
	${CC} -c -o $@ ${CFLAGS} utils/parse_args.c

utils/parse_args_legacy.o: utils/parse_args.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT utils/parse_args.c

utils/debug.o: utils/debug.c
	${CC} -c -o $@ ${CFLAGS} utils/debug.c

utils/debug_legacy.o: utils/debug.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT utils/debug.c

tests/frequency.o: tests/frequency.c
	${CC} -c -o $@ ${CFLAGS} tests/frequency.c

tests/frequency_legacy.o: tests/frequency.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/frequency.c

tests/blockFrequency.o: tests/blockFrequency.c
	${CC} -c -o $@ ${CFLAGS} tests/blockFrequency.c

tests/blockFrequency_legacy.o: tests/blockFrequency.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/blockFrequency.c

tests/cusum.o: tests/cusum.c
	${CC} -c -o $@ ${CFLAGS} tests/cusum.c

tests/cusum_legacy.o: tests/cusum.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/cusum.c

tests/runs.o: tests/runs.c
	${CC} -c -o $@ ${CFLAGS} tests/runs.c

tests/runs_legacy.o: tests/runs.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/runs.c

tests/longestRunOfOnes.o: tests/longestRunOfOnes.c
	${CC} -c -o $@ ${CFLAGS} tests/longestRunOfOnes.c

tests/longestRunOfOnes_legacy.o: tests/longestRunOfOnes.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/longestRunOfOnes.c

tests/rank.o: tests/rank.c
	${CC} -c -o $@ ${CFLAGS} tests/rank.c

tests/rank_legacy.o: tests/rank.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/rank.c

tests/discreteFourierTransform.o: tests/discreteFourierTransform.c
	${CC} -c -o $@ ${CFLAGS} tests/discreteFourierTransform.c

tests/discreteFourierTransform_legacy.o: tests/discreteFourierTransform.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/discreteFourierTransform.c

tests/nonOverlappingTemplateMatchings.o: tests/nonOverlappingTemplateMatchings.c
	${CC} -c -o $@ ${CFLAGS} tests/nonOverlappingTemplateMatchings.c

tests/nonOverlappingTemplateMatchings_legacy.o: tests/nonOverlappingTemplateMatchings.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/nonOverlappingTemplateMatchings.c

tests/overlappingTemplateMatchings.o: tests/overlappingTemplateMatchings.c
	${CC} -c -o $@ ${CFLAGS} tests/overlappingTemplateMatchings.c

tests/overlappingTemplateMatchings_legacy.o: tests/overlappingTemplateMatchings.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/overlappingTemplateMatchings.c

tests/universal.o: tests/universal.c
	${CC} -c -o $@ ${CFLAGS} tests/universal.c

tests/universal_legacy.o: tests/universal.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/universal.c

tests/approximateEntropy.o: tests/approximateEntropy.c
	${CC} -c -o $@ ${CFLAGS} tests/approximateEntropy.c

tests/approximateEntropy_legacy.o: tests/approximateEntropy.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/approximateEntropy.c

tests/randomExcursions.o: tests/randomExcursions.c
	${CC} -c -o $@ ${CFLAGS} tests/randomExcursions.c

tests/randomExcursions_legacy.o: tests/randomExcursions.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/randomExcursions.c

tests/randomExcursionsVariant.o: tests/randomExcursionsVariant.c
	${CC} -c -o $@ ${CFLAGS} tests/randomExcursionsVariant.c

tests/randomExcursionsVariant_legacy.o: tests/randomExcursionsVariant.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/randomExcursionsVariant.c

tests/serial.o: tests/serial.c
	${CC} -c -o $@ ${CFLAGS} tests/serial.c

tests/serial_legacy.o: tests/serial.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/serial.c

tests/linearComplexity.o: tests/linearComplexity.c
	${CC} -c -o $@ ${CFLAGS} tests/linearComplexity.c

tests/linearComplexity_legacy.o: tests/linearComplexity.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT tests/linearComplexity.c

utils/dfft.o: utils/dfft.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT utils/dfft.c

utils/matrix.o: utils/matrix.c
	${CC} -c -o $@ ${CFLAGS} utils/matrix.c

utils/matrix_legacy.o: utils/matrix.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT utils/matrix.c

utils/genutils.o: utils/genutils.c
	${CC} -c -o $@ ${CFLAGS} utils/genutils.c

utils/genutils_legacy.o: utils/genutils.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT utils/genutils.c

utils/cephes.o: utils/cephes.c
	${CC} -c -o $@ ${CFLAGS} utils/cephes.c

utils/cephes_legacy.o: utils/cephes.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT utils/cephes.c

utils/utilities.o: utils/utilities.c
	${CC} -c -o $@ ${CFLAGS} utils/utilities.c

utils/utilities_legacy.o: utils/utilities.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT utils/utilities.c

utils/generators.o: utils/generators.c
	${CC} -c -o $@ ${CFLAGS} utils/generators.c

utils/generators_legacy.o: utils/generators.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT utils/generators.c

utils/dyn_alloc.o: utils/dyn_alloc.c
	${CC} -c -o $@ ${CFLAGS} utils/dyn_alloc.c

utils/dyn_alloc_legacy.o: utils/dyn_alloc.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT utils/dyn_alloc.c

utils/driver.o: utils/driver.c
	${CC} -c -o $@ ${CFLAGS} utils/driver.c

utils/driver_legacy.o: utils/driver.c
	${CC} -c -o $@ ${CFLAGS} -DLEGACY_FFT utils/driver.c

mkapertemplate: ../tools/mkapertemplate.c utils/debug.o
	${CC} -o $@ ${CFLAGS} -I . ../tools/mkapertemplate.c utils/debug.o

# utility rules
#
clean:
	${RM} -f ${OBJ} ${LEGACY_ONLY_OBJ}

clobber: clean
	@for i in ${TARGETS} ${LEGACY_TARGETS} tags Makefile.bak mkapertemplate; do \
	    if [[ -e "$$i" ]]; then \
		echo ${RM} -f "$$i"; \
		${RM} -f "$$i"; \
	    fi; \
	done
	@for i in valrun minivalrun osxmalloc miniosxmalloc; do \
	    if [[ -e "$$i" ]]; then \
		echo ${RM} -rf "$$i"; \
		${RM} -rf "$$i"; \
	    fi; \
	done
	@for i in $$(seq -w 01 15); do \
	    if [[ -e "eachrun_$$i" ]]; then \
		echo ${RM} -rf "eachrun_$$i"; \
		${RM} -rf "eachrun_$$i"; \
	    fi; \
	done
	@for i in ${SRCS}; do \
	    if [[ -e "$$i"'~' ]]; then \
		echo ${RM} -f "$$i"'~'; \
		${RM} -f "$$i"'~'; \
	    fi; \
	done

tags: ${SRC}
	${RM} -f tags
	@echo "${CTAGS} ${SRCS}"
	${CTAGS} ${SRCS}

ctags: tags

picky: ${SRCS}
	picky -v -w132 ${SRCS}
	@if grep -l '[	 ]$$' ${SRCS} >/dev/null; then \
	    echo ; \
	    echo "also found bogus tabs and/or spaces in:"; \
	    grep -l '[	 ]$$' ${SRCS}; \
	    exit 1; \
	fi

indent: ${SRCS}
	@for i in ${SRCS}; do \
	    ${RM} -f "$$i"'~'; \
	    ${INDENT} "$$i"; \
	    if ${CMP} -s "$$i"'~' "$$i"; then \
		${RM} -f "$$i"'~'; \
	    else \
		echo "=-= indent changed $$i =-="; \
		${DIFF} -u "$$i"'~' "$$i"; \
	    fi; \
	done

valgrind: sts
	${RM} -rf valrun
	${MKDIR} valrun
	${VALGRIND} ${VALGRIND_OPTIONS} ./sts -b -i 20 -g 4 -I 1 -w valrun -v 3 1000000 2>&1 | \
		${TEE} valrun/valgrind.out

valgrindeach: sts
	@for i in $$(seq -w 01 15); do \
	    if [[ -e "eachrun_$$i" ]]; then \
		echo ${RM} -rf "eachrun_$$i"; \
		${RM} -rf "eachrun_$$i"; \
	    fi; \
	done
	for i in $$(seq -w 01 15); do \
	    ${MKDIR} -p "eachrun_$$i"; \
	    ${VALGRIND} ${VALGRIND_OPTIONS} ./sts -b -t "$$i" -i 20 -g 4 -I 1 -w "eachrun_$$i" -v 3 1000000 2>&1 | \
		    ${TEE} eachrun_$$i/valgrind.out; \
	done

minivalgrind: sts
	${RM} -rf minivalrun
	${MKDIR} minivalrun
	${VALGRIND} ${VALGRIND_OPTIONS} ./sts -b -i 2 -g 4 -I 1 -w minivalrun -v 3 1000000 2>&1 | \
		${TEE} minivalrun/valgrind.out

osxtest: sts
	${RM} -rf osxmalloc osxmalloc/malloc.out
	${MKDIR} -p osxmalloc
	${OSX_MALLOC_ENV} MallocStackLoggingDirectory=osxmalloc \
	  MallocLogFile=osxmalloc/malloc.out \
	  ./sts -b -i 20 -g 4 -I 1 -w osxmalloc -v 3 1000000
	@if [[ -f osxmalloc/malloc.out && -s osxmalloc/malloc.out ]]; then \
	    echo "=-=-= malloc error log follows"; \
	    cat osxmalloc/malloc.out; \
	    echo "=-=-= malloc error log preceeds"; \
	else \
	    echo "=-=-= no osxmalloc/malloc.out data, removing =-=-="; \
	    rm -f osxmalloc/malloc.out; \
	fi

miniosxtest: sts
	${RM} -rf miniosxmalloc miniosxmalloc/malloc.out
	${MKDIR} -p miniosxmalloc
	${OSX_MALLOC_ENV} MallocStackLoggingDirectory=miniosxmalloc \
	  MallocLogFile=miniosxmalloc/malloc.out \
	  ./sts -b -i 2 -g 4 -I 1 -w miniosxmalloc -v 3 1000000
	@if [[ -f miniosxmalloc/malloc.out && -s miniosxmalloc/malloc.out ]]; then \
	    echo "=-=-= malloc error log follows"; \
	    cat miniosxmalloc/malloc.out; \
	    echo "=-=-= malloc error log preceeds"; \
	else \
	    echo "=-=-= no miniosxmalloc/malloc.out data, removing =-=-="; \
	    rm -f miniosxmalloc/malloc.out; \
	fi

rebuild: clobber sts

depend:
	@${RM} -f Makefile.bak
	@echo "${MAKEDEPEND} -I. -Y -- ${CFLAGS} -- ${CSRC}"
	@-${MAKEDEPEND} -I. -Y -- ${CFLAGS} -- ${CSRC} 2>&1 | \
	    ${EGREP} -v 'string\.h|stdlib\.h|stdio\.h|errno\.h|stdarg\.h|getopt\.h|limits\.h' | \
	    ${EGREP} -v 'time\.h|math\.h|sys/types\.h|unistd\.h|fcntl\.h|sys/stat\.h'
	@-if ${CMP} -s Makefile Makefile.bak; then \
	    echo "${MAKEDEPEND} did not change anything, backup not needed"; \
	    echo ${RM} -f Makefile.bak; \
	    ${RM} -f Makefile.bak; \
	else \
	    echo "${MAKEDEPEND} modified Makefile, try running:"; \
	    echo "  diff -u Makefile.bak Makefile"; \
	fi

install: all
	if [ ! -d "${DESTBINDIR}" ]; then \
	    ${MKDIR} -p "${DESTBINDIR}"; \
	fi
	${INSTALL} -c -m 0755 sts ${DESTBINDIR}

# DO NOT DELETE THIS LINE -- make depend depends on it.

sts.o: utils/defs.h utils/config.h utils/dyn_alloc.h
sts.o: utils/utilities.h utils/externs.h
sts.o: utils/defs.h utils/debug.h
tests/frequency.o: utils/externs.h utils/defs.h utils/utilities.h
tests/frequency.o: utils/debug.h utils/cephes.h
tests/blockFrequency.o: utils/externs.h utils/defs.h utils/cephes.h
tests/blockFrequency.o: utils/utilities.h utils/debug.h
tests/cusum.o: utils/externs.h utils/defs.h utils/cephes.h utils/utilities.h
tests/cusum.o: utils/debug.h
tests/runs.o: utils/externs.h utils/defs.h utils/cephes.h utils/utilities.h
tests/runs.o: utils/debug.h
tests/longestRunOfOnes.o: utils/externs.h utils/defs.h utils/cephes.h
tests/longestRunOfOnes.o: utils/utilities.h utils/debug.h
tests/serial.o: utils/externs.h utils/defs.h utils/cephes.h utils/utilities.h
tests/serial.o: utils/debug.h
tests/rank.o: utils/externs.h utils/defs.h utils/cephes.h utils/matrix.h
tests/rank.o: utils/defs.h utils/config.h utils/dyn_alloc.h
tests/rank.o: utils/utilities.h utils/debug.h
tests/discreteFourierTransform.o: utils/externs.h utils/defs.h
tests/discreteFourierTransform.o: utils/utilities.h utils/cephes.h
tests/discreteFourierTransform.o: utils/debug.h
tests/nonOverlappingTemplateMatchings.o: utils/externs.h utils/defs.h
tests/nonOverlappingTemplateMatchings.o: utils/utilities.h utils/cephes.h
tests/nonOverlappingTemplateMatchings.o: utils/debug.h
tests/overlappingTemplateMatchings.o: utils/externs.h utils/defs.h
tests/overlappingTemplateMatchings.o: utils/utilities.h utils/cephes.h
tests/overlappingTemplateMatchings.o: utils/debug.h
tests/universal.o: utils/externs.h utils/defs.h utils/utilities.h
tests/universal.o: utils/cephes.h utils/debug.h
tests/approximateEntropy.o: utils/externs.h utils/defs.h utils/utilities.h
tests/approximateEntropy.o: utils/cephes.h utils/debug.h
tests/randomExcursions.o: utils/externs.h utils/defs.h utils/cephes.h
tests/randomExcursions.o: utils/utilities.h utils/debug.h
tests/randomExcursionsVariant.o: utils/externs.h utils/defs.h utils/cephes.h
tests/randomExcursionsVariant.o: utils/utilities.h utils/debug.h
tests/linearComplexity.o: utils/externs.h utils/defs.h utils/cephes.h
tests/linearComplexity.o: utils/utilities.h utils/debug.h
utils/cephes.o: utils/cephes.h utils/debug.h
utils/matrix.o: utils/externs.h utils/defs.h utils/matrix.h utils/defs.h
utils/matrix.o: utils/config.h utils/dyn_alloc.h
utils/matrix.o: utils/debug.h
utils/utilities.o: utils/externs.h utils/defs.h utils/utilities.h
utils/utilities.o: utils/generators.h utils/debug.h
utils/generators.o: utils/externs.h utils/defs.h utils/utilities.h
utils/generators.o: utils/generators.h utils/genutils.h utils/config.h
utils/generators.o: utils/debug.h
utils/genutils.o: utils/genutils.h utils/config.h utils/debug.h
utils/parse_args.o: utils/externs.h utils/defs.h utils/utilities.h
utils/parse_args.o: utils/debug.h
utils/debug.o: utils/debug.h utils/externs.h utils/defs.h
utils/dyn_alloc.o: utils/externs.h utils/defs.h utils/debug.h
utils/dyn_alloc.o: utils/utilities.h
utils/driver.o: utils/defs.h utils/utilities.h utils/debug.h
utils/driver.o: utils/stat_fncs.h
