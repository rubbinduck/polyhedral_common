#ifndef SHVEC_DOUBLE_INCLUDE
#define SHVEC_DOUBLE_INCLUDE

#include "LatticeDefinitions.h"

#include <shvec.h>
#include <la_support.h>


template<typename T, typename Tint>
MyMatrix<Tint> T_ShortVector(MyMatrix<T> const& eMat, T const&MaxNorm)
{
  int check, mode, number;
  double bound, **gram_matrix;
  shvec_request request;
  shvec_info info;
  int dim=eMat.rows();
  double FudgeFact=1.1;
  //  std::cerr << "T_ShortVector, step 1\n";
  if (dim == 1) {
    MyMatrix<Tint> TheSHV(2,1);
    TheSHV(0,0)=1;
    TheSHV(1,0)=1;
    return TheSHV;
  }
  number = 0;
  check = 0;
  double MaxNorm_d;
  GET_DOUBLE(MaxNorm, MaxNorm_d);
  bound=MaxNorm_d*FudgeFact;
  mode = SHVEC_MODE_BOUND;
  //  std::cerr << "T_ShortVector, step 2\n";
  doubleMakeSquareMatrix(dim, &gram_matrix);
  for (int i = 0; i < dim; i++)
    for (int j = 0; j < dim; j++) {
      T eVal=eMat(i,j);
      double eVal_d;
      GET_DOUBLE(eVal, eVal_d);
      gram_matrix[j][i] = eVal_d;
    }
  request = (shvec_request) malloc(sizeof(struct shvec_request_struct));
  if (request == NULL) {
    std::cerr << "Let us die now, it was nice so far...\n";
    throw TerminalException{1};
  }
  //  std::cerr << "T_ShortVector, step 3\n";
  request->ints.ComputeTheta=0;
  initShvecReq(dim, gram_matrix, NULL, check, request);
  
  request->bound = bound;
  request->mode = mode;
  request->number = number;
  //  std::cerr << "T_ShortVector, step 4\n";
  info = (shvec_info) malloc(sizeof(struct shvec_info_struct));
  if (info == NULL) {
    std::cerr << "Let us die now, it was nice so far...\n";
    throw TerminalException{1};
  }
  //  std::cerr << "T_ShortVector, step 5\n";
  initShvecInfo(info);
  //  std::cerr << "T_ShortVector, step 6\n";
  computeShvec(request, info);
  //  std::cerr << "T_ShortVector, step 7\n";

  /* output results */
  int PreNbVect=info->short_vectors_number;
  std::vector<int> Status(PreNbVect);
  int nbShort=0;
  for (int i = 0; i <PreNbVect; i++) {
    MyVector<int> eVect(dim);
    for (int j=0; j<dim; j++)
      eVect(j)=info->short_vectors[i][j];
    T eNorm=EvaluationQuadForm<T,int>(eMat, eVect);
    int ePos=-1;
    if (eNorm <= MaxNorm) {
      ePos=0;
      nbShort++;
    }
    Status[i]=ePos;
  }
  //  std::cerr << "T_ShortVector, step 8\n";
  MyMatrix<Tint> TheSHV(2*nbShort, dim);
  int idx=0;
  for (int iShort=0; iShort<PreNbVect; iShort++) {
    if (Status[iShort] == 0) {
      for (int j=0; j<dim; j++) {
	int eVal=info->short_vectors[iShort][j];
	TheSHV(2*idx  , j)=eVal;
	TheSHV(2*idx+1, j)=-eVal;
      }
      idx++;
    }
  }
  //  std::cerr << "T_ShortVector, step 9\n";
  destroyShvecInfo(info);
  //  std::cerr << "T_ShortVector, step 10\n";
  destroyShvecReq(request);
  //  std::cerr << "T_ShortVector, step 11\n";
  doubleDestroySquareMatrix(dim, &gram_matrix);
  //  std::cerr << "T_ShortVector, step 12\n";
  free(request);
  //  std::cerr << "T_ShortVector, step 13\n";
  free(info);
  //  std::cerr << "T_ShortVector, step 14\n";
  return TheSHV;
}


