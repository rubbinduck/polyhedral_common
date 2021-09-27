#include "MAT_Matrix.h"
#include "NumberTheory.h"
#include "MatrixCanonicalForm.h"
#include "Temp_PolytopeEquiStab.h"
#include "vinberg_code.h"


int main(int argc, char* argv[])
{
  try {
    if (argc != 3 && argc != 2) {
      std::cerr << "VIN_ComputeDomain [FileI] [FileO]\n";
      std::cerr << "or\n";
      std::cerr << "VIN_ComputeDomain [FileI]\n";
      throw TerminalException{1};
    }
    using T=mpq_class;
    using Tint=mpz_class;

    std::string FileI = argv[1];
    std::ifstream is(FileI);
    //
    MyMatrix<Tint> G = ReadMatrix<Tint>(is);
    std::cerr << "We have G\n";
    MyMatrix<Tint> v0 = ReadVector<Tint>(is);
    std::cerr << "We have v0\n";
    VinbergTot<T,Tint> Vtot = GetVinbergAux<T,Tint>(G, v0);
    std::cerr << "We have Vtot\n";
    //
    std::vector<MyVector<Tint>> ListRoot = FindRoots(Vtot);
    DataReflectionGroup<T,Tint> data = GetDataReflectionGroup<T,Tint>(ListRoot, G);
    //
    if (argc == 2) {
      Print_DataReflectionGroup(data, std::cerr);
    } else {
      std::string FileO = argv[2];
      std::ofstream os(FileO);
      Print_DataReflectionGroup(data, os);
    }
  }
  catch (TerminalException const& e) {
    exit(e.eVal);
  }
}
