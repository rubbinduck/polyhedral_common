// Copyright (C) 2022 Mathieu Dutour Sikiric <mathieu.dutour@gmail.com>
#ifndef SRC_INDEFINITE_LEGENDRE_EQUATION_H_
#define SRC_INDEFINITE_LEGENDRE_EQUATION_H_

// clang-format off
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <utility>
#include <map>
#include <set>
#include "factorizations.h"
#include "Positivity.h"
// clang-format on

#ifdef DEBUG
#define DEBUG_LEGENDRE
#endif

/*
  The ternary equation x M x = 0 for L a 3x3 matrix
  is named the Legendre equation.
  The paper we use is LEGENDRE’S THEOREM, LEGRANGE’S DESCENT
  https://public.csusm.edu/aitken_html/notes/legendre.pdf
  Refered to as P1 below.
  -
  Here we just put the test of existence.
 */

template<typename T>
std::map<T, size_t> product_entry(std::map<T, size_t> const& x, std::map<T, size_t> const& y) {
  std::map<T, size_t> ret = x;
  for (auto & kv: y) {
    ret[kv.first] += kv.second;
  }
  return ret;
}


template<typename T>
T evaluate_entry(std::map<T, size_t> const& x) {
  T prod(1);
  for (auto & kv: x) {
    for (size_t u=0; u<kv.second; u++) {
      prod *= kv.first;
    }
  }
  return prod;
}

template<typename T>
struct LegendreReductionInformation {
  MyMatrix<T> TransMat;
  MyVector<T> aReduced;
  std::vector<std::map<T,size_t>> l_entry;
};


/*
  We apply Corollary 4 of P1 to case of abc is square-free.
  The conditions are:
  * a,b,c are not all of the same sign
  * −bc is a square modulo |a|
  * −ac is a square modulo |b|
  * −ab is a square modulo |c|.
 */
template <typename T> bool determine_solvability_dim3(LegendreReductionInformation<T> const& lri, [[maybe_unused]] std::ostream& os) {
  MyVector<T> const& aReduced = lri.aReduced;
  size_t n_plus = 0;
  size_t n_minus = 0;
  for (int i = 0; i < 3; i++) {
    if (aReduced(i) > 0) {
      n_plus++;
    }
    if (aReduced(i) < 0) {
      n_minus++;
    }
  }
#ifdef DEBUG_LEGENDRE
  os << "LEG: n_plus=" << n_plus << " n_minus=" << n_minus << "\n";
#endif
  if (n_plus == 0 || n_minus == 0) {
#ifdef DEBUG_LEGENDRE
    os << "LEG: Returning false by positivity condition\n";
#endif
    return false;
  }
  T a = aReduced(0);
  T b = aReduced(1);
  T c = aReduced(2);
#ifdef DEBUG_LEGENDRE
  T prod = a * b * c;
  os << "LEG: prod=" << prod << "\n";
#endif
  T a_abs = T_abs(a);
  T b_abs = T_abs(b);
  T c_abs = T_abs(c);
  std::map<T, size_t> a_abs_map = lri.l_entry[0];
  std::map<T, size_t> b_abs_map = lri.l_entry[1];
  std::map<T, size_t> c_abs_map = lri.l_entry[2];
  T a_cnt = -b * c;
  T b_cnt = -a * c;
  T c_cnt = -a * b;
#ifdef DEBUG_LEGENDRE
  os << "LEG: a_abs=" << a_abs << " a_cnt=" << a_cnt << "\n";
  os << "LEG: b_abs=" << b_abs << " b_cnt=" << b_cnt << "\n";
  os << "LEG: c_abs=" << c_abs << " c_cnt=" << c_cnt << "\n";
#endif
  //
  // Test early termination by using Jacobi.
  // This is preferable because Jacobi is super fast to compute.
  //
  bool test_jacobi_a = compute_jacobi_symbol(a_cnt, a_abs);
  bool test_jacobi_b = compute_jacobi_symbol(b_cnt, b_abs);
  bool test_jacobi_c = compute_jacobi_symbol(c_cnt, c_abs);
#ifdef DEBUG_LEGENDRE
  os << "LEG: test_jacobi, a=" << test_jacobi_a << " b=" << test_jacobi_b << " c=" << test_jacobi_c << "\n";
#endif
  if (!test_jacobi_a || !test_jacobi_b || !test_jacobi_c) {
#ifdef DEBUG_LEGENDRE
    os << "LEG: early termination from Jacobi criterion\n";
#endif
    return false;
  }
  //
  bool test_a = is_quadratic_residue_map(a_cnt, a_abs_map);
#ifdef DEBUG_LEGENDRE
  os << "QUADTEST: [" << a_cnt << "," << a_abs << "," << test_a << "],\n";
#endif
  if (!test_a) {
#ifdef DEBUG_LEGENDRE
    os << "LEG: Returning false by quadratic residue for a\n";
#endif
    return false;
  }
  //
  //
  bool test_b = is_quadratic_residue_map(b_cnt, b_abs_map);
#ifdef DEBUG_LEGENDRE
  os << "QUADTEST: [" << b_cnt << "," << b_abs << "," << test_b << "],\n";
#endif
  if (!test_b) {
#ifdef DEBUG_LEGENDRE
    os << "LEG: Returning false by quadratic residue for b\n";
#endif
    return false;
  }
  //
  //
  bool test_c = is_quadratic_residue_map(c_cnt, c_abs_map);
#ifdef DEBUG_LEGENDRE
  os << "QUADTEST: [" << c_cnt << "," << c_abs << "," << test_c << "],\n";
#endif
  if (!test_c) {
#ifdef DEBUG_LEGENDRE
    os << "LEG: Returning false by quadratic residue for c\n";
#endif
    return false;
  }
#ifdef DEBUG_LEGENDRE
  os << "LEG: Returning true as the Legendre condition for solvability are satisfied\n";
#endif
  return true;
}

