#!/usr/bin/make
#
# sts - stastical test suite

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

# utilities
#
SHELL= bash
CC= cc
INSTALL= install
UNCOMPRESS= uncompress
RM= rm
CP= cp
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
LIBS= -lm
OPT=
#OPT= -O3
#DEBUG=
DEBUG= -g3
#CFLAGS= -std=c99 -D_ISOC99_SOURCE -Wall -Werror -pedantic -DDEBUG_LINT ${OPT} ${DEBUG}
CFLAGS= -std=c99 -D_ISOC99_SOURCE -Wall -Werror -pedantic ${OPT} ${DEBUG}
#CFLAGS= -std=c99 -D_ISOC99_SOURCE -Wall -pedantic ${OPT} ${DEBUG}

# locations
#
ROOTDIR= ..
DATADIR= ${ROOTDIR}/data
#
DESTLIBDIR= /usr/global/lib/sts
DESTBINDIR= /usr/global/bin
DESTDATADIR= ${DESTLIBDIR}/data

# source
#
CSRC= assess.c frequency.c blockFrequency.c \
	cusum.c runs.c longestRunOfOnes.c \
	serial.c rank.c discreteFourierTransform.c \
	nonOverlappingTemplateMatchings.c \
	overlappingTemplateMatchings.c universal.c \
	approximateEntropy.c randomExcursions.c \
	randomExcursionsVariant.c linearComplexity.c \
	dfft.c cephes.c matrix.c \
	utilities.c generators.c genutils.c \
	Parse_args.c debug.c dyn_alloc.c driver.c

HSRC= cephes.h config.h defs.h \
	dfft.h externs.h generators.h genutils.h \
	matrix.h stat_fncs.h utilities.h debug.h \
	dyn_alloc.h

SRCS= ${CSRC} ${HSRC}

OBJ= assess.o frequency.o blockFrequency.o \
      cusum.o runs.o longestRunOfOnes.o \
      serial.o rank.o discreteFourierTransform.o \
      nonOverlappingTemplateMatchings.o \
      overlappingTemplateMatchings.o universal.o \
      approximateEntropy.o randomExcursions.o \
      randomExcursionsVariant.o linearComplexity.o \
      dfft.o cephes.o matrix.o \
      utilities.o generators.o genutils.o \
      Parse_args.o debug.o dyn_alloc.o driver.o

DATA= ${DATADIR}/BBS-1165667.dat ${DATADIR}/data.bad_rng ${DATADIR}/data.e \
	${DATADIR}/data.pi ${DATADIR}/data.sha1 ${DATADIR}/data.sqrt2 \
	${DATADIR}/data.sqrt3

TARGETS= assess ../assess

# default rule
#
all: ${TARGETS} ${DATA}

assess: ${OBJ}
	${RM} -f $@
	${CC} ${CFLAGS} ${OBJ} ${LIBS} -o $@

../assess: assess
	${RM} -f $@
	${CP} -f -p assess $@

# object dependencies and rules
#
assess.o: assess.c
	${CC} -c -o $@ ${CFLAGS} assess.c

Parse_args.o: Parse_args.c
	${CC} -c -o $@ ${CFLAGS} Parse_args.c

debug.o: debug.c
	${CC} -c -o $@ ${CFLAGS} debug.c

frequency.o: frequency.c
	${CC} -c -o $@ ${CFLAGS} frequency.c

blockFrequency.o: blockFrequency.c
	${CC} -c -o $@ ${CFLAGS} blockFrequency.c

cusum.o: cusum.c
	${CC} -c -o $@ ${CFLAGS} cusum.c

runs.o: runs.c
	${CC} -c -o $@ ${CFLAGS} runs.c

longestRunOfOnes.o: longestRunOfOnes.c
	${CC} -c -o $@ ${CFLAGS} longestRunOfOnes.c

rank.o: rank.c
	${CC} -c -o $@ ${CFLAGS} rank.c

discreteFourierTransform.o: discreteFourierTransform.c
	${CC} -c -o $@ ${CFLAGS} discreteFourierTransform.c

nonOverlappingTemplateMatchings.o: nonOverlappingTemplateMatchings.c
	${CC} -c -o $@ ${CFLAGS} nonOverlappingTemplateMatchings.c

