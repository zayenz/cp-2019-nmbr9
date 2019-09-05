# Nmbr9 as a Constraint Programming Challenge

This repository contains the code for the abstract "Nmbr9 as a
Constraint Programming Challenge" by Mikael Zayenz Lagerkvist. The
abstract is submitted to the 25th International Conference on
Principles and Practice of Constraint Programming 2019.

The code is located in the directory `code/` and is a CMake C++ 17
project. 


## Licenses

* The code is licensed under the MIT license.
* The paper is licensed under CC-BY-SA-4.0.

Relevant license texts are available in the subdirectories.

## Building and running the experiments

[Gecode 6.2.0](https://www.gecode.org/ ) is required to be
installed on the system before building. The code has only been tested
on a Mac, it is likely that another way of discovering and linking
Gecode might be needed for other platforms, currently the CMake
`find_library` method is used for finding and setting up Gecode.

First, create a new build directory `code/build`. From within `build`, use 
```
$ cmake ..
$ make
``` 
to build the application.

There will be an executable produced `src/nmbr9-cli` that uses the
Gecode script command line driver for running the model.


