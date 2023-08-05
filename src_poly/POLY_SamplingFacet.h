// Copyright (C) 2022 Mathieu Dutour Sikiric <mathieu.dutour@gmail.com>
#ifndef SRC_POLY_POLY_SAMPLINGFACET_H_
#define SRC_POLY_POLY_SAMPLINGFACET_H_

#include "POLY_LinearProgramming.h"
#include "POLY_PolytopeFct.h"
#include "POLY_lrslib.h"
#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

struct recSamplingOption {
  int critlevel;
  int maxnbcall;
  int maxnbsize;
  std::string prog;
};

template <typename T>
vectface Kernel_DUALDESC_SamplingFacetProcedure(
    MyMatrix<T> const &EXT, recSamplingOption const &eOption, int &nbCall) {
  int dim = RankMat(EXT);
  int len = EXT.rows();
  std::string prog = eOption.prog;
  int critlevel = eOption.critlevel;
  int maxnbcall = eOption.maxnbcall;
  int maxnbsize = eOption.maxnbsize;
  std::cerr << "critlevel=" << critlevel << " prog=" << prog
            << " maxnbcall=" << maxnbcall << "\n";
  auto IsRecursive = [&]() -> bool {
    if (len < critlevel)
      return false;
    if (dim < 15)
      return false;
    return true;
  };
  bool DoRecur = IsRecursive();
  vectface ListFace(EXT.rows());
  std::vector<int> ListStatus;
  auto FuncInsert = [&](Face const &eFace) -> void {
    for (auto &fFace : ListFace) {
      if (fFace.count() == eFace.count())
        return;
    }
    ListFace.push_back(eFace);
    ListStatus.push_back(0);
  };
  std::cerr << "dim=" << dim << "  len=" << len << "\n";
  if (!DoRecur) {
    auto comp_dd = [&]() -> vectface {
      if (prog == "lrs")
        return lrs::DualDescription_incd(EXT);
      if (prog == "cdd")
        return cdd::DualDescription_incd(EXT);
      std::cerr << "Failed to find a matching program\n";
      throw TerminalException{1};
    };
    vectface ListIncd = comp_dd();
    for (auto &eFace : ListIncd)
      FuncInsert(eFace);
    std::cerr << "DirectDualDesc |ListFace|=" << ListFace.size() << "\n";
    nbCall++;
    return ListFace;
  }
  Face eInc = FindOneInitialVertex(EXT);
  FuncInsert(eInc);
  while (true) {
    int nbCases = ListFace.size();
    bool IsFinished = true;
    for (int iC = 0; iC < nbCases; iC++)
      if (ListStatus[iC] == 0) {
        // we liberally increase the nbCall value
        nbCall++;
        IsFinished = false;
        ListStatus[iC] = 1;
        Face eFace = ListFace[iC];
        MyMatrix<T> EXTred = SelectRow(EXT, eFace);
        vectface ListRidge =
            Kernel_DUALDESC_SamplingFacetProcedure(EXTred, eOption, nbCall);
        for (auto &eRidge : ListRidge) {
          Face eFlip = ComputeFlipping(EXT, eFace, eRidge);
          FuncInsert(eFlip);
        }
        if (maxnbsize != -1) {
          int siz = ListFace.size();
          if (maxnbsize > siz) {
            std::cerr << "Ending by maxsize criterion\n";
            std::cerr << "siz=" << siz << " maxnbsize=" << maxnbsize << "\n";
            return ListFace;
          }
        }
        if (maxnbcall != -1) {
          if (nbCall > maxnbcall) {
            std::cerr << "Ending by maxnbcall\n";
            return ListFace;
          }
        }
      }
    if (IsFinished)
      break;
  }
  std::cerr << "RecursiveDualDesc |ListFace|=" << ListFace.size() << "\n";
  return ListFace;
}

template <typename T>
vectface
DUALDESC_SamplingFacetProcedure(MyMatrix<T> const &EXT,
                                std::vector<std::string> const &ListOpt) {
  std::string prog = "lrs";
  int critlevel = 50;
  int maxnbcall = -1;
  int maxnbsize = 20;
  for (auto &eOpt : ListOpt) {
    std::vector<std::string> ListStrB = STRING_Split(eOpt, "_");
    if (ListStrB.size() == 2) {
      if (ListStrB[0] == "prog")
        prog = ListStrB[1];
      if (ListStrB[0] == "critlevel")
        std::istringstream(ListStrB[1]) >> critlevel;
      if (ListStrB[0] == "maxnbcall")
        std::istringstream(ListStrB[1]) >> maxnbcall;
      if (ListStrB[0] == "maxnbsize")
        std::istringstream(ListStrB[1]) >> maxnbsize;
    }
  }
  if (prog != "lrs" && prog != "cdd") {
    std::cerr << "We have prog=" << prog << "\n";
    std::cerr << "but the only allowed input formats are lrs and cdd\n";
    throw TerminalException{1};
  }
  recSamplingOption eOption;
  eOption.maxnbcall = maxnbcall;
  eOption.prog = prog;
  eOption.critlevel = critlevel;
  eOption.maxnbsize = maxnbsize;
  int nbcall = 0;
  return Kernel_DUALDESC_SamplingFacetProcedure(EXT, eOption, nbcall);
}