overlappingTemplateMatchings.o: overlappingTemplateMatchings.c
	${CC} -c -o $@ ${CFLAGS} overlappingTemplateMatchings.c

universal.o: universal.c
	${CC} -c -o $@ ${CFLAGS} universal.c

approximateEntropy.o: approximateEntropy.c
	${CC} -c -o $@ ${CFLAGS} approximateEntropy.c

randomExcursions.o: randomExcursions.c
	${CC} -c -o $@ ${CFLAGS} randomExcursions.c

randomExcursionsVariant.o: randomExcursionsVariant.c
	${CC} -c -o $@ ${CFLAGS} randomExcursionsVariant.c

serial.o: serial.c
	${CC} -c -o $@ ${CFLAGS} serial.c

linearComplexity.o: linearComplexity.c
	${CC} -c -o $@ ${CFLAGS} linearComplexity.c

dfft.o: dfft.c
	${CC} -c -o $@ ${CFLAGS} dfft.c

matrix.o: matrix.c
	${CC} -c -o $@ ${CFLAGS} matrix.c

genutils.o: genutils.c
	${CC} -c -o $@ ${CFLAGS} genutils.c

cephes.o: cephes.c
	${CC} -c -o $@ ${CFLAGS} cephes.c

utilities.o: utilities.c
	${CC} -c -o $@ ${CFLAGS} utilities.c

generators.o: generators.c
	${CC} -c -o $@ ${CFLAGS} generators.c

dyn_alloc.o: dyn_alloc.c
	${CC} -c -o $@ ${CFLAGS} dyn_alloc.c

driver.o: driver.c
	${CC} -c -o $@ ${CFLAGS} driver.c

# utility rules
#
clean:
	${RM} -f ${OBJ}

clobber: clean
	@for i in ${TARGETS} tags Makefile.bak; do \
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

valgrind: assess
	${RM} -rf valrun
	${MKDIR} valrun
	${VALGRIND} ${VALGRIND_OPTIONS} ./assess -b -i 20 -g 4 -I 1 -w valrun -v 3 1000000 2>&1 | \
		${TEE} valrun/valgrind.out

valgrindeach: assess
	@for i in $$(seq -w 01 15); do \
	    if [[ -e "eachrun_$$i" ]]; then \
		echo ${RM} -rf "eachrun_$$i"; \
		${RM} -rf "eachrun_$$i"; \
	    fi; \
	done
	for i in $$(seq -w 01 15); do \
	    ${MKDIR} -p "eachrun_$$i"; \
	    ${VALGRIND} ${VALGRIND_OPTIONS} ./assess -b -t "$$i" -i 20 -g 4 -I 1 -w "eachrun_$$i" -v 3 1000000 2>&1 | \
		    ${TEE} eachrun_$$i/valgrind.out; \
	done

minivalgrind: assess
	${RM} -rf minivalrun
	${MKDIR} minivalrun
	${VALGRIND} ${VALGRIND_OPTIONS} ./assess -b -i 2 -g 4 -I 1 -w minivalrun -v 3 1000000 2>&1 | \
		${TEE} minivalrun/valgrind.out

osxtest: assess
	${RM} -rf osxmalloc osxmalloc/malloc.out
	${MKDIR} -p osxmalloc
	${OSX_MALLOC_ENV} MallocStackLoggingDirectory=osxmalloc \
	  MallocLogFile=osxmalloc/malloc.out \
	  ./assess -b -i 20 -g 4 -I 1 -w osxmalloc -v 3 1000000
	@if [[ -f osxmalloc/malloc.out && -s osxmalloc/malloc.out ]]; then \
	    echo "=-=-= malloc error log follows"; \
	    cat osxmalloc/malloc.out; \
	    echo "=-=-= malloc error log preceeds"; \
	else \
	    echo "=-=-= no osxmalloc/malloc.out data, removing =-=-="; \
	    rm -f osxmalloc/malloc.out; \
	fi

