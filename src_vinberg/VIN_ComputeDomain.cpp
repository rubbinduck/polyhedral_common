#include "Permlib_specific.h"
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
    auto print=[&](std::ostream & os) -> void {
      int nVect = ListRoot.size();
      os << "|ListRoot|=" << nVect << "\n";
      for (int i=0; i<nVect; i++)
        WriteVector(os, ListRoot[i]);
      Print_DataReflectionGroup(data, os);
    };
    //
    if (argc == 2) {
      print(std::cerr);
    } else {
      std::string FileO = argv[2];
      std::ofstream os(FileO);
      print(os);
    }
  }
  catch (TerminalException const& e) {
    exit(e.eVal);
  }
}