/*
  We apply some prime reduction so that no prime factor is shared between the
  coefficients. We return something so that:
  * The vector aRet expressing the matrix
  * The diagonal matrix B such that
  B Diag(aRet) B = u Diag(aV)  for some coefficient u.
 */
template <typename T>
LegendreReductionInformation<T> reduction_information(MyVector<T> const &aV, [[maybe_unused]] std::ostream& os) {
#ifdef DEBUG_LEGENDRE
  if (aV.size() != 3) {
    std::cerr << "The length should be exactly 3\n";
    throw TerminalException{1};
  }
#endif
  T a = aV(0);
  T b = aV(1);
  T c = aV(2);
  T a_abs = T_abs(a);
  T b_abs = T_abs(b);
  T c_abs = T_abs(c);
  std::vector<T> a_help{b_abs, c_abs};
  std::vector<T> b_help{a_abs, c_abs};
  std::vector<T> c_help{a_abs, b_abs};
  std::map<T, size_t> a_map = FactorsIntMap_help(a_abs, a_help);
  std::map<T, size_t> b_map = FactorsIntMap_help(b_abs, b_help);
  std::map<T, size_t> c_map = FactorsIntMap_help(c_abs, c_help);
  MyMatrix<T> TransMat = IdentityMat<T>(3);
  //
  // Eliminating the even prime powers.
  //
  auto get_set = [&](std::map<T, size_t> const &map,
                     int const &idx) -> std::set<T> {
    std::set<T> eset;
    for (auto &kv : map) {
      T const &val = kv.first;
      size_t mult = kv.second;
      size_t r = mult % 2;
      size_t q = mult / 2;
      if (r > 0)
        eset.insert(val);
      TransMat(idx, idx) *= MyPow(val, q);
    }
    return eset;
  };
  std::set<T> a_set = get_set(a_map, 0);
  std::set<T> b_set = get_set(b_map, 1);
  std::set<T> c_set = get_set(c_map, 2);
  //
  // Now reducing so that no primes is shared
  //
  std::set<T> primes;
  for (auto &kv : a_map) {
    primes.insert(kv.first);
  }
  for (auto &kv : b_map) {
    primes.insert(kv.first);
  }
  for (auto &kv : c_map) {
    primes.insert(kv.first);
  }
  T a_prod(1);
  T b_prod(1);
  T c_prod(1);
  T ab_prod(1);
  T ac_prod(1);
  T bc_prod(1);
  std::map<T, size_t> a_prod_map, b_prod_map, c_prod_map, ab_prod_map, ac_prod_map, bc_prod_map;
  for (auto &p : primes) {
    bool a_in = a_set.count(p) == 1;
    bool b_in = b_set.count(p) == 1;
    bool c_in = c_set.count(p) == 1;
    if (a_in && !b_in && !c_in) {
      a_prod *= p;
      a_prod_map[p] += 1;
    }
    if (!a_in && b_in && c_in) {
      bc_prod *= p;
      bc_prod_map[p] += 1;
    }
    if (!a_in && b_in && !c_in) {
      b_prod *= p;
      b_prod_map[p] += 1;
    }
    if (a_in && !b_in && c_in) {
      ac_prod *= p;
      ac_prod_map[p] += 1;
    }
    if (!a_in && !b_in && c_in) {
      c_prod *= p;
      c_prod_map[p] += 1;
    }
    if (a_in && b_in && !c_in) {
      ab_prod *= p;
      ab_prod_map[p] += 1;
    }
  }
  TransMat(0, 0) *= bc_prod;
  TransMat(1, 1) *= ac_prod;
  TransMat(2, 2) *= ab_prod;
  MyVector<T> aRet(3);
#ifdef DEBUG_LEGENDRE
  os << "LEG: a_prod=" << a_prod << " bc_prod=" << bc_prod << "\n";
  os << "LEG: b_prod=" << b_prod << " ac_prod=" << ac_prod << "\n";
  os << "LEG: c_prod=" << c_prod << " ab_prod=" << ab_prod << "\n";
  os << "LEG: primes=";
  for (auto & p : primes) {
    os << " " << p;
  }
  os << "\n";
#endif
  aRet(0) = T_sign(a) * a_prod * bc_prod;
  aRet(1) = T_sign(b) * b_prod * ac_prod;
  aRet(2) = T_sign(c) * c_prod * ab_prod;
  std::map<T, size_t> a_bc_prod_map = product_entry(a_prod_map, bc_prod_map);
  std::map<T, size_t> b_ac_prod_map = product_entry(b_prod_map, ac_prod_map);
  std::map<T, size_t> c_ab_prod_map = product_entry(c_prod_map, ab_prod_map);
  std::vector<std::map<T, size_t>> l_entry{a_bc_prod_map, b_ac_prod_map, c_ab_prod_map};
#ifdef DEBUG_LEGENDRE
  std::map<T, size_t> a_map2 = FactorsIntMap(T_abs(aRet(0)));
  std::map<T, size_t> b_map2 = FactorsIntMap(T_abs(aRet(1)));
  std::map<T, size_t> c_map2 = FactorsIntMap(T_abs(aRet(2)));
  for (auto & kv : a_map2) {
    T const& p = kv.first;
    if (b_map2.count(p) == 1) {
      std::cerr << "p should not be in the b_map2\n";
    }
    if (c_map2.count(p) == 1) {
      std::cerr << "p should not be in the c_map2\n";
    }
  }
  for (auto & kv : b_map2) {
    T const& p = kv.first;
    if (a_map2.count(p) == 1) {
      std::cerr << "p should not be in the a_map2\n";
    }
    if (c_map2.count(p) == 1) {
      std::cerr << "p should not be in the c_map2\n";
    }
  }
  for (auto & kv : c_map2) {
    T const& p = kv.first;
    if (a_map2.count(p) == 1) {
      std::cerr << "p should not be in the a_map2\n";
    }
    if (b_map2.count(p) == 1) {
      std::cerr << "p should not be in the b_map2\n";
    }
  }
#endif
  return {TransMat, aRet, l_entry};
}


