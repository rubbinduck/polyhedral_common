#ifndef INCLUDE_EDGEWALK_H
#define INCLUDE_EDGEWALK_H


#include "two_dim_lorentzian.h"
#include "coxeter_dynkin.h"
#include "vinberg_code.h"



template<typename T>
MyMatrix<T> ComputeLattice_LN(MyMatrix<T> const& G, T const& N)
{
  int n = G.rows();
  MyMatrix<T> M1 = IdentityMat<T>(n);
  MyMatrix<T> M2 = (N / 2) * Inverse(G);
  return IntersectionLattice(M1, M2);
}


template<typename T, typename Tint>
struct FundDomainVertex {
  MyVector<T> gen;
  std::vector<MyVector<Tint>> l_roots;
};



template<typename T, typename Tint>
struct RootCandidate {
  int sign; // 0 for 0, 1 for positive, -1 for negative
  T quant1; // this is (k.alpha_{N,\Delta'})^2 / R_{N,\Delta'}
  T quant2; // this is (k.alpha_{N,\Delta'})^2 / N
  T e_norm;
  MyVector<Tint> alpha;
};

template<typename T>
int get_sign_sing(T const& val)
{
  if (val > 0)
    return 1;
  if (val < 0)
    return -1;
  return 0;
}


template<typename T>
int get_sign_pair_t(T const& p1, T const& p2)
{
  if (p1 < p2)
    return 1;
  if (p1 > p2)
    return -1;
  return 0;
}



// return 0 is p1 == p2 :  1 if p1 < p2 : -1 if p1 > p2
template<typename T>
int get_sign_pair_stdpair(std::pair<int,T> const& p1, std::pair<int,T> const& p2)
{
  if (p1.first == p2.first)
    return get_sign_pair_t(p1.second, p2.second);
  return get_sign_pair_t(p1.first, p2.first);
}


template<typename T, typename Tint>
RootCandidate<T,Tint> gen_possible_extension(MyMatrix<T> const& G, MyVector<T> const& k, MyVector<Tint> const& alpha, T const& res_norm, T const& e_norm)
{
  MyVector<T> alpha_T = UniversalVectorConversion<T,Tint>(alpha);
  T scal = - k.dot(G * alpha_T);
  T quant1 = (scal * scal) / res_norm;
  T quant2 = (scal * scal) / e_norm;
  return {get_sign_sing(scal), quant1, quant2, e_norm, alpha};
}


// sign as before + means preferable according to page 27.
// return 1 if poss1 is preferable to poss2
template<typename T, typename Tint>
int get_sign_cand(RootCandidate<T,Tint> const& poss1, RootCandidate<T,Tint> const& poss2)
{
  int sign1 = get_sign_pair_stdpair({poss1.sign, poss1.quant1}, {poss2.sign, poss2.quant1});
  if (sign1 != 0)
    return sign1; // because -k.alpha1 / sqrt(R1)    <     -k.alpha2 / sqrt(R2)   correspond to 1 in the above.
  int sign2 = get_sign_pair_stdpair({poss1.sign, poss1.quant2}, {poss2.sign, poss2.quant2});
  if (sign2 != 0)
    return sign2; // because -k.alpha1 / sqrt(N1)    <     -k.alpha2 / sqrt(N2)   correspond to 1 in the above.
  int sign3 = get_sign_pair_t(poss1.e_norm, poss2.e_norm);
  return sign3; // because N1 < N2 corresponds to 1
}



template<typename T, typename Tint>
RootCandidate<T,Tint> get_best_candidate(std::vector<RootCandidate<T,Tint>> const& l_cand)
{
  if (l_cand.size() == 0) {
    std::cerr << "We have zero candidates. Abort\n";
    throw TerminalException{1};
  }
  RootCandidate<T,Tint> best_cand = l_cand[0];
  for (auto & eCand : l_cand) {
    if (get_sign_cand(eCand, best_cand) == 1) {
      best_cand = eCand;
    }
  }
  return best_cand;
}






/*
  We take the notations as in EDGEWALK paper.
  ---The dimension is n+1
  ---We have an edge e between two rays k and k'.
  We have u1, .... u(n-1) roots that are orthogonal and U the real span of those vectors.
  ---P is the real plane orthogonal to U.
  ---pi_U and pi_P are the corresponding projectors
  ---How is (1/2) P defined and correspond to k (typo correction)
  ---


 */
