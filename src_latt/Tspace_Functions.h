// Copyright (C) 2022 Mathieu Dutour Sikiric <mathieu.dutour@gmail.com>
#ifndef SRC_LATT_TSPACE_FUNCTIONS_H_
#define SRC_LATT_TSPACE_FUNCTIONS_H_

// clang-format off
#include "POLY_PolytopeFct.h"
#include "ShortestUniversal.h"
#include "Temp_PolytopeEquiStab.h"
#include "Temp_Positivity.h"
#include <set>
#include <vector>
// clang-format on

#ifdef DEBUG
#define DEBUG_TSPACE_GENERAL
#endif


/*
  By a T-space, we mean a vector space which intersect the cone of positive definite
  matrices. We are interested here only on those positive definite elements.
  ---
  The motivation for this kind of construction is for finding good packing, covering, etc.
  The idea is that the smaller dimensionality allows for a smaller number of
  combinatorial possibilities.
  ---
  However, if the total number of combinatorial possibilities is expected to be smaller,
  the combinatorial complexity on programming and thinking is definitely higher.
  The thing that are considered in this section:
  * The generation of the T-spaces
  * The computation of stabilizer and equivalence of positive definite matrices in
  the T-space.
  ---



 */





/*
  The structure of a T-space (see the papers by Mathieu Dutour Sikiric,
  Frank Vallentin and Achill Schuermann).

  We have the following:
  * A SuperMat is a positive definite matrix in the T-space.
  * The ListMat is a basis of matrices of the T-space.
  * The ListLineMat is the line matrix for effective computation.
  * The ListComm is a set of matrices which are not necessarily of
    determinant 1. The group of symmetries has to commutes with the
    elements in ListComm.
 */
template <typename T> struct LinSpaceMatrix {
  int n;
  MyMatrix<T> SuperMat;
  std::vector<MyMatrix<T>> ListMat;
  std::vector<std::vector<T>> ListLineMat;
  std::vector<MyMatrix<T>> ListComm;
};

template<typename T>
LinSpaceMatrix<T> BuildLinSpace(MyMatrix<T> const& SuperMat, std::vector<MyMatrix<T>> const& ListMat, std::vector<MyMatrix<T>> const& ListComm) {
  int n = SuperMat.rows();
  std::vector<std::vector<T>> ListLineMat;
  for (auto & eMat : ListMat) {
    std::vector<T> eV = GetLineVector(eMat);
    ListLineMat.push_back(eV);
  }
  return {n, SuperMat, ListMat, ListLineMat, ListComm};
}


template<typename T>
MyMatrix<T> GetRandomPositiveDefinite(LinSpaceMatrix<T> const& LinSpa) {
  int n = LinSpa.n;
  MyMatrix<T> TheMat = ZeroMatrix<T>(n, n);
  int N = 2;
  for (auto & eMat : LinSpa.ListMat) {
    int coef = random() % (2 * N + 1) - N;
    TheMat += coef * eMat;
  }
  while(true) {
    if (IsPositiveDefinite(TheMat)) {
      return TheMat;
    }
    TheMat += LinSpa.SuperMat;
  }
}

template <typename T>
MyMatrix<T> GetMatrixFromBasis(std::vector<MyMatrix<T>> const &ListMat,
                               MyVector<T> const &eVect) {
  int n = ListMat[0].rows();
  MyMatrix<T> RetMat = ZeroMatrix<T>(n, n);
  int nbMat = ListMat.size();
  int siz = eVect.size();
  if (siz != nbMat) {
    std::cerr << "Error in GetMatrixFromBasis\n";
    std::cerr << "  siz=" << siz << "\n";
    std::cerr << "nbMat=" << nbMat << "\n";
    std::cerr << "But they should be both equal\n";
    throw TerminalException{1};
  }
  for (int iMat = 0; iMat < nbMat; iMat++)
    RetMat += eVect(iMat) * ListMat[iMat];
  return RetMat;
}

