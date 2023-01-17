// Copyright (C) 2022 Mathieu Dutour Sikiric <mathieu.dutour@gmail.com>
#include "NumberTheory.h"
#include "POLY_Hyperplane.h"

int main(int argc, char *argv[]) {
  SingletonTime time1;
  try {
    if (argc != 3) {
      std::cerr << "Number of argument is = " << argc << "\n";
      std::cerr << "This program is used as\n";
      std::cerr << "POLY_GapFindHyperplaneRegions [DATAIN] [DATAOUT]\n";
      std::cerr << "\n";
      std::cerr << "DATAIN : The polyhedral cone inequalities\n";
      std::cerr << "DATAOUT : The list of irredundant facets\n";
      return -1;
    }
    //
    std::ifstream is(argv[1]);
    using T = mpq_class;
    MyMatrix<T> ListV = ReadMatrix<T>(is);
    //
    vectface ListFace = EnumerateHyperplaneRegions(ListV);
    int n_vect = ListV.rows();
    std::cerr << "len(ListFace)=" << ListFace.size() << "\n";
    //
    std::ofstream os(argv[2]);
    os << "return [";
    int nbFace = ListFace.size();
    for (int i = 0; i < nbFace; i++) {
      if (i > 0)
        os << ",\n";
      //
      Face f = ListFace[i];
      std::vector<int> f_vect;
      for (int i_vect = 0; i_vect < n_vect; i_vect++)
        if (f[i_vect])
          f_vect.push_back(i_vect + 1);
      os << "[";
      for (size_t u = 0; u < f_vect.size(); u++) {
        if (u > 0)
          os << ",";
        os << f_vect[u];
      }
      os << "]";
    }
    os << "];\n";
    std::cerr << "Normal termination of the program\n";
  } catch (TerminalException const &e) {
    std::cerr << "Error in POLY_GapFindHyperplaneRegions\n";
    exit(e.eVal);
  }
  runtime(time1);
}
