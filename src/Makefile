#!/usr/bin/make
#
# sts - stastical test suite

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

# how to compile
#
OPT=
DEBUG= -g3
# XXX - release without -Werror: CFLAGS= -std=c99 -Wall -pedantic ${OPT} ${DEBUG}
CFLAGS= -std=c99 -Wall -Werror -pedantic ${OPT} ${DEBUG}

# locations
#
ROOTDIR= ..
TEMPLATEDIR= ${ROOTDIR}/templates
DATADIR= ${ROOTDIR}/data
#
DESTLIBDIR= /usr/local/lib/sts
DESTBINDIR= /usr/local/bin
DESTTEMPLATEDIR= ${DESTLIBDIR}/templates
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
	Parse_args.c debug.c dyn_alloc.c

HSRC= cephes.h config.h decls.h defs.h \
	dfft.h externs.h generators.h genutils.h \
	matrix.h stat_fncs.h utilities.h

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
      Parse_args.o debug.o dyn_alloc.o

# source data files
#
TEMPLATES= \
	${TEMPLATEDIR}/template2 ${TEMPLATEDIR}/template3 \
	${TEMPLATEDIR}/template4 ${TEMPLATEDIR}/template5 \
	${TEMPLATEDIR}/template6 ${TEMPLATEDIR}/template7 \
	${TEMPLATEDIR}/template8 ${TEMPLATEDIR}/template9 \
	${TEMPLATEDIR}/template10 ${TEMPLATEDIR}/template11 \
	${TEMPLATEDIR}/template12 ${TEMPLATEDIR}/template13 \
	${TEMPLATEDIR}/template14 ${TEMPLATEDIR}/template15 \
	${TEMPLATEDIR}/template16

DATA= ${DATADIR}/BBS.dat ${DATADIR}/data.bad_rng ${DATADIR}/data.e \
	${DATADIR}/data.pi ${DATADIR}/data.sha1 ${DATADIR}/data.sqrt2 \
	${DATADIR}/data.sqrt3

# These template files are build from compressed source
#
BUILT_TEMPLATES= ${TEMPLATEDIR}/template17 \
	${TEMPLATEDIR}/template18 ${TEMPLATEDIR}/template19 \
	${TEMPLATEDIR}/template20 ${TEMPLATEDIR}/template21

TARGETS= assess ../assess

# default rule
#
all: ${TARGETS} ${TEMPLATES} ${BUILT_TEMPLATES} ${DATA}

assess: ${OBJ}
	${RM} -f $@
	${CC} ${CFLAGS} ${OBJ} -lm -o $@

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

nonOverlappingTemplateMatchings.o: \
    nonOverlappingTemplateMatchings.c
	${CC} -c -o $@ ${CFLAGS} nonOverlappingTemplateMatchings.c

overlappingTemplateMatchings.o: \
	overlappingTemplateMatchings.c
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

# The larger templates are shipped compressed, uncompress
# for use by sts
#
${TEMPLATEDIR}/template17: ${TEMPLATEDIR}/template17.Z
	${RM} -f $@
	${UNCOMPRESS} -d -f -c $? > $@

${TEMPLATEDIR}/template18: ${TEMPLATEDIR}/template18.Z
	${RM} -f $@
	${UNCOMPRESS} -d -f -c $? > $@

${TEMPLATEDIR}/template19: ${TEMPLATEDIR}/template19.Z
	${RM} -f $@
	${UNCOMPRESS} -d -f -c $? > $@

${TEMPLATEDIR}/template20: ${TEMPLATEDIR}/template20.Z
	${RM} -f $@
	${UNCOMPRESS} -d -f -c $? > $@

${TEMPLATEDIR}/template21: ${TEMPLATEDIR}/template21.Z
	${RM} -f $@
	${UNCOMPRESS} -d -f -c $? > $@

# utility rules
#
clean:
	${RM} -f ${OBJ}

clobber: clean
	${RM} -f ${TARGETS}
	${RM} -f tags makefile.bak
	${RM} -f ${BUILT_TEMPLATES}

tags: ${SRC}
	${RM} -f tags
	@echo "${CTAGS} ${SRCS}"
	@-${CTAGS} ${SRCS} 2>&1 | \
	    ${EGREP} -v 'Second entry ignored|Duplicate entry in file config.h'

ctags: tags

rebuild: clobber assess

depend:
	@${RM} -f Makefile.bak
	@echo "${MAKEDEPEND} -I. -Y -- ${CFLAGS} -- ${SRCS}"
	@-${MAKEDEPEND} -I. -Y -- ${CFLAGS} -- ${SRCS} 2>&1 | \
	    ${EGREP} -v 'string.h|stdlib.h|stdio.h|errno.h|stdarg.h|getopt.h' | \
	    ${EGREP} -v 'time.h|math.h|sys/types.h|unistd.h|fcntl.h|sys/stat.h'
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
	if [ ! -d "${DESTTEMPLATEDIR}" ]; then \
	    ${MKDIR} -p "${DESTTEMPLATEDIR}"; \
	fi
	if [ ! -d "${DESTDATADIR}" ]; then \
	    ${MKDIR} -p "${DESTDATADIR}"; \
	fi
	${INSTALL} -c -m 0755 assess ${DESTBINDIR}
	${INSTALL} -m 0444 ${TEMPLATES} ${DESTTEMPLATEDIR}
	${INSTALL} -m 0444 ${DATA} ${DESTDATADIR}

# DO NOT DELETE THIS LINE -- make depend depends on it.

assess.o: defs.h config.h cephes.h utilities.h stat_fncs.h
frequency.o: externs.h defs.h config.h utilities.h
blockFrequency.o: externs.h defs.h config.h cephes.h utilities.h
cusum.o: externs.h defs.h config.h cephes.h utilities.h
runs.o: externs.h defs.h config.h cephes.h utilities.h
longestRunOfOnes.o: externs.h defs.h config.h cephes.h utilities.h
serial.o: externs.h defs.h config.h cephes.h utilities.h
rank.o: externs.h defs.h config.h cephes.h matrix.h utilities.h
discreteFourierTransform.o: externs.h defs.h config.h utilities.h cephes.h
discreteFourierTransform.o: dfft.h
nonOverlappingTemplateMatchings.o: externs.h defs.h config.h utilities.h
nonOverlappingTemplateMatchings.o: cephes.h
overlappingTemplateMatchings.o: externs.h defs.h config.h utilities.h
overlappingTemplateMatchings.o: cephes.h
universal.o: externs.h defs.h config.h utilities.h cephes.h
approximateEntropy.o: externs.h defs.h config.h utilities.h cephes.h
randomExcursions.o: externs.h defs.h config.h cephes.h utilities.h
randomExcursionsVariant.o: externs.h defs.h config.h cephes.h utilities.h
linearComplexity.o: externs.h defs.h config.h cephes.h utilities.h
dfft.o: dfft.h
cephes.o: cephes.h defs.h config.h
matrix.o: externs.h defs.h config.h matrix.h
utilities.o: externs.h defs.h config.h utilities.h generators.h stat_fncs.h
generators.o: externs.h defs.h config.h utilities.h generators.h genutils.h
genutils.o: genutils.h config.h
Parse_args.o: externs.h defs.h config.h utilities.h
debug.o: externs.h defs.h config.h
dyn_alloc.o: defs.h config.h
defs.o: config.h
externs.o: defs.h config.h
genutils.o: config.h