template<typename T, typename Tint>
FundDomainVertex<T,Tint> EdgewalkProcedure(MyMatrix<T> const& G, MyVector<T> const& k, std::vector<MyVector<Tint>> l_ui, std::vector<T> const& l_norms, MyVector<Tint> const& v_disc)
{
  int dim = G.size();
  size_t n_root = l_ui.size();
  MyMatrix<T> Space(n_root,dim);
  MyMatrix<T> EquaB(n_root+1,dim);
  for (size_t i_root=0; i_root<n_root; i_root++) {
    MyVector<T> eV = UniversalVectorConversion<T,Tint>(l_ui[i_root]);
    MyVector<T> eP = G * eV;
    T eScal = k.dot(eP);
    if (eScal != 0) {
      std::cerr << "The scalar product should be 0\n";
      throw TerminalException{1};
    }
    for (int i=0; i<dim; i++) {
      Space(i_root,i) = eV(i);
      EquaB(i_root,i) = eP(i);
    }
  }
  MyVector<T> eP = G * k;
  for (int i=0; i<dim; i++)
    EquaB(n_root,i) = eP(i);
  MyMatrix<T> NSP = NullspaceMat(EquaB);
  if (NSP.rows() != 1) {
    std::cerr << "The dimension should be exactly 2\n";
    throw TerminalException{1};
  }
  MyVector<T> r0 = GetMatrixRow(NSP,0);
  std::vector<RootCandidate<T,Tint>> l_candidates;
  bool allow_euclidean = false;
  std::vector<Possible_Extension<T>> l_extension = ComputePossibleExtensions(G, l_ui, l_norm, allow_euclidean);
  for (auto & e_extension : l_extension) {
    T e_norm = e_extension.e_norm;
    MyMatrix<T> Latt = ComputeLattice_LN(G, e_norm);
    // Now getting into the LN space
    MyMatrix<T> Space_LN = Space * Inverse(Latt);
    MyMatrix<T> G_LN = Latt * G * Latt.transpose();
    MyMatrix<T> Equas = Space_LN * G_LN;
    MyMatrix<T> NSP = NullspaceIntMat(TransposedMat(Equas));
    MyMatrix<T> GP_LN = NSP * G_LN * NSP.transpose();
    MyVector<T> r0_LN = Inverse(Latt).transpose() * r0;
    MyVector<T> r0_NSP = SolutionMat(NSP, r0_LN);
    MyVector<Tint> r0_work = UniversalVectorConversion<Tint,T>(RemoveVectorFraction(r0_NSP));
    std::optional<MyVector<Tint>> opt_v = get_first_next_vector(GP_LN, r0_work, e_extension.res_norm);
    if (opt_v) {
      MyVector<T> v = UniversalVectorConversion<T,Tint>(*opt_v);
      MyVector<T> alpha_T = e_extension.u + v * NSP;
      MyVector<Tint> alpha = UniversalVectorConversion<Tint,T>(root_T);
      RootCandidate<T,Tint> eCand = gen_possible_extension(G, k, alpha, e_extension.res_norm, e_norm);
      l_candidates.push_back(eCand);
    }
  }
  if (l_candidates.size() > 0) {
    RootCandidate<T,Tint> best_cand = get_best_candidate(l_candidates);
    std::vector<MyVector<Tint>> l_roots = l_ui;
    l_roots.push_back(best_cand.alpha);
    MyMatrix<T> Mat_root = UniversalMatrixConversion<T,Tint>(MatrixFromVectorFamily(l_roots));
    MyMatrix<T> EquaMat = Mat_root * G;
    MyMatrix<T> NSP = NullspaceTrMat(EquaMat);
    if (NSP.rows() != 1) {
      std::cerr << "We should have exactly one row\n";
      throw TerminalException{1};
    }
    MyVector<T> gen = GetMatrixRow(NSP, 0);
    T scal = gen.dot(G * k);
    if (scal > 0) {
      return {gen, l_roots};
    }
    if (scal < 0) {
      return {-gen, l_roots};
    }
    std::cerr << "Failed to find a matching entry\n";
    throw TerminalException{1};
  }
  // So, no candidates were found. We need to find isotropic vectors.
  MyMatrix<T> NSPbas(n,2);
  SetMatrixRow(NSPbas, k, 0);
  SetMatrixRow(NSPbas, r0, 1);
  MyMatrix<T> Gred = NSPbas * G * NSPbas.transpose();
  std::optional<MyMatrix<T>> Factor_opt = GetIsotropicFactorization(Gred);
  if (!opt) {
    std::cerr << "The matrix is not isotropic. Major rethink are needed\n";
    throw TerminalException{1};
  }
  MyMatrix<T> Factor = *Factor_opt;
  // We want a vector inside of the cone (there are two: C and -C)
  auto get_can_gen=[&](MyVector<T> const& v) -> MyVector<T> {
    T scal = k.dot(G * v);
    if (scal > 0)
      return v;
    if (scal < 0)
      return -v;
    std::cerr << "We should have scal != 0 to be able to conclude\n";
    throw TerminalException{1};
  };
  std::vector<MyVector<T>> l_gens;
  for (size_t i=0; i<2; i++) {
    // a x + by correspond to the ray (u0, u1) = (-b, a)
    T u0 = -Factor(i,1);
    T u1 =  Factor(i,0);
    MyVector<T> gen = u0 * k + u1 * r0;
    MyVector<T> can_gen = get_can_gen(gen);
    MyVector<T> v_disc_t = UniversalVectorConversion<T,Tint>(v_disc);
    T scal = v_disc_t.dot(G * can_gen);
    if (scal > 0)
      l_gens.push_back(can_gen);
  }
  if (l_gens.size() != 1) {
    std::cerr << "We should have just one vector in order to conclude. Rethink needed\n";
    throw TerminalException{1};
  }
  std::vector<MyVector<Tint>> l_roots = l_ui;
  MyVector<Tint> w(n);
  /// FILL OUT THE CODE
  l_roots.push_back(w);
  return {l_gens[0], l_roots};
}