template <typename T>
MyMatrix<T> LINSPA_GetMatrixInTspace(LinSpaceMatrix<T> const &LinSpa,
                                     MyVector<T> const &eVect) {
  return GetMatrixFromBasis(LinSpa.ListMat, eVect);
}

template <typename T>
MyVector<T> LINSPA_GetVectorOfMatrixExpression(LinSpaceMatrix<T> const &LinSpa,
                                               MyMatrix<T> const &eMat) {
  MyVector<T> eMatVect = SymmetricMatrixToVector(eMat);
  int dimSymm = eMatVect.size();
  int dimLinSpa = LinSpa.ListMat.size();
  MyMatrix<T> TotalMatrix(dimLinSpa, dimSymm);
  for (int iLinSpa = 0; iLinSpa < dimLinSpa; iLinSpa++) {
    MyVector<T> V = SymmetricMatrixToVector(LinSpa.ListMat[iLinSpa]);
    AssignMatrixRow(TotalMatrix, iLinSpa, V);
  }
  std::optional<MyVector<T>> RecSol = SolutionMat(TotalMatrix, eMatVect);
  MyVector<T> V = unfold_opt(RecSol, "Failure in SolutionMat");
  return V;
}

template <typename T> T T_GRAM_GetUpperBound(MyMatrix<T> const &TheMat) {
  T TheLowEst = 0;
  int n = TheMat.rows();
  for (int i = 0; i < n; i++) {
    T eVal = TheMat(i, i);
    if (eVal > TheLowEst)
      TheLowEst = eVal;
  }
  T MaxNorm = TheLowEst;
  return MaxNorm;
}

template <typename T>
void T_UpdateListValue(MyMatrix<T> const &eMat, T const &TheTol,
                       std::vector<T> &ListVal) {
  int nbRow = eMat.nbRow;
  int nbCol = eMat.nbCol;
  for (int iRow = 0; iRow < nbRow; iRow++)
    for (int iCol = 0; iCol < nbCol; iCol++) {
      bool WeFound = false;
      int nb = ListVal.size();
      T fVal = eMat(iRow, iCol);
      for (int iVal = 0; iVal < nb; iVal++) {
        T hVal = ListVal[iVal];
        if (T_abs(hVal - fVal) < TheTol)
          WeFound = true;
      }
      if (!WeFound)
        ListVal.push_back(fVal);
    }
}

template <typename T>
MyMatrix<T> T_GRAM_GetScalProdMat(MyMatrix<T> const &eMat,
                                  MyMatrix<int> const &ListShort) {
  int nbShort = ListShort.rows();
  int n = ListShort.cols();
  MyMatrix<T> ScalProdMat(nbShort, nbShort);
  for (int iShort = 0; iShort < nbShort; iShort++)
    for (int jShort = 0; jShort < nbShort; jShort++) {
      T eScal = 0;
      for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
          int eVal12 = ListShort(jShort, j) * ListShort(iShort, i);
          T eVal = eMat(i, j);
          eScal += eVal12 * eVal;
        }
      ScalProdMat(iShort, jShort) = eScal;
    }
  return ScalProdMat;
}

template <typename T, typename Tint, typename Tgroup>
void T_GetGramMatrixAutomorphismGroup(
    MyMatrix<T> const &eMat, Tgroup &GRPperm,
    std::vector<MyMatrix<Tint>> &ListMatrGens) {
  using Tidx_value = int16_t;
  T MaxDet = T_GRAM_GetUpperBound(eMat);
  MyMatrix<Tint> ListShort = T_ShortVector(eMat, MaxDet);
  MyMatrix<T> ListShort_T = UniversalMatrixConversion<T, Tint>(ListShort);
  WeightMatrix<true, T, Tidx_value> WMat =
      T_TranslateToMatrix_QM_SHV<T, Tint, Tidx_value>(eMat, ListShort);
  GRPperm = GetStabilizerWeightMatrix(WMat);
  ListMatrGens.clear();
  for (auto &eGen : GRPperm.group->S) {
    MyMatrix<T> M3_T =
        RepresentVertexPermutation(ListShort_T, ListShort_T, *eGen);
    MyMatrix<Tint> M3_I = UniversalMatrixConversion<Tint, T>(M3_T);
    ListMatrGens.push_back(M3_I);
  }
}

