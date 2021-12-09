#include "Permutation.h"
#include "Group.h"
#include "GRP_GroupFct.h"
#include "Temp_PolytopeEquiStab.h"

int main(int argc, char *argv[])
{
  try {
    if (argc != 2 && argc != 3) {
      std::cerr << "Number of argument is = " << argc << "\n";
      std::cerr << "This program is used as\n";
      std::cerr << "GRP_ListMat_Subset_EXT_Automorphism [INfile] [OUTfile]\n";
      std::cerr << "        or\n";
      std::cerr << "GRP_ListMat_Subset_EXT_Automorphism [INfile]\n";
      std::cerr << "\n";
      std::cerr << "INfile    : The file containing the group\n";
      std::cerr << "OUTfile   : The file containing the two pairs\n";
      return -1;
    }
    //    using T = mpz_class;
    using T = long;
    using Tidx = int32_t;
    using Telt = permutalib::SingleSidedPerm<Tidx>;
    using Tint = mpz_class;
    using Tgroup = permutalib::Group<Telt,Tint>;
    //
    std::cerr << "GRP_ComputeAut_ListMat_Subset_EXT : Reading input\n";
    //
    std::ifstream is(argv[1]);
    int nbMat;
    is >> nbMat;
    std::vector<MyMatrix<T>> ListMat;
    for (int iMat=0; iMat<nbMat; iMat++) {
      MyMatrix<T> eMat = ReadMatrix<T>(is);
      ListMat.push_back(eMat);
    }
    MyMatrix<T> EXT = ReadMatrix<T>(is);
    int n_rows= EXT.rows();
    std::cerr << "n_rows=" << n_rows << "\n";
    Face eSubset = ReadFace(is);
    std::cerr << "|eSubset|=" << eSubset.size() << " / " << eSubset.count() << "\n";
    //
    const bool use_scheme = true;
    std::vector<std::vector<Tidx>> ListGen = GetListGenAutomorphism_ListMat_Subset<T,Tidx,use_scheme>(EXT, ListMat, eSubset);
    //
    std::vector<Telt> LGen;
    for (auto & eList : ListGen)
      LGen.emplace_back(std::move(Telt(std::move(eList))));
    Tgroup GRP(LGen, n_rows);
    std::cerr << "|GRP|=" << GRP.size() << "\n";
    if (argc == 3) {
      std::ofstream os(argv[2]);
      os << "return " << GRP.GapString() << ";\n";
    }
  }
  catch (TerminalException const& e) {
    exit(e.eVal);
  }
}