miniosxtest: assess
	${RM} -rf miniosxmalloc miniosxmalloc/malloc.out
	${MKDIR} -p miniosxmalloc
	${OSX_MALLOC_ENV} MallocStackLoggingDirectory=miniosxmalloc \
	  MallocLogFile=miniosxmalloc/malloc.out \
	  ./assess -b -i 2 -g 4 -I 1 -w miniosxmalloc -v 3 1000000
	@if [[ -f miniosxmalloc/malloc.out && -s miniosxmalloc/malloc.out ]]; then \
	    echo "=-=-= malloc error log follows"; \
	    cat miniosxmalloc/malloc.out; \
	    echo "=-=-= malloc error log preceeds"; \
	else \
	    echo "=-=-= no miniosxmalloc/malloc.out data, removing =-=-="; \
	    rm -f miniosxmalloc/malloc.out; \
	fi

rebuild: clobber assess

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
	if [ ! -d "${DESTLIBDIR}" ]; then \
	    ${MKDIR} -p "${DESTLIBDIR}"; \
	fi
	if [ ! -d "${DESTBINDIR}" ]; then \
	    ${MKDIR} -p "${DESTBINDIR}"; \
	fi
	if [ ! -d "${DESTDATADIR}" ]; then \
	    ${MKDIR} -p "${DESTDATADIR}"; \
	fi
	${INSTALL} -c -m 0755 assess ${DESTBINDIR}
	${INSTALL} -m 0444 ${DATA} ${DESTDATADIR}

# DO NOT DELETE THIS LINE -- make depend depends on it.

assess.o: defs.h config.h dyn_alloc.h cephes.h utilities.h stat_fncs.h
assess.o: externs.h debug.h
frequency.o: externs.h defs.h config.h dyn_alloc.h utilities.h debug.h
frequency.o: cephes.h
blockFrequency.o: externs.h defs.h config.h dyn_alloc.h cephes.h utilities.h
blockFrequency.o: debug.h
cusum.o: externs.h defs.h config.h dyn_alloc.h cephes.h utilities.h debug.h
runs.o: externs.h defs.h config.h dyn_alloc.h cephes.h utilities.h debug.h
longestRunOfOnes.o: externs.h defs.h config.h dyn_alloc.h cephes.h
longestRunOfOnes.o: utilities.h debug.h
serial.o: externs.h defs.h config.h dyn_alloc.h cephes.h utilities.h debug.h
rank.o: externs.h defs.h config.h dyn_alloc.h cephes.h matrix.h utilities.h
rank.o: debug.h
discreteFourierTransform.o: externs.h defs.h config.h dyn_alloc.h utilities.h
discreteFourierTransform.o: cephes.h dfft.h debug.h
nonOverlappingTemplateMatchings.o: externs.h defs.h config.h dyn_alloc.h
nonOverlappingTemplateMatchings.o: utilities.h cephes.h debug.h
overlappingTemplateMatchings.o: externs.h defs.h config.h dyn_alloc.h
overlappingTemplateMatchings.o: utilities.h cephes.h debug.h
universal.o: externs.h defs.h config.h dyn_alloc.h utilities.h cephes.h
universal.o: debug.h
approximateEntropy.o: externs.h defs.h config.h dyn_alloc.h utilities.h
approximateEntropy.o: cephes.h debug.h
randomExcursions.o: externs.h defs.h config.h dyn_alloc.h cephes.h
randomExcursions.o: utilities.h debug.h
randomExcursionsVariant.o: externs.h defs.h config.h dyn_alloc.h cephes.h
randomExcursionsVariant.o: utilities.h debug.h
linearComplexity.o: externs.h defs.h config.h dyn_alloc.h cephes.h
linearComplexity.o: utilities.h debug.h
dfft.o: dfft.h
cephes.o: cephes.h defs.h config.h dyn_alloc.h externs.h debug.h
matrix.o: externs.h defs.h config.h dyn_alloc.h matrix.h debug.h
utilities.o: externs.h defs.h config.h dyn_alloc.h utilities.h generators.h
utilities.o: stat_fncs.h debug.h
generators.o: externs.h defs.h config.h dyn_alloc.h utilities.h generators.h
generators.o: genutils.h debug.h
genutils.o: genutils.h config.h debug.h
Parse_args.o: externs.h defs.h config.h dyn_alloc.h utilities.h debug.h
debug.o: debug.h externs.h defs.h config.h dyn_alloc.h
dyn_alloc.o: dyn_alloc.h debug.h
driver.o: defs.h config.h dyn_alloc.h utilities.h debug.h stat_fncs.h
driver.o: externs.h