template<typename T>
std::pair<MyMatrix<T>, MyVector<T>> get_reduced_diagonal(MyMatrix<T> const &M, [[maybe_unused]] std::ostream& os) {
  DiagSymMat<T> dsm = DiagonalizeNonDegenerateSymmetricMatrix(M);
  MyVector<T> V1 = GetDiagonal(dsm.RedMat);
#ifdef DEBUG_LEGENDRE
  os << "V1=" << StringVectorGAP(V1) << "\n";
#endif
  MyVector<T> V2 = RemoveFractionVector(V1);
#ifdef DEBUG_LEGENDRE
  os << "V2=" << StringVectorGAP(V2) << "\n";
  for (int i=0; i<3; i++) {
    if (V2(i) == 0) {
      std::cerr << "LEG: V2(i) should be non-zero\n";
      throw TerminalException{1};
    }
  }
#endif
  return {dsm.Transform, V2};
}



template <typename T> bool ternary_has_isotropic_vector(MyMatrix<T> const &M, std::ostream& os) {
  using Tring = typename underlying_ring<T>::ring_type;
  std::pair<MyMatrix<T>, MyVector<T>> pair = get_reduced_diagonal(M, os);
  MyVector<Tring> red_diag_A = UniversalVectorConversion<Tring, T>(pair.second);
  LegendreReductionInformation<Tring> lri = reduction_information(red_diag_A, os);
  return determine_solvability_dim3(lri, os);
}


