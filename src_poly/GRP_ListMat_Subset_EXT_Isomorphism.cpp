#include "GRP_GroupFct.h"
#include "Temp_PolytopeEquiStab.h"

int main(int argc, char *argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "Number of argument is = " << argc << "\n";
      std::cerr << "This program is used as\n";
      std::cerr << "GRP_ListMat_Subset_EXT_Isomorphism [INfile] [OUTfile]\n";
      std::cerr << "\n";
      std::cerr << "INfile    : The file containing the group\n";
      std::cerr << "OUTfile   : The file containing the two pairs\n";
      return -1;
    }
    using T = mpz_class;
    using Tidx = unsigned int;
    //
    std::cerr << "Reading input\n";
    //
    std::ifstream is(argv[1]);
    int nbMat;
    is >> nbMat;
    //
    std::vector<MyMatrix<T>> ListMat1;
    for (int iMat = 0; iMat < nbMat; iMat++) {
      MyMatrix<T> eMat = ReadMatrix<T>(is);
      ListMat1.push_back(eMat);
    }
    MyMatrix<T> EXT1 = ReadMatrix<T>(is);
    int n_row1 = EXT1.rows();
    std::vector<T> Vdiag1(n_row1);
    for (int i = 0; i < n_row1; i++) {
      T val;
      is >> val;
      Vdiag1[i] = val;
    }
    //
    is >> nbMat;
    std::vector<MyMatrix<T>> ListMat2;
    for (int iMat = 0; iMat < nbMat; iMat++) {
      MyMatrix<T> eMat = ReadMatrix<T>(is);
      ListMat2.push_back(eMat);
    }
    MyMatrix<T> EXT2 = ReadMatrix<T>(is);
    int n_row2 = EXT2.rows();
    std::vector<T> Vdiag2(n_row2);
    for (int i = 0; i < n_row2; i++) {
      T val;
      is >> val;
      Vdiag2[i] = val;
    }
    //
    const bool use_scheme = true;
    std::optional<std::vector<Tidx>> PairTest =
        TestEquivalence_ListMat_Vdiag<T, Tidx, use_scheme>(
            EXT1, ListMat1, Vdiag1, EXT2, ListMat2, Vdiag2);
    //
    std::ofstream os(argv[2]);
    if (!PairTest) {
      os << "return false;\n";
    } else {
      const std::vector<Tidx> &V = *PairTest;
      int n_rows = EXT1.rows();
      os << "return [";
      for (int i = 0; i < n_rows; i++) {
        if (i > 0)
          os << ",";
        os << (V[i] + 1);
      }
      os << "];\n";
    }
  } catch (TerminalException const &e) {
    exit(e.eVal);
  }
}
