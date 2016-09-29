/*****************************************************************************
 U T I L I T Y  F U N C T I O N  P R O T O T Y P E S
 *****************************************************************************/

#ifndef UTILITY_H
#   define UTILITY_H

extern long int getNumber(FILE * input, FILE * output);
extern double getDouble(FILE * input, FILE * output);
extern long int getNumberOrDie(FILE * stream);
extern char *getString(FILE * stream);
extern void makePath(char *dir);
extern bool checkWritePermissions(char *dir);
extern bool checkReadPermissions(char *dir);
extern FILE *openTruncate(char *filename);
extern char *filePathName(char *head, char *tail);
extern char *data_filename_format(int partitionCount);
extern void precheckPath(struct state *state, char *dir);
extern char *precheckSubdir(struct state *state, char *subdir);
extern long int str2longint(bool * success_p, char *string);
extern long int str2longint_or_die(char *string);
extern int displayGeneratorOptions(void);
extern void generatorOptions(struct state *state);
extern void chooseTests(struct state *state);
extern void fixParameters(struct state *state);
extern void fileBasedBitStreams(struct state *state);
extern void readHexDigitsInBinaryFormat(struct state *state);
extern bool convertToBits(struct state *state, BYTE * x, long int xBitLength, long int bitsNeeded, long int *num_0s,
			  long int *num_1s, long int *bitsRead);
extern void invokeTestSuite(struct state *state);
extern void nist_test_suite(struct state *state);
extern void createDirectoryTree(struct state *state);
extern void write_sequence(struct state *state);
extern void print_option_summary(struct state *state, char *where);

#endif				/* UTILITY_H */
