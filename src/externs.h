#ifndef EXTERNS_H
#define EXTERNS_H

#include "defs.h"

/*****************************************************************************
                   G L O B A L   D A T A  S T R U C T U R E S 
 *****************************************************************************/

extern const char * const version;		// sts version
extern char * program;				// our name (argv[0])
extern long int debuglevel;			// -v lvl: defines the level of verbosity for debugging
extern const struct driver test[NUMOFTESTS+1];	// test drivers

extern BitSequence	*epsilon;		// BIT STREAM
extern FILE		*stats[NUMOFTESTS+1];	// FILE OUTPUT STREAM
extern FILE		*results[NUMOFTESTS+1];	// FILE OUTPUT STREAM
extern FILE		*freqfp;		// FILE OUTPUT STREAM
extern FILE		*summary;		// FILE OUTPUT STREAM

#endif /*EXTERNS_H*/
