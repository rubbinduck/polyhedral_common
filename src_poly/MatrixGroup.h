#ifndef INCLUDE_MATRIX_GROUP_H
#define INCLUDE_MATRIX_GROUP_H

#include "factorizations.h"
#include "MAT_MatrixInt.h"
#include "GRP_GroupFct.h"
#include "PERM_Fct.h"
#include "MatrixGroupBasic.h"



template<typename T>
struct MatrixGroup {
  int n;
  std::vector<MyMatrix<T>> ListGen;
};

// This is an internal type used for permutation representations
// of matrix groups.
// We hope that the right structures have already been implemented in permlib
// and that it is only a matter of template substritutions
template<typename T, typename Telt>
struct PairPermMat {
public:
  PairPermMat(PairPermMat const& p) = default;

  PairPermMat(Telt const& ePerm, MyMatrix<T> const& eMat) : perm(ePerm), mat(eMat) {}

  Telt GetPerm() const
  {
    return perm;
  }
  MyMatrix<T> GetMat() const
  {
    return mat;
  }
  PairPermMat operator*(PairPermMat const& p) const
  {
    Telt ePerm=perm*p.perm;
    MyMatrix<T> eMat=mat*p.mat;
    return PairPermMat(ePerm, eMat);
  }
  PairPermMat& operator*=(PairPermMat const& p)
  {
    Telt ePerm=perm*p.perm;
    MyMatrix<T> eMat=mat*p.mat;
    perm=ePerm;
    mat=eMat;
    return *this;
  }
  PairPermMat& operator^=(PairPermMat const& p)
  {
    Telt ePerm=p.perm*perm;
    MyMatrix<T> eMat=p.mat*mat;
    perm=ePerm;
    mat=eMat;
    return *this;
  }
  PairPermMat operator~() const
  {
    Telt ePerm=~perm;
    MyMatrix<T> eMat=Inverse(mat);
    return PairPermMat(ePerm, eMat);
  }
  bool operator==(PairPermMat const& p)
  {
    return p.mat==mat;
  }
  bool operator<(PairPermMat const &p) const
  {
    int n=mat.rows();
    for (int i=0; i<n; i++)
      for (int j=0; j<n; j++)
	if (mat(i,j) < p.mat(i,j))
	  return true;
    return false;
  }
  bool isIdentity() const
  {
    int n=mat.rows();
    for (int i=0; i<n; i++)
      for (int j=0; j<n; j++)
	if (i != j && mat(i,j) != 0)
	  return false;
    for (int j=0; j<n; j++)
      if (mat(j,j) != 1)
	  return false;
    return true;
  }
  int at(int const& val) const
  {
    return perm.at(val);
  }
  int size() const
  {
    return perm.size();
  }
private:
  Telt perm;
  MyMatrix<T> mat;
};

// Compute Orbit of an object of type T2 under 
// a group generated by elements of type T1
template<typename T1, typename T2, typename F>
std::vector<T2> OrbitComputation(std::vector<T1> const& ListGen, T2 const& a, const F& f)
{
  std::vector<int> ListStatus;
  std::vector<T2> TheOrbit;
  auto fInsert=[&](T2 const& u) -> void {
    int len=ListStatus.size();
    for (int i=0; i<len; i++)
      if (TheOrbit[i] == u)
	return;
    TheOrbit.push_back(u);
    ListStatus.push_back(0);
  };
  fInsert(a);
  while(true) {
    bool IsFinished=true;
    int len=ListStatus.size();
    for (int i=0; i<len; i++)
      if (ListStatus[i] == 0) {
	IsFinished=false;
	ListStatus[i]=1;
	for (auto & eGen : ListGen) {
	  T2 u=f(TheOrbit[i],eGen);
	  fInsert(u);
	}
      }
    if (IsFinished)
      break;
  }
  return TheOrbit;
}

template<typename T, typename Telt>
struct FiniteMatrixGroup {
  int n;
  MyMatrix<T> EXTfaithAct;
  std::vector<MyMatrix<T>> ListMatrGen;
  std::vector<Telt> ListPermGen;
};