template<typename T>
bool is_square_free(T const& a) {
  std::map<T, size_t> map = FactorsIntMap(T_abs(a));
  for (auto &kv : map) {
    if (kv.second >= 2) {
      return false;
    }
  }
  return true;
}

template<typename T>
std::pair<T,T> separate_square_factor(T const& val) {
  std::map<T, size_t> map = FactorsIntMap(T_abs(val));
  T sqr(1);
  T nosqr(1);
  using Tint = uint32_t;
  Tint two(2);
  for (auto & kv : map) {
    T const& p = kv.first;
    Tint mult = kv.second;
    std::pair<Tint, Tint> pair = ResQuoInt(mult, two);
    Tint res = pair.first;
    Tint quot = pair.second;
    for (Tint u=0; u<res; u++) {
      nosqr *= p;
    }
    for (Tint u=0; u<quot; u++) {
      sqr *= p;
    }
  }
  return {sqr, nosqr};
}


/*
  See Definition 1 of Page 4 of P1
 */
template<typename T>
bool satisfy_descent_condition(std::pair<T,T> const& pair) {
  T const& a = pair.first;
  T const& b = pair.second;
  if (!is_square_free(a)) {
#ifdef DEBUG_LEGENDRE
    std::cerr << "LEG: a=" << a << " is not square free\n";
#endif
    return false;
  }
  if (!is_square_free(b)) {
#ifdef DEBUG_LEGENDRE
    std::cerr << "LEG: b=" << b << " is not square free\n";
#endif
    return false;
  }
  if (a < 0 && b < 0) {
#ifdef DEBUG_LEGENDRE
    std::cerr << "LEG: a and b are negative not allowed\n";
#endif
    return false;
  }
  T d = GcdPair(a, b);
  T c = - (a / d) * (b / d);
  if (!is_quadratic_residue(a, T_abs(b))) {
#ifdef DEBUG_LEGENDRE
    std::cerr << "LEG: a is not a quadratic residue for b\n";
#endif
    return false;
  }
  if (!is_quadratic_residue(b, T_abs(a))) {
#ifdef DEBUG_LEGENDRE
    std::cerr << "LEG: b is not a quadratic residue for a\n";
#endif
    return false;
  }
  if (!is_quadratic_residue(c, T_abs(d))) {
#ifdef DEBUG_LEGENDRE
    std::cerr << "LEG: c=" << c << " d=" << d << "\n";
    std::cerr << "LEG: c is not a quadratic residue for d\n";
#endif
    return false;
  }
  return true;
}



/*
  Map the equation into a Lagrange normal equation.
  The equation in standard form is
  Z^2 = a X^2 + b Y^2
  The original equation is
  a0 X0^2 + a1 X1^2 + a2 X2^2 = 0
  Rewritten equation with min_idx give us:
  aI X_I^2 = - a(I+1) X_(I+1)^2 - a(I+2) X(I+2)^2
  X_I^2 = - a(I+1) / aI X_(I+1)^2 - a(I+2) / aI X(I+2)^2
  X(I)   = Z
  X(I+1) = aI X
  X(I+2) = aI Y
  a = - a(I+1) aI
  b = - a(I+2) aI
  Z^2 = a X^2 + b Y^2
  X = X(I+1) / aI
  Y = X(I+2) / aI
  Z = X(I)
    or after rescale
  X =    X(I+1)
  Y =    X(I+2)
  Z = aI X(I)
    
*/
template<typename T>
std::pair<MyMatrix<T>, std::pair<T, T>> get_lagrange_normal(MyVector<T> const& v) {
  size_t min_idx=0;
  T min_val = T_abs(v(0));
  for (size_t u=1; u<3; u++) {
    T val = T_abs(v(u));
    if (val < min_val) {
      min_val = val;
      min_idx = u;
    }
  }
#ifdef DEBUG_LEGENDRE
  std::cerr << "LEG: min_idx=" << min_idx << " min_val=" << min_val << "\n";
#endif
  size_t three(3);
  size_t idxZ = min_idx;
  size_t idxX = min_idx+1;
  if (idxX >= three) {
    idxX -= three;
  }
  size_t idxY = min_idx+2;
  if (idxY >= three) {
    idxY -= three;
  }
#ifdef DEBUG_LEGENDRE
  std::cerr << "LEG: idxX=" << idxX << " idxY=" << idxY << " idxZ=" << idxZ << "\n";
#endif
  T cVal = v(idxZ);
  T a = -v(idxX) * cVal;
  T b = -v(idxY) * cVal;
  MyMatrix<T> M = ZeroMatrix<T>(3,3);
  // Not sure about the matrix below.
  M(idxX,0) = cVal;
  M(idxY,1) = cVal;
  M(idxZ,2) = 1;
#ifdef DEBUG_LEGENDRE
  std::cerr << "LEG: We have M\n";
#endif
  std::pair<T,T> pair{a, b};
  return {M, pair};
}

