# GSAFSolver

A solver for abstract argumentation frameworks with collective attacks (SETAF) capable of providing RUP-style inconsistency proofs for unsatisfiable instances.

## 1 Installation

Requires fmt (c.f. https://fmt.dev). Build using cmake.

## 2 Usage 

Usage: solver [OPTIONS] -i `<FILE>`

Options:
  * -i `<FILE>`\
     A file that contains the encoding of the instance. 
  * -c `<FILE>`\
     A file to which the inconsistency proof should be printed to, if the instance has no extension.
  * -d `<FILE>`\
     A file that contains the instance description.
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
     A file that contains the required arguments.           
  * -s `<SEMANTICS>`\
     The semantics that the proof adheres to. [possible values: Stable]     
  * -t `<TIMEOUT>`\
     The timeout in seconds 0 for no limit. [default: 0]