// The space must be defining a finite index subgroup of T^n
template<typename T, typename Tgroup>
FiniteMatrixGroup<T,typename Tgroup::Telt> LinearSpace_ModStabilizer(FiniteMatrixGroup<T,typename Tgroup::Telt> const& GRPmatr, MyMatrix<T> const& TheSpace, T const& TheMod)
{
  using Telt = typename Tgroup::Telt;
  using Tidx = typename Telt::Tidx;
  int n=GRPmatr.n;
  MyMatrix<T> ModSpace=TheMod*IdentityMat<T>(n);
#ifdef DEBUG_MATRIX_GROUP
  std::cerr << "TheMod=" << TheMod << "\n";
  std::cerr << "n=" << n << "\n";
  std::cerr << "TheSpace=\n";
  WriteMatrix(std::cerr, TheSpace);
#endif
  MyMatrix<T> TheSpaceMod=Concatenate(TheSpace, ModSpace);
  auto VectorMod=[&](MyVector<T> const& V) -> MyVector<T> {
    MyVector<T> Vret(n);
    for (int i=0; i<n; i++) {
      T q=QuoInt(V(i), TheMod);
      T nVal=V(i) - q*TheMod;
      //      std::cerr << "i=" << i << " V(i)=" << V(i) << " nVal=" << nVal << "\n";
      Vret(i)=nVal;
    }
    return Vret;
  };
  CanSolIntMat<T> eCan=ComputeCanonicalFormFastReduction(TheSpaceMod);
  std::function<MyVector<T>(MyVector<T> const&,MyMatrix<T> const&)> TheAction=[&](MyVector<T> const& eClass, MyMatrix<T> const& eElt) -> MyVector<T> {
    MyVector<T> eVect=ProductVectorMatrix(eClass, eElt);
    return VectorMod(eVect);
  };
  auto IsStabilizing=[&](MyVector<T> &V, FiniteMatrixGroup<T,Telt> const& TheGRP) -> bool {
    for (int i=0; i<n; i++) {
      MyVector<T> eVect=GetMatrixRow(TheSpace, i);
#ifdef DEBUG_MATRIX_GROUP
      std::cerr << "i=" << i << " |ListMatrGen|=" << TheGRP.ListMatrGen.size() << "\n";
#endif
      for (auto & eGen : TheGRP.ListMatrGen) {
	MyVector<T> eVectG=ProductVectorMatrix(eVect, eGen);
	ResultSolutionIntMat<T> eRes=SolutionIntMat(TheSpaceMod, eVectG);
	bool test=CanTestSolutionIntMat(eCan, eVectG);
	if (test != eRes.TheRes) {
	  std::cerr << "Inconsistency of result between two SolutionIntMat functions\n";
	  throw TerminalException{1};
	}
	if (!eRes.TheRes) {
	  V=VectorMod(eVect);
#ifdef DEBUG_MATRIX_GROUP
	  std::cerr << "eVect=\n";
	  WriteVector(std::cerr, eVect);
	  std::cerr << "V=\n";
	  WriteVector(std::cerr, V);
#endif
	  return false;
	}
      }
    }
    return true;
  };
  FiniteMatrixGroup<T,Telt> GRPret=GRPmatr;
  std::vector<MyVector<T>> ListVect;
  int nbRow=GRPmatr.EXTfaithAct.rows();
#ifdef DEBUG_MATRIX_GROUP
  std::cerr << "|G(EXT)|=" << Tgroup(GRPmatr.ListPermGen, nbRow).size() << "\n";
#endif

  /*  for (int i=0; i<nbRow; i++) {
    MyVector<T> eLine=GetMatrixRow(GRPmatr.EXTfaithAct, i);
    ListVect.push_back(eLine);
    }*/
  while(true) {
    MyVector<T> V;
    bool test=IsStabilizing(V, GRPret);
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "test=" << test << "\n";
#endif
    if (test)
      break;
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "V=\n";
    WriteVector(std::cerr, V);
#endif
    std::vector<MyVector<T>> O=OrbitComputation(GRPret.ListMatrGen, V, TheAction);
    int Osiz=O.size();
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "Osiz=" << Osiz << "\n";
#endif
    int siz=nbRow + Osiz;
    //    std::vector<MyVector<T>> ListWork=ConcatenateVect(ListVect, O);
    //    int siz=ListWork.size();
    //    std::cerr << "siz=" << siz << "\n";
    Telt ePermS=Telt(SortingPerm<MyVector<T>,Tidx>(O));
    Telt ePermSinv=~ePermS;
    /*    std::cerr << "We have ePermS\n";
    for (int i=0; i<Osiz; i++) {
      std::cerr << "i=" << i << "/" << siz << "\n";
      int j=ePermS.at(i);
      WriteVector(std::cerr, O[j]);
      }*/
    std::vector<Telt> ListPermGenProv;
    int nbGen=GRPret.ListMatrGen.size();
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "nbGen=" << nbGen << "\n";
#endif
    for (int iGen=0; iGen<nbGen; iGen++) {
      MyMatrix<T> eMatrGen=GRPret.ListMatrGen[iGen];
      Telt ePermGen=GRPret.ListPermGen[iGen];
      std::vector<Tidx> v(siz);
      Tidx nbRow_tidx=nbRow;
      for (Tidx i=0; i<nbRow_tidx; i++) {
	Tidx j=ePermGen.at(i);
	v[i]=j;
      }
#ifdef DEBUG_MATRIX_GROUP
      std::cerr << "We have initialized the first part\n";
#endif
      std::vector<MyVector<T>> ListImage(Osiz);
      for (int iV=0; iV<Osiz; iV++) {
	MyVector<T> eV=O[iV];
	MyVector<T> fV=TheAction(eV, eMatrGen);
	ListImage[iV]=fV;
      }
#ifdef DEBUG_MATRIX_GROUP
      std::cerr << "We have ListImage\n";
#endif
      Telt ePermB=Telt(SortingPerm<MyVector<T>,Tidx>(ListImage));
      Telt ePermGenSelect=ePermB*ePermSinv;
#ifdef DEBUG_MATRIX_GROUP
      std::cerr << "We have ePermGenSelect\n";
#endif
      for (int iO=0; iO<Osiz; iO++) {
	int jO=ePermGenSelect.at(iO);
	v[nbRow+iO]=nbRow+jO;
      }
      Telt eNewPerm(std::move(v));
#ifdef DEBUG_MATRIX_GROUP
      std::cerr << "We have eNewPerm\n";
#endif
      ListPermGenProv.emplace_back(std::move(eNewPerm));
    }
    Tgroup GRPwork(ListPermGenProv, siz);
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "|GRPwork|=" << GRPwork.size() << "\n";
#endif
    Face eFace(siz);
    for (int iO=0; iO<Osiz; iO++) {
      ResultSolutionIntMat<T> eRes=SolutionIntMat(TheSpaceMod, O[iO]);
      if (eRes.TheRes)
	eFace[nbRow + iO]=1;
    }
    Tgroup eStab=GRPwork.Stabilizer_OnSets(eFace);
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "|eStab|=" << eStab.size() << "\n";
#endif
    std::vector<MyMatrix<T>> ListMatrGen;
    std::vector<Telt> ListPermGen;
    std::vector<Telt> LGen = eStab.GeneratorsOfGroup();
    Tidx nbRow_tidx = nbRow;
    for (auto & eGen : LGen) {
      std::vector<Tidx> v(nbRow);
      for (Tidx i=0; i<nbRow_tidx; i++)
	v[i]=OnPoints(i, eGen);
      Telt ePerm(std::move(v));
      MyMatrix<T> eMatr=FindTransformation(GRPmatr.EXTfaithAct, GRPmatr.EXTfaithAct, ePerm);
      ListPermGen.emplace_back(std::move(ePerm));
      ListMatrGen.emplace_back(std::move(eMatr));
    }
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "All generators have been created\n";
#endif
    GRPret={n, GRPmatr.EXTfaithAct, ListMatrGen, ListPermGen};
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "GRPret has been created\n";
#endif
  }
  return GRPret;
}


