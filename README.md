GameOfLife
==========

A parallel implementation of The Conway's Game Of Life using MPI

There's also an interactive demo-version of the game, although, its totally Serial code.

Usage
-----
$ mpirun -q -mca btl ^openib -np [N_PROCESSORS] ./app.exe [ARG1] [ARG2] [ARG3]

The Arguments
-------------
There are two different modes to use. These modes are the same for both the Serial and the Parallel versions.

1. Generate random gameboard
  ./app.exe [FIELD_SIZE_X] [FIELD_SIZE_Y] [N_ITERATIONS]

2. Load a gameboard from the file [TEXTFILE.TXT]
  ./app.exe -L [FILENAME.TXT] [N_ITERATIONS]

Partnership
-----------
If one would create a nice GUI to cover the project, it'll be highly appreciated!
