#!/bin/bash

rm -f Result
rm -f log_*
rm -rf Saving_Bank*
rm -rf Saving_Polyhedral*


mpirun -np 2 ../../src_dualdesc/POLY_MPI_DualDesc Main_mpi.nml