template<typename T>
T LinearSpace_GetDivisor(MyMatrix<T> const& TheSpace)
{
  T TheDet=T_abs(DeterminantMat(TheSpace));
  T eDiv=1;
  int n=TheSpace.rows();
  while(true) {
    bool IsOK=true;
    for (int i=0; i<n; i++)
      if (IsOK) {
	MyVector<T> eVect=ZeroVector<T>(n);
	eVect(i)=eDiv;
	bool test=SolutionIntMat(TheSpace, eVect).TheRes;
	if (!test)
	  IsOK=false;
      }
    if (IsOK)
      return eDiv;
    if (eDiv > TheDet) {
      std::cerr << "Clear error in LinearSpace_GetDivisor\n";
      throw TerminalException{1};
    }
    eDiv += 1;
  }
}



template<typename T, typename Tgroup>
FiniteMatrixGroup<T,typename Tgroup::Telt> LinearSpace_Stabilizer(FiniteMatrixGroup<T,typename Tgroup::Telt> const& GRPmatr, MyMatrix<T> const& TheSpace)
{
  using Telt=typename Tgroup::Telt;
  int n=GRPmatr.n;
#ifdef DEBUG_MATRIX_GROUP
  std::cerr << "TheSpace=\n";
  WriteMatrixGAP(std::cerr, TheSpace);
  std::cerr << "det(TheSpace)=" << DeterminantMat(TheSpace) << "\n";
#endif
  auto IsStabilizing=[&](FiniteMatrixGroup<T,Telt> const& TheGRP) -> bool {
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "Begin IsStabilizing test\n";
#endif
    for (int i=0; i<n; i++) {
      MyVector<T> eVect=GetMatrixRow(TheSpace, i);
      for (auto & eGen : TheGRP.ListMatrGen) {
	MyVector<T> eVectG=ProductVectorMatrix(eVect, eGen);
	ResultSolutionIntMat<T> eRes=SolutionIntMat(TheSpace, eVectG);
	if (!eRes.TheRes) {
#ifdef DEBUG_MATRIX_GROUP
	  std::cerr << "Leaving IsStabilzing: false\n";
#endif
	  return false;
	}
      }
    }
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "Leaving IsStabilzing: true\n";
#endif
    return true;
  };
  if (IsStabilizing(GRPmatr))
    return GRPmatr;
  T LFact=LinearSpace_GetDivisor(TheSpace);
  std::vector<T> eList=FactorsInt(LFact);
  int siz=eList.size();
#ifdef DEBUG_MATRIX_GROUP
  std::cerr << "LFact=" << LFact << " siz=" << siz << "\n";
#endif
  FiniteMatrixGroup<T,Telt> GRPret=GRPmatr;
  for (int i=1; i<=siz; i++) {
    T TheMod=1;
    for (int j=0; j<i; j++)
      TheMod *= eList[j];
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "TheMod=" << TheMod << "\n";
#endif
    GRPret=LinearSpace_ModStabilizer<T,Tgroup>(GRPret, TheSpace, TheMod);
    if (IsStabilizing(GRPret))
      return GRPret;
  }
  if (!IsStabilizing(GRPret)) {
    std::cerr << "Error in LinearSpace_Stabilizer\n";
    throw TerminalException{1};
  }
  return GRPret;
}