/*
  Lemma 6 operation from P1.
 */
template<typename T>
std::pair<MyMatrix<T>, std::pair<T,T>> descent_operation(std::pair<T,T> const& pair) {
  T const& a = pair.first;
  T const& b = pair.second;
  std::optional<T> opt = find_quadratic_residue(a, b);
  T u = unfold_opt(opt, "we expect to get u");
  T diff = u*u - a;
  T quot = diff / b;
  std::pair<T,T> pair2 = separate_square_factor(quot);
  T e = pair2.first;
  T bp = pair2.second;
  MyMatrix<T> M = ZeroMatrix<T>(3,3);
  M(0,0) = u;
  M(0,2) = 1;
  M(1,1) = b * e;
  M(2,0) = a;
  M(2,2) = u;
  std::pair<T, T> pair3{a, bp};
  return {M, pair3};
}

template<typename T>
std::pair<MyMatrix<T>, std::pair<T,T>> switch_xy(std::pair<T,T> const& pair) {
  T const& a = pair.first;
  T const& b = pair.second;
  MyMatrix<T> M = ZeroMatrix<T>(3,3);
  M(0,1) = 1;
  M(1,0) = 1;
  M(2,2) = 1;
  std::pair<T, T> pair2{b, a};
  return {M, pair2};
}

/*
  Equation Z^2 = a X^2 + b Y^2
  Value a=0 or b=0 would allow early termination but if so, the
  quadratic form would be degenerate.
 */
template<typename T>
std::optional<MyVector<T>> get_trivial_solution(std::pair<T,T> const& pair) {
  T const& a = pair.first;
  T const& b = pair.second;
  MyVector<T> V = ZeroVector<T>(3);
  if (a == 1) {
    V(0) = 1;
    V(2) = 1;
    return V;
  }
  if (b == 1) {
    V(1) = 1;
    V(2) = 1;
    return V;
  }
  return {};
}


template<typename T>
MyVector<T> execute_lagrange_descent(MyVector<T> const& diag) {
  std::pair<MyMatrix<T>, std::pair<T, T>> pair3 = get_lagrange_normal(diag);
  std::pair<T, T> work_pair = pair3.second;
#ifdef DEBUG_LEGENDRE
  std::cerr << "LEG: We have work_pair\n";
  if (!satisfy_descent_condition(work_pair)) {
    std::cerr << "LEG: a=" << work_pair.first << " b=" << work_pair.second << "\n";
    std::cerr << "LEG: work_pair 1, should satisfy the descent condition\n";
    throw TerminalException{1};
  }
#endif
  std::vector<std::pair<MyMatrix<T>, std::pair<T, T>>> l_pair;
  auto get_sol=[&](MyVector<T> const& V) -> MyVector<T> {
    size_t len = l_pair.size();
#ifdef DEBUG_LEGENDRE
    std::cerr << "LEG: get_sol beginning len=" << len << "\n";
    std::cerr << "LEG: get_sol V=" << StringVector(V) << "\n";
#endif
    MyVector<T> V_work = V;
    for (size_t u=0; u<len; u++) {
      size_t v = len - 1 - u;
      V_work = l_pair[v].first * V_work;
#ifdef DEBUG_LEGENDRE
      T const& a = l_pair[v].second.first;
      T const& b = l_pair[v].second.second;
      T const& x = V_work(0);
      T const& y = V_work(1);
      T const& z = V_work(2);
      T diff = z*z - a*x*x - b*y*y;
      if (diff != 0) {
        std::cerr << "Consistency error in the descent\n";
        throw TerminalException{1};
      }
#endif
    }
#ifdef DEBUG_LEGENDRE
    std::cerr << "LEG: pair3.first=\n";
    WriteMatrix(std::cerr, pair3.first);
#endif
    V_work = pair3.first * V_work;
#ifdef DEBUG_LEGENDRE
    std::cerr << "LEG: V_work=" << StringVector(V_work) << "\n";
    T sum(0);
    for (int i=0; i<3; i++) {
      T const& val = V_work(i);
      sum += diag(i) * val * val;
    }
    if (sum != 0) {
      std::cerr << "LEG: pair3.first=\n";
      WriteMatrix(std::cerr, pair3.first);
      std::cerr << "LEG: diag=" << StringVector(diag) << "\n";
      std::cerr << "LEG: V_work=" << StringVector(V_work) << "\n";
      std::cerr << "Consistency error in the lagrange reduction\n";
      throw TerminalException{1};
    }
#endif
    return V_work;
  };
  while(true) {
    std::optional<MyVector<T>> opt = get_trivial_solution(work_pair);
    if (opt) {
      MyVector<T> V = *opt;
      return get_sol(V);
    }
    T a_abs = T_abs(work_pair.first);
    T b_abs = T_abs(work_pair.second);
    if (a_abs > b_abs) {
      std::pair<MyMatrix<T>, std::pair<T,T>> pair_xy = switch_xy(work_pair);
      work_pair = pair_xy.second;
      l_pair.push_back(pair_xy);
    }
    std::pair<MyMatrix<T>, std::pair<T,T>> pair_desc = descent_operation(work_pair);
    work_pair = pair_desc.second;
#ifdef DEBUG_LEGENDRE
    if (!satisfy_descent_condition(work_pair)) {
      std::cerr << "LEG: work_pair 2, should satisfy the descent condition\n";
      throw TerminalException{1};
    }
#endif
    l_pair.push_back(pair_desc);
  }
}