template<typename T, typename Tint>
struct PairVertices {
  FundDomainVertex<T,Tint> vert1;
  FundDomainVertex<T,Tint> vert2;
  MyMatrix<Tint> BasisRoot;
};

template<typename T, typename Tint>
PairVertices<T,Tint> gen_pair_vertices(FundDomainVertex<T,Tint> const& vert1, FundDomainVertex<T,Tint> const& vert2)
{
  std::unordered_set<MyVector<Tint>> set_v;
  for (auto & eV : vert1.l_roots)
    set_v.insert(eV);
  for (auto & eV : vert2.l_roots)
    set_v.insert(eV);
  std::vector<MyVector<Tint>> l_roots;
  for (auto & eV : set_v)
    l_roots.push_back(eV);
  MyMatrix<Tint> MatV = MatrixFromVectorFamily(l_roots);
  return {vert1, vert2, MatV};
}



template<typename T, typename Tint>
struct ResultEdgewalk {
  std::vector<MyMatrix<Tint>> l_gen_isom_cox;
  std::vector<PairVertices<T,Tint>> l_orbit_pair_vertices;
};





template<typename T, typename Tint>
ResultEdgeWalk<T,Tint> LORENTZ_RunEdgewalkAlgorithm(MyMatrix<T> const& G, std::vector<T> const& l_norms, FundDomainVertex<T,Tint> const& eVert)
{
  std::vector<MyMatrix<Tint>> l_gen_isom_cox;
  using Tdone = std::pair<bool,bool>;
  using Tentry = std::pair<Tdone, PairVertices<T,Tint>>;
  std::vector<Tentry> l_entry;
  auto f_insert_gen=[&](MyMatrix<Tint> const& eP) -> void {
    MyMatrix<T> eP_T = UniversalMatrixConversion<T,Tint>(eP);
    MyMatrix<T> G2 = eP_T * G * eP_T.transpose();
    if (G2 != G) {
      std::cerr << "The matrix eP should leave the quadratic form invariant\n";
      throw TerminalException{1};
    }
    l_gen_isom_cox.push_back(eP);
  };
  auto func_insert_pair_vertices=[&](Tentry const& v_pair) -> void {
    for (auto & u_pair : l_entry) {
      std::optional<MyMatrix<Tint>> equiv_opt = f_equiv(u_pair.second, v_pair.second);
      if (eauiv_opt) {
        f_insert_gen(*eauiv_opt);
        return;
      }
    }
    l_entry.push_back(v_pair);
    for (auto & eGen : f_stab(v_pair)) {
      f_insert_gen(eGen);
    }
  };
  size_t len = eVert.l_roots.size();
  auto insert_edges_from_vertex=[&](FundDomainVertex<T,Tint> const& theVert) -> void {
    for (size_t i=0; i<len; i++) {
      std::vector<MyVector<Tint>> l_ui;
      for (size_t j=0; j<len; j++) {
        if (i != j) {
          l_ui.push_back(theVert.l_roots[j]);
        }
      }
      MyVector<Tint> v_disc = theVert.l_roots[i];
      FundDomainVertex<T,Tint> fVert = EdgewalkProcedure(G, theVert.gen, l_ui, l_norms, v_disc);
      PairVertices<T,Tint> epair = gen_pair_vertices(theVert, fVert);
      Tentry entry{{true,false},epair};
      func_insert_pair_vertices(entry);
    }
  };
  while(true) {
    bool IsFinished = true;
    for (auto & entry : l_entry) {
      Tdone & eDone = entry.first;
      if (eDone.first) {
        eDone.first = false;
        insert_edges_from_vertex(entry.second.vert1);
        IsFinished = false;
      }
      if (eDone.second) {
        eDone.second = false;
        insert_edges_from_vertex(entry.second.vert2);
        IsFinished = false;
      }
    }
    if (IsFinished)
      break;
  }
  std::vector<PairVertices<T,Tint>> l_orbit_pair_vertices;
  for (auto & epair : l_entry)
    l_orbit_pair_vertices.push_back(epair.second);
  return {l_gen_isom_cox, l_orbit_pair_vertices};
}









#endif