template<typename T, typename Telt>
struct ResultTestModEquivalence {
  bool TheRes;
  FiniteMatrixGroup<T,Telt> GRPwork;
  MyMatrix<T> eEquiv;
};



template<typename T, typename Tgroup>
ResultTestModEquivalence<T, typename Tgroup::Telt> LinearSpace_ModEquivalence(FiniteMatrixGroup<T, typename Tgroup::Telt> const& GRPmatr, MyMatrix<T> const& TheSpace1, MyMatrix<T> const& TheSpace2, T const& TheMod)
{
  using Telt = typename Tgroup::Telt;
  using Tidx = typename Telt::Tidx;
  int n=TheSpace1.rows();
#ifdef DEBUG_MATRIX_GROUP
  std::cerr << "------------------------------------------------------\n";
  std::cerr << "TheSpace1=\n";
  WriteMatrix(std::cerr, TheSpace1);
  std::cerr << "TheSpace2=\n";
  WriteMatrix(std::cerr, TheSpace2);
  std::cerr << "TheMod=" << TheMod << "\n";
#endif
  MyMatrix<T> ModSpace=TheMod*IdentityMat<T>(n);
  MyMatrix<T> TheSpace1Mod=Concatenate(TheSpace1, ModSpace);
  MyMatrix<T> TheSpace2Mod=Concatenate(TheSpace2, ModSpace);
  auto VectorMod=[&](MyVector<T> const& V) -> MyVector<T> {
    MyVector<T> Vret(n);
    for (int i=0; i<n; i++)
      Vret(i) = ResInt(V(i), TheMod);
    return Vret;
  };
  auto TheAction=[&](MyVector<T> const& eClass, MyMatrix<T> const& eElt) -> MyVector<T> {
    MyVector<T> eVect=ProductVectorMatrix(eClass, eElt);
    return VectorMod(eVect);
  };
  auto IsEquiv=[&](MyVector<T> & V, MyMatrix<T> const& eEquiv) -> bool {
    for (int i=0; i<n; i++) {
      MyVector<T> eVect=GetMatrixRow(TheSpace1, i);
      MyVector<T> eVectG=ProductVectorMatrix(eVect, eEquiv);
      ResultSolutionIntMat<T> eRes=SolutionIntMat(TheSpace2Mod, eVectG);
      if (!eRes.TheRes) {
	V=VectorMod(eVectG);
#ifdef DEBUG_MATRIX_GROUP
        std::cerr << "   i=" << i << "\n";
        std::cerr << "   Found vector eVect=";
        WriteVector(std::cerr, eVect);
        std::cerr << "   Found vector V=";
        WriteVector(std::cerr, V);
        std::cerr << "   Found vector eVectG=";
        WriteVector(std::cerr, eVectG);
        std::cerr << "   eEquiv=\n";
        WriteMatrix(std::cerr, eEquiv);
#endif
	return false;
      }
    }
    return true;
  };
  auto IsStabilizing=[&](FiniteMatrixGroup<T,Telt> const& GRPin) -> bool {
    for (auto &eGen : GRPin.ListMatrGen) {
      for (int i=0; i<n; i++) {
        MyVector<T> eVect=GetMatrixRow(TheSpace2, i);
        MyVector<T> eVectG=ProductVectorMatrix(eVect, eGen);
        ResultSolutionIntMat<T> eRes=SolutionIntMat(TheSpace2Mod, eVectG);
        if (!eRes.TheRes)
          return false;
      }
    }
    return true;
  };
  FiniteMatrixGroup<T,Telt> GRPwork = GRPmatr;
  MyMatrix<T> eElt=IdentityMat<T>(n);
  int nbRow=GRPmatr.EXTfaithAct.rows();
  /*  std::vector<MyVector<T>> ListVect;
  for (int i=0; i<nbRow; i++) {
    MyVector<T> eLine=GetMatrixRow(GRPmatr.EXTfaithAct, i);
    ListVect.push_back(eLine);
    }*/
  size_t iter=0;
  while(true) {
    MyVector<T> V;
    bool test=IsEquiv(V, eElt);
    if (test) {
      if (!IsStabilizing(GRPwork)) { // There is a potential bug here.
        std::cerr << "The GRPwork is not preserving TheSpace2\n";
        throw TerminalException{1};
      }
#ifdef DEBUG_MATRIX_GROUP
      std::cerr << "eElt is correct, we return from here\n";
#endif
      return {true, GRPwork, eElt};
    }
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "V =";
    WriteVector(std::cerr, V);
#endif
    std::vector<MyVector<T>> O=OrbitComputation(GRPwork.ListMatrGen, V, TheAction);
    int Osiz=O.size();
    //    std::vector<MyVector<T>> ListWork=ConcatenateVect(ListVect, O);
    int siz=nbRow + Osiz;
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "Osiz=" << Osiz << " nbRow=" << nbRow << " iter=" << iter << " O=\n";
#endif
    for (auto & eV : O)
      WriteVector(std::cerr, eV);
    iter++;
    Telt ePermS=Telt(SortingPerm<MyVector<T>,Tidx>(O));
    Telt ePermSinv=~ePermS;
    std::vector<Telt> ListPermGenProv;
    int nbGen=GRPwork.ListMatrGen.size();
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "nbGen=" << nbGen << "\n";
#endif
    for (int iGen=0; iGen<nbGen; iGen++) {
      MyMatrix<T> eMatrGen=GRPwork.ListMatrGen[iGen];
      Telt ePermGen=GRPwork.ListPermGen[iGen];
      std::vector<Tidx> v(siz);
      Tidx nbRow_tidx=nbRow;
      for (Tidx i=0; i<nbRow_tidx; i++)
	v[i] = ePermGen.at(i);
      std::vector<MyVector<T>> ListImage;
      for (auto & eV : O)
	ListImage.push_back(TheAction(eV, eMatrGen));
      Telt ePermB=Telt(SortingPerm<MyVector<T>,Tidx>(ListImage));
      Telt ePermGenSelect=ePermB*ePermSinv;
      for (int iO=0; iO<Osiz; iO++) {
	Tidx jO=ePermGenSelect.at(iO);
	v[nbRow_tidx + iO] = nbRow_tidx + jO;
      }
      Telt eNewPerm(v);
      ListPermGenProv.emplace_back(std::move(eNewPerm));
    }
    Tgroup GRPperm(ListPermGenProv, siz);
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "|GRPperm|=" << GRPperm.size() << " siz=" << siz << "\n";
#endif
    MyMatrix<T> TheSpace1work = TheSpace1*eElt;
    MyMatrix<T> TheSpace1workMod = Concatenate(TheSpace1work, ModSpace);
    //    MyMatrix<T> TheSpace1workMod = TheSpace1Mod * eElt;
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "eElt=\n";
    WriteMatrix(std::cerr, eElt);
#endif
    Face eFace1(siz);
    Face eFace2(siz);
    for (int iO=0; iO<Osiz; iO++) {
      MyVector<T> eVect=O[iO];
#ifdef DEBUG_MATRIX_GROUP
      std::cerr << "iO=" << iO << " eVect=";
      WriteVector(std::cerr, eVect);
#endif
      ResultSolutionIntMat<T> eRes1=SolutionIntMat(TheSpace1workMod, eVect);
      if (eRes1.TheRes)
	eFace1[nbRow + iO]=1;
      ResultSolutionIntMat<T> eRes2=SolutionIntMat(TheSpace2Mod, eVect);
      if (eRes2.TheRes)
	eFace2[nbRow + iO]=1;
    }
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "|eFace1|=" << eFace1.size() << " / " << eFace1.count() << "    |eFace2|=" << eFace2.size() << " / " << eFace2.count() << "\n";
#endif
    if (eFace1.count() == 0 && eFace2.count() == 0) {
      std::cerr << "Error in LinearSpace_ModEquivalence. |eFace1| = |eFace2| = 0\n";
      std::cerr << "Clear bug\n";
      throw TerminalException{1};
    }
    std::pair<bool,Telt> eRes=GRPperm.RepresentativeAction_OnSets(eFace1, eFace2);
    if (!eRes.first)
      return {false, {}, {}};
    Tgroup eStab=GRPperm.Stabilizer_OnSets(eFace2);
    std::vector<MyMatrix<T>> ListMatrGen;
    std::vector<Telt> ListPermGen;
    std::vector<Telt> LGen = eStab.GeneratorsOfGroup();
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << " |LGen|=" << LGen.size() << "\n";
#endif
    for (auto & eGen : LGen) {
      std::vector<Tidx> v(nbRow);
      for (int i=0; i<nbRow; i++)
	v[i] = OnPoints(i, eGen);
      Telt ePerm(std::move(v));
      MyMatrix<T> eMatr=FindTransformation(GRPmatr.EXTfaithAct, GRPmatr.EXTfaithAct, ePerm);
      ListPermGen.emplace_back(std::move(ePerm));
      ListMatrGen.emplace_back(std::move(eMatr));
    }
    GRPwork={n, GRPmatr.EXTfaithAct, ListMatrGen, ListPermGen};
    //
    MyMatrix<T> eMat=FindTransformation(GRPmatr.EXTfaithAct, GRPmatr.EXTfaithAct, eRes.second);
#ifdef DEBUG_MATRIX_GROUP
    std::cerr << "eMat=\n";
    WriteMatrix(std::cerr, eMat);
#endif
    eElt = eElt*eMat;
  }
  for (auto & eGen : GRPwork.ListMatrGen) {
    MyVector<T> V;
    bool test=IsEquiv(V, eGen);
    if (!test) {
      std::cerr << "We have a major inconsistency in LinearSpace_ModEquivalence\n";
      throw TerminalException{1};
    }
  }
  return {true, GRPwork, eElt};
}