template <typename T, typename Tint, typename Telt>
bool T_TestGramMatrixEquivalence(MyMatrix<T> const &eMat1,
                                 MyMatrix<T> const &eMat2) {
  using Tidx_value = int16_t;
  T MaxDet1 = T_GRAM_GetUpperBound(eMat1);
  T MaxDet2 = T_GRAM_GetUpperBound(eMat2);
  T MaxDet = MaxDet1;
  if (MaxDet1 > MaxDet2)
    MaxDet = MaxDet2;
  MyMatrix<Tint> ListShort1 = T_ShortVector(eMat1, MaxDet);
  MyMatrix<Tint> ListShort2 = T_ShortVector(eMat2, MaxDet);
  WeightMatrix<true, T, Tidx_value> WMat1 =
      T_TranslateToMatrix_QM_SHV<T, Tint, Tidx_value>(eMat1, ListShort1);
  WeightMatrix<true, T, Tidx_value> WMat2 =
      T_TranslateToMatrix_QM_SHV<T, Tint, Tidx_value>(eMat2, ListShort2);
  std::optional<Telt> eResEquiv =
      TestEquivalenceWeightMatrix<T, Telt>(WMat1, WMat2);
  return eResEquiv;
}

/*
  Compute an invariant of the gram matrix
  so that it does not change after arithmetic
  equivalence.
  However, it is not invariant under scaling.
*/
template <typename T, typename Tint>
size_t GetInvariantGramShortest(MyMatrix<T> const &eGram,
                                MyMatrix<Tint> const &SHV,
                                size_t const& seed,
                                [[maybe_unused]] std::ostream & os) {
  T eDet = DeterminantMat(eGram);
  int nbVect = SHV.rows();
#ifdef DEBUG_TSPACE_GENERAL
  os << "eDet=" << eDet << " nbVect=" << nbVect << "\n";
#endif
  std::vector<MyVector<T>> ListV;
  for (int iVect=0; iVect<nbVect; iVect++) {
    MyVector<Tint> V = GetMatrixRow(SHV, iVect);
    MyVector<T> V_T = UniversalVectorConversion<T,Tint>(V);
    ListV.push_back(V_T);
  }
  std::map<T,size_t> map_diag, map_off_diag;
  for (int iVect = 0; iVect < nbVect; iVect++) {
    MyVector<T> Vprod = eGram * ListV[iVect];
    T eNorm = Vprod.dot(ListV[iVect]);
    map_diag[eNorm] += 1;
    for (int jVect=iVect+1; jVect<nbVect; jVect++) {
      T eScal = Vprod.dot(ListV[jVect]);
      map_off_diag[eScal] += 1;
    }
  }
  auto combine_hash = [](size_t &seed, size_t new_hash) -> void {
    seed ^= new_hash + 0x9e3779b8 + (seed << 6) + (seed >> 2);
  };
  size_t hash_ret = seed;
  size_t hash_det = std::hash<T>()(eDet);
  combine_hash(hash_ret, hash_det);
  for (auto & kv : map_diag) {
    size_t hash_T = std::hash<T>()(kv.first);
    combine_hash(hash_ret, hash_T);
    combine_hash(hash_ret, kv.second);
  }
  for (auto & kv : map_off_diag) {
    size_t hash_T = std::hash<T>()(kv.first);
    combine_hash(hash_ret, hash_T);
    combine_hash(hash_ret, kv.second);
  }
  return hash_ret;
}

// clang-format off
#endif  // SRC_LATT_TSPACE_FUNCTIONS_H_
// clang-format on