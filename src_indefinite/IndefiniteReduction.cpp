// Copyright (C) 2022 Mathieu Dutour Sikiric <mathieu.dutour@gmail.com>
// clang-format off
#ifdef OSCAR_USE_BOOST_GMP_BINDINGS
# include "NumberTheoryBoostGmpInt.h"
#else
# include "NumberTheory.h"
#endif
#include "Indefinite_LLL.h"
// clang-format on

int main(int argc, char *argv[]) {
  SingletonTime time1;
  try {
    if (argc != 4 && argc != 2) {
      std::cerr << "IndefiniteReduction [FileI] [OutFormat] [FileO]\n";
      std::cerr << "or\n";
      std::cerr << "IndefiniteReduction [FileI]\n";
      throw TerminalException{1};
    }
#ifdef OSCAR_USE_BOOST_GMP_BINDINGS
    using T = boost::multiprecision::mpq_rational;
    using Tint = boost::multiprecision::mpz_int;
#else
    using T = mpq_class;
    using Tint = mpz_class;
#endif
    std::string FileI = argv[1];
    std::string OutFormat = "GAP";
    std::string FileO = "stderr";
    if (argc == 4) {
      OutFormat = argv[2];
      FileO = argv[3];
    }
    //
    MyMatrix<T> M = ReadMatrixFile<T>(argv[1]);
    std::cerr << "We have M\n";
    //
    auto print_result = [&](std::ostream &os) -> void {
      ResultReduction<T, Tint> ResRed =
          ComputeReductionIndefinitePermSign<T, Tint>(M);
      MyMatrix<T> B_T = UniversalMatrixConversion<T, Tint>(ResRed.B);
      MyMatrix<T> M_Control = B_T * M * B_T.transpose();
      if (T_abs(DeterminantMat(B_T)) != 1) {
        std::cerr << "B_T should have determinant 1 or -1\n";
        throw TerminalException{1};
      }
      if (M_Control != ResRed.Mred) {
        std::cerr << "M_Control is not what it should be\n";
        throw TerminalException{1};
      }
      if (OutFormat == "GAP") {
        os << "return rec(B:=";
        WriteMatrixGAP(os, ResRed.B);
        os << ", Mred:=";
        WriteMatrixGAP(os, ResRed.Mred);
        os << ");\n";
        return;
      }
      if (OutFormat == "Oscar") {
        WriteMatrix(std::cerr, ResRed.B);
        WriteMatrix(std::cerr, ResRed.Mred);
        return;
      }
      std::cerr << "Failed to find a matching entry for OutFormat=" << OutFormat << "\n";
      throw TerminalException{1};
    };
    if (FileO == "stderr") {
      print_result(std::cerr);
    } else {
      if (FileO == "stdout") {
        print_result(std::cout);
      } else {
        std::ofstream os(FileO);
        print_result(os);
      }
    }
    std::cerr << "Normal termination of the program\n";
  } catch (TerminalException const &e) {
    std::cerr << "Error in IndefiniteReduction\n";
    exit(e.eVal);
  }
  runtime(time1);
}
