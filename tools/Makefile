#!/usr/bin/make
#
# tools - statistical test suite optional tools
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
LIBS= -lm
#OPT=
OPT= -O3
#DEBUG=
DEBUG= -g3
LOC_INC=
#CFLAGS= -std=c99 -D_ISOC99_SOURCE -Wall -Werror -pedantic -DDEBUG_LINT ${LOC_INC} ${OPT} ${DEBUG}
#CFLAGS= -std=c99 -D_ISOC99_SOURCE -Wall -Werror -pedantic ${LOC_INC} ${OPT} ${DEBUG}
CFLAGS= -std=c99 -D_ISOC99_SOURCE -Wall -pedantic ${LOC_INC} ${OPT} ${DEBUG}

# locations
#
ROOTDIR= ..
DESTBINDIR= /usr/local/bin

# source
#
CSRC= generators.c mkapertemplate.c

HSRC= 

SRCS= ${CSRC} ${HSRC}

OBJ= generators.o mkapertemplate.o

# We only mention this code because it is referenced in the source code
# comments and we want to be sure it is kept around.
#
TOOLS_SRC= pi_term.nb pi_term.txt 

TARGETS= mkapertemplate generators

# default rule
#
all: ${TARGETS} ${TOOLS_SRC}

mkapertemplate: mkapertemplate.o
	${CC} -o $@ ${CFLAGS} mkapertemplate.o

generators: generators.o
	${CC} -o $@ ${CFLAGS} -lm generators.o

# object dependencies and rules
#
mkapertemplate.o: mkapertemplate.c
	${CC} -c -o $@ ${CFLAGS} mkapertemplate.c

generators.o: generators.c
	${CC} -c -o $@ ${CFLAGS} generators.c

# utility rules
#
clean:
	${RM} -f ${OBJ}

clobber: clean
	@for i in ${TARGETS} tags Makefile.bak mkapertemplate; do \
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

