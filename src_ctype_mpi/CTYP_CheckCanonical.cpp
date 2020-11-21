#include "MAT_Matrix.h"
#include "NumberTheory.h"
#include "MatrixCanonicalForm.h"
#include "Temp_PolytopeEquiStab.h"


int main(int argc, char* argv[])
{
  try {
    if (argc != 2) {
      std::cerr << "CTYP_MakeInitialFile [FileIn]\n";
      throw TerminalException{1};
    }
    using Tmat=mpz_class;
    //
    std::string FileIn  = argv[1];
    //
    std::ifstream is(FileIn);
    int nbType;
    is >> nbType;
    auto get_random_equivalent=[&](MyMatrix<Tmat> const& eMat) -> MyMatrix<Tmat> {
      int nbRow = eMat.rows();
      int n = eMat.cols();
      std::vector<int> ePerm = RandomPermutation(nbRow);
      std::vector<int> AttV(nbRow,0);
      std::cerr << "ePerm=[";
      for (int iRow=0; iRow<nbRow; iRow++) {
        if (iRow > 0)
          std::cerr << ",";
        std::cerr << ePerm[iRow];
        AttV[ePerm[iRow]] += 1;
      }
      std::cerr << "]\n";
      for (int iRow=0; iRow<nbRow; iRow++) {
        if (AttV[iRow] != 1) {
          std::cerr << "Error in ePerm\n";
          throw TerminalException{1};
        }
      }
      MyMatrix<Tmat> eUnimod = RandomUnimodularMatrix<Tmat>(n);
      MyMatrix<Tmat> eProd = eMat * eUnimod;
      MyMatrix<Tmat> RetMat(nbRow, n);
      for (int iRow=0; iRow<nbRow; iRow++) {
        int jRow = ePerm[iRow];
        for (int i=0; i<n; i++)
          RetMat(iRow, i) = eProd(jRow, i);
      }
      SignRenormalizationMatrix(RetMat);
      return RetMat;
    };
    for (int iType=0; iType<nbType; iType++) {
      std::cerr << "iType : " << iType << " / " << nbType << "\n";
      MyMatrix<Tmat> eMat1 = ReadMatrix<Tmat>(is);
      MyMatrix<Tmat> eMat1_Can = LinPolytopeAntipodalIntegral_CanonicForm<Tmat>(eMat1);
      for (int i=0; i<10; i++) {
        MyMatrix<Tmat> eMat2 = get_random_equivalent(eMat1);
        MyMatrix<Tmat> eMat2_Can = LinPolytopeAntipodalIntegral_CanonicForm<Tmat>(eMat2);
        if (!TestEqualityMatrix(eMat1_Can, eMat2_Can)) {
          std::cerr << "Inconsistency in the canonical code\n";
          std::cerr << "Mat1_Can=\n";
          WriteMatrix(std::cerr, eMat1_Can);
          std::cerr << "Mat2_Can=\n";
          WriteMatrix(std::cerr, eMat2_Can);
          throw TerminalException{1};
        }
      }
    }
  }
  catch (TerminalException const& e) {
    exit(e.eVal);
  }
}