template<typename T, typename Tgroup>
std::optional<MyMatrix<T>> LinearSpace_Equivalence(FiniteMatrixGroup<T,typename Tgroup::Telt> const& GRPmatr, MyMatrix<T> const& TheSpace1, MyMatrix<T> const& TheSpace2)
{
  static_assert(is_ring_field<T>::value, "Requires T to be a field in LinearSpace_Equivalence");
#ifdef DEBUG_MATRIX_GROUP
  std::cerr << "Beginning of LinearSpace_Equivalence\n";
#endif
  using Telt=typename Tgroup::Telt;
  int n=TheSpace1.rows();
  T LFact1=LinearSpace_GetDivisor(TheSpace1);
  T LFact2=LinearSpace_GetDivisor(TheSpace2);
#ifdef DEBUG_MATRIX_GROUP
  std::cerr << "LFact1 = " << LFact1 << "\n";
  std::cerr << "LFact2 = " << LFact2 << "\n";
#endif
  if (LFact1 != LFact2)
    return {};
  std::vector<T> eList=FactorsInt(LFact1);
#ifdef DEBUG_MATRIX_GROUP
  std::cerr << "eList =";
  for (auto & eVal : eList)
    std::cerr << " " << eVal;
  std::cerr << "\n";
#endif
  auto IsEquivalence=[&](MyMatrix<T> const& eEquiv) -> bool {
    for (int i=0; i<n; i++) {
      MyVector<T> eVect=GetMatrixRow(TheSpace1, i);
      MyVector<T> eVectG=ProductVectorMatrix(eVect, eEquiv);
      ResultSolutionIntMat<T> eRes=SolutionIntMat(TheSpace2, eVectG);
      if (!eRes.TheRes)
	return false;
    }
    return true;
  };
  FiniteMatrixGroup<T,Telt> GRPwork=GRPmatr;
  int siz=eList.size();
  FiniteMatrixGroup<T,Telt> GRPret=GRPmatr;
  MyMatrix<T> eElt=IdentityMat<T>(n);
  for (int i=1; i<=siz; i++) {
    if (IsEquivalence(eElt))
      return eElt;
    T TheMod=1;
    for (int j=0; j<i; j++)
      TheMod *= eList[j];
    MyMatrix<T> TheSpace1Img=TheSpace1*eElt;
    ResultTestModEquivalence<T,Telt> eRes=LinearSpace_ModEquivalence<T,Tgroup>(GRPwork, TheSpace1Img, TheSpace2, TheMod);
    if (!eRes.TheRes)
      return {};
    eElt = eElt*eRes.eEquiv;
    GRPwork = eRes.GRPwork;
  }
  if (!IsEquivalence(eElt)) {
    std::cerr << "Error in LinearSpace_Equivalence\n";
    throw TerminalException{1};
  }
  return eElt;
}


