#include "MAT_Matrix.h"
#include "NumberTheory.h"
#include "MatrixCanonicalForm.h"
#include "Temp_PolytopeEquiStab.h"
#include "vinberg_code.h"


int main(int argc, char* argv[])
{
  try {
    if (argc != 4 && argc != 2) {
      std::cerr << "LORENTZ_FundDomain_Vinberg [FileI] [mode] [FileO]\n";
      std::cerr << "or\n";
      std::cerr << "LORENTZ_FundDomain_Vinberg [FileI]\n";
      throw TerminalException{1};
    }
    using T=mpq_class;
    using Tint=mpz_class;

    std::string FileI = argv[1];
    std::ifstream is(FileI);
    //
    MyMatrix<Tint> G = ReadMatrix<Tint>(is);
    std::cerr << "We have G\n";
    MyMatrix<T> G_T=UniversalMatrixConversion<T,Tint>(G);
    DiagSymMat<T> DiagInfo = DiagonalizeNonDegenerateSymmetricMatrix(G_T);
    if (DiagInfo.nbZero != 0 || DiagInfo.nbMinus != 1) {
      std::cerr << "We have nbZero=" << DiagInfo.nbZero << " nbPlus=" << DiagInfo.nbPlus << " nbMinus=" << DiagInfo.nbMinus << "\n";
      std::cerr << "In the hyperbolic geometry we should have nbZero=0 and nbMinus=1\n";
      throw TerminalException{1};
    }
    MyMatrix<Tint> v0 = ReadVector<Tint>(is);
    std::cerr << "We have v0\n";
    VinbergTot<T,Tint> Vtot = GetVinbergAux<T,Tint>(G, v0);
    std::cerr << "We have Vtot\n";
    //
    std::vector<MyVector<Tint>> ListRoot = FindRoots(Vtot);
    DataReflectionGroup<T,Tint> data = GetDataReflectionGroup<T,Tint>(ListRoot, G);
    //
    std::string mode = "text";
    if (argc == 2) {
      Print_DataReflectionGroup(data, mode, std::cerr);
    } else {
      mode = argv[2];
      //
      std::string FileO = argv[3];
      std::ofstream os(FileO);
      Print_DataReflectionGroup(data, mode, os);
    }
  }
  catch (TerminalException const& e) {
    exit(e.eVal);
  }
}