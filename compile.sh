#!/bin/bash
set -e
#(cd src_latt && make clean && make)
(cd src_poly && make clean && make)
(cd src_polydecomp && make clean && make)
(cd src_poly && make -f Makefile_double_cddlib clean && make -f Makefile_double_cddlib)
(cd src_short && make clean && make)
(cd src_dualdesc && make clean && make)
(cd src_sparse_solver && make clean && make)
(cd src_spectra && make clean && make)
(cd src_copos && make clean && make)
(cd src_ctype_mpi && make clean && make)
(cd src_perfect && make clean && make)
(cd src_indefinite && make clean && make)
(cd src_perfect_mpi && make clean && make)
(cd src_lorentzian && make clean && make)