template<typename T,typename Tgroup>
FiniteMatrixGroup<T,typename Tgroup::Telt> LinPolytopeIntegral_Automorphism_Subspaces(FiniteMatrixGroup<T,typename Tgroup::Telt> const& GRP)
{
  static_assert(is_ring_field<T>::value, "Requires T to be a field in LinPolytopeIntegral_Automorphism_Subspaces");
  using Telt=typename Tgroup::Telt;
  int dim=GRP.EXTfaithAct.cols();
  MyMatrix<T> eBasis=GetZbasis(GRP.EXTfaithAct);
  MyMatrix<T> EXTbas=GRP.EXTfaithAct*Inverse(eBasis);
  std::vector<MyMatrix<T>> ListMatrGen;
  for (auto & eGen : GRP.ListPermGen) {
    MyMatrix<T> TheMat=FindTransformation(EXTbas, EXTbas, eGen);
    ListMatrGen.push_back(TheMat);
  }
  FiniteMatrixGroup<T,Telt> GRPmatr{dim, EXTbas, ListMatrGen, GRP.ListPermGen};
  MyMatrix<T> LattToStab=RemoveFractionMatrix(Inverse(eBasis));
  FiniteMatrixGroup<T,Telt> eStab=LinearSpace_Stabilizer<T,Tgroup>(GRPmatr, LattToStab);
  std::vector<MyMatrix<T>> ListMatrGensB;
  for (auto & eGen : eStab.ListPermGen) {
    MyMatrix<T> TheMat=FindTransformation(GRP.EXTfaithAct, GRP.EXTfaithAct, eGen);
    if (!IsIntegralMatrix(TheMat)) {
      std::cerr << "Clear error in the code\n";
      throw TerminalException{1};
    }
    ListMatrGensB.push_back(TheMat);
  }
  return {dim, GRP.EXTfaithAct, ListMatrGensB, eStab.ListPermGen};
}