template<typename T, typename Tint>
resultCVP<T,Tint> CVPVallentinProgram_double(MyMatrix<T> const& GramMat, MyVector<T> const& eV)
{
  int check, mode, number;
  double **gram_matrix;
  shvec_request request;
  shvec_info info;
  int dim=GramMat.rows();
  if (dim == 1) {
    std::cerr << "Need to program the case of dimension 1\n";
    throw TerminalException{1};
  }
  number = 0;
  check = 0;
  double bound=0;
  //  mode = SHVEC_MODE_BOUND;
  mode = SHVEC_MODE_SHORTEST_VECTORS;
  // mode = SHVEC_MODE_THETA_SERIES;
  doubleMakeSquareMatrix(dim, &gram_matrix);
  for (int i = 0; i < dim; i++)
    for (int j = 0; j < dim; j++) {
      T eVal=GramMat(i,j);
      double eVal_d;
      GET_DOUBLE(eVal, eVal_d);
      gram_matrix[j][i] = eVal_d;
    }
  request = (shvec_request) malloc(sizeof(struct shvec_request_struct));
  if (request == NULL) {
    std::cerr << "Let us die now, it was nice so far...\n";
    throw TerminalException{1};
  }
  initShvecReq(dim, gram_matrix, NULL, check, request);
  
  request->bound = bound;
  request->mode = mode;
  request->number = number;
  info = (shvec_info) malloc(sizeof(struct shvec_info_struct));
  if (info == NULL) {
    std::cerr << "Let us die now, it was nice so far...\n";
    throw TerminalException{1};
  }
  for (int i=0; i<dim; i++) {
    T eVal = -eV[i];
    double eVal_d;
    GET_DOUBLE(eVal, eVal_d);
    request->coset[i]=eVal_d;
  }
  
  initShvecInfo(info);
  computeShvec(request, info);

  /* output results */
  int PreNbVect=info->short_vectors_number;
  std::vector<int> Status(PreNbVect);
  T MinNorm;
  int nbShort=0;
  for (int i = 0; i <PreNbVect; i++) {
    MyVector<T> eVect(dim);
    for (int j=0; j<dim; j++) {
      T eDiff=info->short_vectors[i][j] - eV(j);
      eVect(j)=eDiff;
    }
    T eNorm=EvaluationQuadForm<T,T>(GramMat, eVect);
    if (i == 0) {
      MinNorm=eNorm;
      Status[i]=1;
      nbShort=1;
    }
    else {
      if (MinNorm == eNorm) {
	Status[i]=1;
	nbShort++;
      }
      else {
	if (eNorm < MinNorm) {
	  for (int j=0; j<i; j++)
	    Status[j]=0;
	  Status[i]=1;
	  nbShort=1;
	  MinNorm=eNorm;
	}
      }
    }
  }
  MyMatrix<Tint> ListVect(nbShort, dim);
  int idx=0;
  for (int iShort=0; iShort<PreNbVect; iShort++)
    if (Status[iShort] == 1) {
      for (int j=0; j<dim; j++) {
	int eVal=info->short_vectors[iShort][j];
	ListVect(idx, j)=eVal;
      }
      idx++;
    }
  destroyShvecInfo(info);
  destroyShvecReq(request);
  doubleDestroySquareMatrix(dim, &gram_matrix);
  free(request);
  free(info);
  return {MinNorm, ListVect};
}

template<typename T, typename Tint>
Tshortest<T,Tint> T_ShortestVector(MyMatrix<T> const& eMat)
{
  T MinNorm=MinimumDiagonal(eMat);
  MyMatrix<Tint> TheSHVall=T_ShortVector<T,Tint>(eMat, MinNorm);
  return SelectShortestVector(eMat, TheSHVall);
}


template<typename T, typename Tint>
MyMatrix<Tint> ExtractInvariantVectorFamily(MyMatrix<T> const& eMat, std::function<bool(MyMatrix<Tint> const&)> const& fCorrect)
{
  T MaxNorm=MaximumDiagonal(eMat);
  MyMatrix<Tint> SHVall=T_ShortVector<T,Tint>(eMat, MaxNorm);
  /*  std::cerr << "MaxNorm = " << MaxNorm << " eMat =\n";
  WriteMatrix(std::cerr, eMat);
  std::cerr << "|SHVall|=" << SHVall.rows() << "\n";
  WriteMatrix(std::cerr, SHVall);*/
  std::set<T> SetNorm;
  int nbSHV=SHVall.rows();
  std::vector<T> ListNorm(nbSHV);
  for (int iSHV=0; iSHV<nbSHV; iSHV++) {
    MyVector<Tint> eRow=GetMatrixRow(SHVall, iSHV);
    T eNorm=EvaluationQuadForm(eMat, eRow);
    ListNorm[iSHV]=eNorm;
    SetNorm.insert(eNorm);
  }
  std::vector<MyVector<Tint>> ListVect;
  for (auto const& eNorm : SetNorm) {
    for (int iSHV=0; iSHV<nbSHV; iSHV++)
      if (ListNorm[iSHV] == eNorm) {
	MyVector<Tint> eRow=GetMatrixRow(SHVall, iSHV);
	ListVect.push_back(eRow);
      }
    MyMatrix<Tint> SHVret=MatrixFromVectorFamily(ListVect);
    if (fCorrect(SHVret))
      return SHVret;
  }
  std::cerr << "We should never reach that stage\n";
  throw TerminalException{1};
}

template<typename T, typename Tint>
MyMatrix<Tint> ExtractInvariantVectorFamilyFullRank(MyMatrix<T> const& eMat)
{
  int n=eMat.rows();
  std::function<bool(MyMatrix<Tint> const&)> fCorrect=[&](MyMatrix<Tint> const& M) -> bool {
    if (RankMat(M) == n)
      return true;
    return false;
  };
  return ExtractInvariantVectorFamily(eMat, fCorrect);
}

template<typename T, typename Tint>
MyMatrix<Tint> ExtractInvariantVectorFamilyZbasis(MyMatrix<T> const& eMat)
{
  int n=eMat.rows();
  std::function<bool(MyMatrix<Tint> const&)> fCorrect=[&](MyMatrix<Tint> const& M) -> bool {
    if (RankMat(M) < n)
      return false;
    Tint indx=Int_IndexLattice(M);
    if (T_Norm(indx) == 1)
      return true;
    return false;
  };
  return ExtractInvariantVectorFamily(eMat, fCorrect);
}







#endif
