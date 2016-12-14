# NIST Statistical Test Suite

This project is a considerably improved version of the NIST Statistical Test Suite (**STS**), a collection of tests used in the 
 evaluation of the randomness of bitstreams of data.

## Purpose

STS can be useful in:

- Evaluating the randomness of bitstreams produced by hardware and software key generators for cryptographic applications.
- Evaluating the quality of pseudo random number generators used in simulation and modeling applications.
- Testing the computational capabilities of hardware (the suite takes time and resources to run).

## Usage
   
#### Get data to test (optional)

// TODO upload in a separate repository

#### How to run

<!--TODO standard usage or with script-->

```sh
$ cd sts
$ make
$ ./sts
```

Be patient: depending on the hardware used and on the number of iterations specified, the suite might take minutes, hours or days 
 to finish the test. 

After the run is done a report will be generated in a file called `report.txt`.

## Project structure

The project has three folders:

- *src*: contains the sourcecode of the tests and 
- *docs*: TODO
- *tools*: TODO

## Improvements

Our major improvement starts from sourcecode of the [original source code of version 2.1.2][site] and consists in several
 bug fixes, code improvements and added documentation. Here are some of our improvements:

- Added an option to run tests in batch mode (executed without human intervention)
- Fixed the implementation of the tests where bugs or inconsistencies with the paper were found
- Improved the performance (execution time is now about 50% shorter)
- Eliminated the use of the global variables in the tests (now a new struct state is instead passed around as pointer)
- Eliminated the use of files to pass data during the execution of the suite (now memory is used instead)
- Introduced a "driver-like" function interface to test functions (instead of calling one monolithic function for each test)
- Added minimum test conditions checks (now a test is disabled when its minimum requirements are not satisfied)
- Improved the memory allocation patterns of each test
- Eliminated the use of "magic numbers" (numeric values that were used without explanation)
- Minimized the use of fixed size arrays, to prevent errors
- Added checks for errors on return from system functions
- Added debugging, notice, warning and fatal error messages
- Fixed the use of uninitialized values (some issues were caught with valgrind)
- Improved the precision of test constants (by using tools such as Mathematica and Calc)
- Improved the variable names when they were confusing or had a name different than the one in the paper
- Improved command line usage by adding new flags (including a -h for usage help)
- Improved the coding style to be more consistent and easier to understand 
- Added extensive and detailed source code documentation
- Improved the support for 64-bit processors
- Improved the Makefile to use best practices and to be more portable
- Fixed the warnings reported by compilers
- Improved the program output in the finalAnalysisReport.txt file
- Fixed memory leaks

## Coming soon

- Graphical visualization of the final p-values in a gnu-plot
- Parallel run in multiple cores and hosts
- Revised version of the STS paper with fixes
- Improved entropy test (not approximate)

## Contributors

The people who have so far contributed to this major improvement of the NIST STS are:

- Landon Curt Noll ([lcn2](https://github.com/lcn2))
- Tom Gilgan ([???](https://github.com/???)) <!-- TODO ask Tom github -->
- Riccardo Paccagnella ([ricpacca](https://github.com/ricpacca))

You can contact us using the following email address: *sts-contributors (at) external (dot) cisco (dot) com*

By no means we believe we have fixed every last bug in this code. Moreover we do not doubt that we have introduced
 new bugs during our transformation. For any such new bugs, we apologize and welcome contributions that fix either old bugs
 or our newly introduced ones. Improvements to the documentation are also a very appreciated contribution.

Please feel invited to contribute by creating a pull request to submit the code you would like to be included. 

## License

The original software was developed at the National Institute of Standards and Technology by employees of the Federal Government 
 in the course of their official duties. Pursuant to title 17 Section 105 of the United States Code this software is not subject
 to copyright protection and is in the public domain. Furthermore, Cisco's contribution is also placed in the public domain.
 The NIST Statistical Test Suite is an experimental system. Neither NIST nor Cisco assume any responsibility whatsoever for
 its use by other parties, and makes no guarantees, expressed or implied, about its quality, reliability, or any other 
 characteristic. We would appreciate acknowledgment if the software is used.
 
## Special thanks

Special thanks go to the original code developers. Without their efforts this modified code would not have been possible.
 The original NIST document, [SP800-22Rev1a][paper], was extremely valuable to us.

The above partial list of issues is presented to help explain why we extensively modified their original code.
 Our bug fixes are an expression of gratitude for their efforts.
 
We also thank the original authors for making their code freely available. We saw the value of their efforts
 and set about the tasks of extending their code to situations they did not intend.


   [site]: <http://csrc.nist.gov/groups/ST/toolkit/rng/documentation_software.html>
   [paper]: <http://csrc.nist.gov/groups/ST/toolkit/rng/documents/SP800-22rev1a.pdf>