/*
  We are looking for a ternary isotropic vector if one exists.

  Now, it goes:
  -- diagonalize the matrix
  -- removal of common primes and square factors.
  -- Express the problem as an equation Z^2 = aX^2 + bY^2
 */
template<typename T>
std::optional<MyVector<T>> TernaryIsotropicVector(MyMatrix<T> const& M, std::ostream& os) {
  using Tring = typename underlying_ring<T>::ring_type;
  std::pair<MyMatrix<T>, MyVector<T>> pair1 = get_reduced_diagonal(M, os);
  MyVector<Tring> red_diag_A = UniversalVectorConversion<Tring, T>(pair1.second);
  LegendreReductionInformation<Tring> lri = reduction_information(red_diag_A, os);
  bool test = determine_solvability_dim3(lri, os);
  if (!test) {
    return {};
  }
  MyVector<Tring> sol1 = execute_lagrange_descent(lri.aReduced);
#ifdef DEBUG_LEGENDRE
  std::cerr << "LEG: sol1=" << StringVector(sol1) << "\n";
#endif
  MyVector<Tring> sol2 = lri.TransMat * sol1;
#ifdef DEBUG_LEGENDRE
  std::cerr << "LEG: sol2=" << StringVector(sol1) << "\n";
  Tring sum(0);
  for (int i=0; i<3; i++) {
    sum += red_diag_A(i) * sol2(i) * sol2(i);
  }
  if (sum != 0) {
    std::cerr << "LEG: lri.TransMat=\n";
    WriteMatrix(std::cerr, lri.TransMat);
    std::cerr << "LEG: red_diag_A=" << StringVector(red_diag_A) << "\n";
    std::cerr << "LEG: lri.aReduced=" << StringVector(lri.aReduced) << "\n";
    std::cerr << "LEG: sol2=" << StringVector(sol2) << "\n";
    std::cerr << "LEG: sol2 is not a solution of the equation\n";
    throw TerminalException{1};
  }
#endif
  MyVector<T> sol3 = UniversalVectorConversion<T,Tring>(sol2);
#ifdef DEBUG_LEGENDRE
  std::cerr << "LEG: sol3=" << StringVector(sol3) << "\n";
  std::cerr << "LEG: pair1.second=\n";
  WriteMatrix(std::cerr, pair1.first);
#endif
  MyVector<T> sol4 = pair1.first.transpose() * sol3;
#ifdef DEBUG_LEGENDRE
  std::cerr << "LEG: sol4=" << StringVector(sol4) << "\n";
  T sum2 = EvaluationQuadForm(M, sol4);
  if (sum2 != 0) {
    std::cerr << "LEG: sol4 is not a solution of the equation\n";
    throw TerminalException{1};
  }
#endif
  return sol4;
}


// clang-format off
#endif  // SRC_INDEFINITE_LEGENDRE_EQUATION_H_
// clang-format on