template<typename T, typename Tgroup>
std::optional<MyMatrix<T>> LinPolytopeIntegral_Isomorphism_Method4(MyMatrix<T> const& EXT1_T, MyMatrix<T> const& EXT2_T, Tgroup const& GRP1, typename Tgroup::Telt const& ePerm, std::function<bool(MyMatrix<T>)> const& IsMatrixCorrect)
{
  using Telt=typename Tgroup::Telt;
  for (auto & fPerm : GRP1) {
    Telt eEquivCand=fPerm*ePerm;
    MyMatrix<T> eBigMat=FindTransformation(EXT1_T, EXT2_T, eEquivCand);
    if (IsMatrixCorrect(eBigMat))
      return eBigMat;
  }
  return {};
}

template<typename T, typename Tgroup>
Tgroup LinPolytopeIntegral_Stabilizer_Method4(MyMatrix<T> const& EXT_T, Tgroup const& GRPisom, std::function<bool(MyMatrix<T>)> const& IsMatrixCorrect)
{
  static_assert(is_ring_field<T>::value, "Requires T to be a field in LinPolytopeIntegral_Stabilizer_Method4");
  using Telt=typename Tgroup::Telt;
  int nbVert=EXT_T.rows();
  std::vector<Telt> generatorList;
  Tgroup GRPret(nbVert);
  auto fInsert=[&](Telt const& ePerm) -> void {
    bool test=GRPret.isin(ePerm);
    if (!test) {
      generatorList.push_back(ePerm);
      GRPret = Tgroup(generatorList, nbVert);
    }
  };
  for (auto & ePerm : GRPisom) {
    MyMatrix<T> eBigMat=FindTransformation(EXT_T, EXT_T, ePerm);
    if (IsMatrixCorrect(eBigMat))
      fInsert(ePerm);
  }
  return GRPret;
}