template <typename T>
vectface Kernel_DirectComputationInitialFacetSet(MyMatrix<T> const &EXT,
                                                 std::string const &ansSamp,
                                                 std::ostream &os) {
  os << "DirectComputationInitialFacetSet ansSamp=" << ansSamp << "\n";
  std::vector<std::string> ListStr = STRING_Split(ansSamp, ":");
  std::string ansOpt = ListStr[0];
  auto get_iter = [&]() -> int {
    int iter = 10;
    if (ListStr.size() > 1) {
      std::vector<std::string> ListStrB = STRING_Split(ListStr[1], "_");
      if (ListStrB.size() == 2 && ListStrB[0] == "iter")
        std::istringstream(ListStrB[1]) >> iter;
    }
    return iter;
  };
  auto compute_samp = [&]() -> vectface {
    if (ansOpt == "lp_cdd") {
      // So possible format is lp_cdd:iter_100
      int iter = get_iter();
      return FindVertices(EXT, iter);
    }
    if (ansOpt == "lp_cdd_min") {
      // So possible format is lp_cdd_min:iter_100
      int iter = get_iter();
      vectface vf = FindVertices(EXT, iter);
      return select_minimum_count(vf);
    }
    if (ansOpt == "sampling") {
      std::vector<std::string> ListOpt;
      int n_ent = ListStr.size();
      for (int i_ent = 1; i_ent < n_ent; i_ent++)
        ListOpt.push_back(ListStr[i_ent]);
      return DUALDESC_SamplingFacetProcedure(EXT, ListOpt);
    }
    if (ansOpt == "lrs_limited") {
      int upperlimit = 100;
      // So possible format is lrs_limited:upperlimit_1000
      if (ListStr.size() > 1) {
        std::vector<std::string> ListStrB = STRING_Split(ListStr[1], "_");
        if (ListStrB.size() == 2 && ListStrB[0] == "upperlimit")
          std::istringstream(ListStrB[1]) >> upperlimit;
      }
      return lrs::DualDescription_incd_limited(EXT, upperlimit);
    }
    std::cerr << "No right program found\n";
    std::cerr << "Let us die\n";
    throw TerminalException{1};
  };
  vectface ListIncd = compute_samp();
  if (ListIncd.size() == 0) {
    std::cerr << "We found 0 facet and that is not good\n";
    throw TerminalException{1};
  }
  return ListIncd;
}

template <typename T>
inline typename std::enable_if<is_ring_field<T>::value, vectface>::type
DirectComputationInitialFacetSet(MyMatrix<T> const &EXT,
                                 std::string const &ansSamp, std::ostream &os) {
  return Kernel_DirectComputationInitialFacetSet(EXT, ansSamp, os);
}

template <typename T>
inline typename std::enable_if<!is_ring_field<T>::value, vectface>::type
DirectComputationInitialFacetSet(MyMatrix<T> const &EXT,
                                 std::string const &ansSamp, std::ostream &os) {
  using Tfield = typename overlying_field<T>::field_type;
  MyMatrix<Tfield> EXTfield = UniversalMatrixConversion<Tfield, T>(EXT);
  return Kernel_DirectComputationInitialFacetSet(EXTfield, ansSamp, os);
}

template <typename T> vectface Kernel_GetFullRankFacetSet(const MyMatrix<T> &EXT, std::ostream& os) {
  size_t dim = EXT.cols();
  size_t n_rows = EXT.rows();
  if (dim == 2) {
    if (n_rows != 2) {
      std::cerr << "In dimension 2, the cone should have exactly two extreme rays\n";
      throw TerminalException{1};
    }
    vectface vf_ret(2);
    Face f1(2), f2(2);
    f1[0] = 1;
    f2[1] = 1;
    vf_ret.push_back(f1);
    vf_ret.push_back(f2);
    return vf_ret;
  }
  size_t nb = 1;
  vectface ListSets = Kernel_FindVertices(EXT, nb);
  Face eSet = ListSets[0];
  MyMatrix<T> EXTsel = ColumnReduction(SelectRow(EXT, eSet));
  os << "|EXTsel|=" << EXTsel.rows() << " / " << EXTsel.cols()
            << " rnk=" << RankMat(EXTsel) << "\n";
  vectface ListRidge = Kernel_GetFullRankFacetSet(EXTsel, os);
  os << "We have ListRidge\n";
  FlippingFramework<T> RPLlift(EXT, eSet);
  os << "We have FlippingFramework\n";
  vectface vf_ret(n_rows);
  vf_ret.push_back(eSet);
  for (auto &eRidge : ListRidge) {
    Face eFace = RPLlift.FlipFace(eRidge);
    vf_ret.push_back(eFace);
  }
  os << "We have vf_ret\n";
#ifdef DEBUG
  MyMatrix<T> FACsamp(vf_ret.size(), dim);
  int pos = 0;
  for (auto & face : vf_ret) {
    MyVector<T> eFAC = FindFacetInequality(EXT, face);
    AssignMatrixRow(FACsamp, pos, eFAC);
    pos++;
  }
  if (RankMat(FACsamp) != static_cast<int>(dim)) {
    std::cerr << "Failed to find a ful rank vector configuration\n";
    throw TerminalException{1};
  }
#endif
  return vf_ret;
}

template <typename T> vectface GetFullRankFacetSet(const MyMatrix<T> &EXT, std::ostream& os) {
  MyMatrix<T> EXT_B = ColumnReduction(EXT);
  MyMatrix<T> EXT_C = Polytopization(EXT_B);
  MyMatrix<T> EXT_D = SetIsobarycenter(EXT_C);
  return Kernel_GetFullRankFacetSet(EXT_D, os);
}


// clang-format off
#endif  // SRC_POLY_POLY_SAMPLINGFACET_H_
// clang-format on
