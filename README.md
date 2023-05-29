# GSAFSolver

A solver for abstract argumentation frameworks with collective attacks (SETAF) capable of providing RUP-style inconsistency proofs for unsatisfiable instances.

## Installation

Requires fmt (c.f. https://fmt.dev). Build using cmake.

## Usage 

Usage: solver [OPTIONS] -i `<FILE>`

Options:
  * -i `<FILE>`\
     A file that contains the encoding of the instance, see also: [instance file format](#instance-file-format).
  * -c `<FILE>`\
     A file to which the inconsistency proof should be printed to, if the instance has no extension.
  * -d `<FILE>`\
     A file that contains the instance description, which can be used to map the argument number to names, see also: [description file format](#description-file-format).
  * -g `<RATE>`\
     The growth rate for clause learning in each cycle. [default: 2]              
  * -h `<HEURISTIC>`\
     The heuristic to to use. [possible values0: None, MaxOutDegree, MinInDegree, PathLengthN, ModifiedPathLengthN; default: None]
  * -n `<EXTENSIONS>`\
     The number of extensions that should be enumerated or 0 for no limit. [default: 0]          
  * -p `<PERCENTAGE>`\
     The percentage of clauses that should be forgotten in each cycle. [default: 0.5]       
  * -q\
     When provided, the extensions are not printed.
  * -r `<FILE>`\
     A file that contains the required arguments, see also: [required arguments file format](#required-arguments-file-format).         
  * -s `<SEMANTICS>`\
     The semantics that the proof adheres to. [possible values: Stable]     
  * -t `<TIMEOUT>`\
     The timeout in seconds 0 for no limit. [default: 0]

## Instance file format

The format is similar to the DIMACS format used by SAT solvers, where the header line gives the number of arguments and attacks and each subsequent line represents an attack, with the attacked argument named first and the arguments in the support trailing.

### Example
```
6 8 0
2 1 0
3 1 0
1 2 0
1 3 0
4 2 3 0
5 4 0
6 5 0
4 6 0
```
The first line of the instance encoding denotes that the framework consists of 6 arguments and 8 attacks. Every following line encodes an attack, with the second line encoding an attack from argument 1 to argument 2 and the 6th line encoding a ''proper'' set attack from arguments 2 and 3 to 4. 

## Description file format

Can be used to map the argument number to names, which will be reflected in the found models and can be referenced in the required argument file. Argument names must be unique.

### Example
```
1 Arg1
2 Arg2
3 Arg3
4 Arg4
```
Assigns the name "Arg1" to argument number 1.

## Required arguments file format

Can be used to instruct the solver to include or exclude some arguments without further justification. 

### Example

```
1
-2
s Arg3
s -Arg4
```
Requires arguments with number 1 and 2 to be included / excluded respectively. Likewise for the arguments with name “Arg3” and “Arg4”.