template<typename T, typename Tgroup>
Tgroup LinPolytopeIntegral_Stabilizer_Method8(MyMatrix<T> const& EXT_T, Tgroup const& GRPisom)
{
  static_assert(is_ring_field<T>::value, "Requires T to be a field in LinPolytopeIntegral_Stabilizer_Method8");
  using Telt=typename Tgroup::Telt;
  int nbVert=EXT_T.rows();
  int dim=EXT_T.cols();
  std::vector<Telt> ListPermGen;
  std::vector<MyMatrix<T>> ListMatrGen;
  std::vector<Telt> LGen = GRPisom.GeneratorsOfGroup();
  for (auto & eGen : LGen) {
    MyMatrix<T> eMat=FindTransformation(EXT_T, EXT_T, eGen);
    ListMatrGen.push_back(eMat);
    ListPermGen.push_back(eGen);
  }
  FiniteMatrixGroup<T,Telt> GRPfin{dim, EXT_T, ListMatrGen, ListPermGen};
  FiniteMatrixGroup<T,Telt> GRPfinal=LinPolytopeIntegral_Automorphism_Subspaces<T,Tgroup>(GRPfin);
  return Tgroup(GRPfinal.ListPermGen, nbVert);
}




template<typename T,typename Tgroup>
std::optional<MyMatrix<T>> LinPolytopeIntegral_Isomorphism_Subspaces(MyMatrix<T> const& EXT1_T, MyMatrix<T> const& EXT2_T, FiniteMatrixGroup<T,typename Tgroup::Telt> const& GRP2, typename Tgroup::Telt const& eEquiv)
{
  static_assert(is_ring_field<T>::value, "Requires T to be a field in LinPolytopeIntegral_Isomorphism_Subspaces");
#ifdef DEBUG_MATRIX_GROUP
  std::cerr << "Beginning of LinPolytopeIntegral_Isomorphism_Subspaces\n";
#endif
  using Telt=typename Tgroup::Telt;
  int dim=EXT1_T.cols();
  MyMatrix<T> eBasis1=GetZbasis(EXT1_T);
  MyMatrix<T> eBasis2=GetZbasis(EXT2_T);
  MyMatrix<T> EXTbas1=EXT1_T*Inverse(eBasis1);
  MyMatrix<T> EXTbas2=EXT2_T*Inverse(eBasis2);

  MyMatrix<T> TheMatEquiv=FindTransformation(EXTbas1, EXTbas2, eEquiv);
  std::vector<MyMatrix<T>> ListMatrGen;
  for (auto & eGen : GRP2.ListPermGen) {
    MyMatrix<T> TheMat=FindTransformation(EXTbas2, EXTbas2, eGen);
    ListMatrGen.push_back(TheMat);
  }
  FiniteMatrixGroup<T,Telt> GRPspace{dim, EXTbas2, ListMatrGen, GRP2.ListPermGen};
  MyMatrix<T> eLatt1=Inverse(eBasis1)*TheMatEquiv;
  MyMatrix<T> eLatt2=Inverse(eBasis2);
  FractionMatrix<T> eRec1=RemoveFractionMatrixPlusCoeff(eLatt1);
  FractionMatrix<T> eRec2=RemoveFractionMatrixPlusCoeff(eLatt2);
  if (eRec1.TheMult != eRec2.TheMult)
    return {};
  std::optional<MyMatrix<T>> eSpaceEquiv=LinearSpace_Equivalence<T,Tgroup>(GRPspace, eRec1.TheMat, eRec2.TheMat);
  if (!eSpaceEquiv)
    return {};
  MyMatrix<T> eMatFinal=Inverse(eBasis1) * TheMatEquiv * (*eSpaceEquiv) * eBasis2;
  if (!IsIntegralMatrix(eMatFinal)) {
    std::cerr << "eMatFinal should be integral\n";
    throw TerminalException{1};
  }
  return eMatFinal;
}


template<typename T, typename Tgroup>
std::optional<MyMatrix<T>> LinPolytopeIntegral_Isomorphism_Method8(MyMatrix<T> const& EXT1_T, MyMatrix<T> const& EXT2_T, Tgroup const& GRP1, typename Tgroup::Telt const& ePerm)
{
  using Telt = typename Tgroup::Telt;
  std::vector<Telt> ListPermGens;
  std::vector<MyMatrix<T>> ListMatrGens;
  int n=EXT2_T.cols();
  std::vector<Telt> LGen = GRP1.GeneratorsOfGroup();
  for (auto & eGen : LGen) {
    Telt ePermGen=(~ePerm) * eGen * ePerm;
    ListPermGens.push_back(ePermGen);
    MyMatrix<T> eMatr=FindTransformation(EXT2_T, EXT2_T, ePermGen);
    ListMatrGens.push_back(eMatr);
  }
  FiniteMatrixGroup<T,Telt> GRP2{n, EXT2_T, ListMatrGens, ListPermGens};
  return LinPolytopeIntegral_Isomorphism_Subspaces<T,Tgroup>(EXT1_T, EXT2_T, GRP2, ePerm);
}






#endif
