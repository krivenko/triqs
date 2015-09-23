/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2013, P. Seth, I. Krivenko, M. Ferrero and O. Parcollet
 *
 * TRIQS is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * TRIQS is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * TRIQS. If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/
#pragma once
#include "./fundamental_operator_set.hpp"
#include "../operators/many_body_operator.hpp"
#include "./hilbert_space.hpp"

#include <vector>
#include <utility>

// Workaround for GCC bug 41933
#include <triqs/utility/compiler_details.hpp>
#if defined GCC_VERSION && GCC_VERSION < 40900
#define GCC_BUG_41933_WORKAROUND
#endif

#ifdef GCC_BUG_41933_WORKAROUND
#include <triqs/utility/tuple_tools.hpp>
#endif

namespace triqs {
namespace hilbert_space {

// DOC TO BE WRITTEN
// fock state convention:
// C^dag_0  ...  C^dag_k   | 0 >
// operator convention:
// C^dag_0 .. C^dag_i ... C^j  .. C^0

/*
   This class is the imperative version of the many_body_operator
   It is a class template on the Hilbert space type and has an optimization
   option UseMap which allows the user to give a map describing the connection
   between Hilbert space when acting on a state. This map is a map of *pointers*
   to the Hilbert spaces, to avoid unnessacry copies.

   If UseMap is false, the constructor takes two arguments:

   imperative_operator(many_body_op, fundamental_ops)

   a trivial identity connection map is then used.

   If UseMap is true, the constructor takes three arguments:

   imperative_operator(many_body_op, fundamental_ops, hilbert_map)
   */

template <typename HilbertType, typename ScalarType = double, bool UseMap = false> class imperative_operator {

 using scalar_t = ScalarType;

 struct one_term_t {
  scalar_t coeff;
  uint64_t d_mask, dag_mask, d_count_mask, dag_count_mask;
 };
 std::vector<one_term_t> all_terms;

 std::vector<sub_hilbert_space> const *sub_spaces;
 using hilbert_map_t = std::vector<int>;
 hilbert_map_t hilbert_map;

 public:
 imperative_operator() {}

 // constructor from a many_body_operator, a fundamental_operator_set and a map (UseMap = true)
 imperative_operator(triqs::utility::many_body_operator<scalar_t> const &op, fundamental_operator_set const &fops,
                     hilbert_map_t hmap = hilbert_map_t(), std::vector<sub_hilbert_space> const *sub_spaces_set = nullptr) {

  sub_spaces = sub_spaces_set;
  hilbert_map = hmap;
  if ((hilbert_map.size() == 0) != !UseMap) TRIQS_RUNTIME_ERROR << "Internal error";

  // The goal here is to have a transcription of the many_body_operator in terms
  // of simple vectors (maybe the code below could be more elegant)
  for (auto const &term : op) {
   std::vector<int> dag, ndag;
   uint64_t d_mask = 0, dag_mask = 0;
   for (auto const &canonical_op : term.monomial) {
    (canonical_op.dagger ? dag : ndag).push_back(fops[canonical_op.indices]);
    (canonical_op.dagger ? dag_mask : d_mask) |= (uint64_t(1) << fops[canonical_op.indices]);
   }
   auto compute_count_mask = [](std::vector<int> const &d) {
    uint64_t mask = 0;
    bool is_on = (d.size() % 2 == 1);
    for (int i = 0; i < 64; ++i) {
     if (std::find(begin(d), end(d), i) != end(d))
      is_on = !is_on;
     else if (is_on)
      mask |= (uint64_t(1) << i);
    }
    return mask;
   };
   uint64_t d_count_mask = compute_count_mask(ndag), dag_count_mask = compute_count_mask(dag);
   all_terms.push_back(one_term_t{term.coef, d_mask, dag_mask, d_count_mask, dag_count_mask});
  }
 }

 template<typename Lambda>
 void update_coeffs(Lambda L) {
  for(auto & M : all_terms) L(M.coeff);
 }

 private:
 template <typename StateType> StateType get_target_st(StateType const &st, std::true_type use_map) const {
  auto n = hilbert_map[st.get_hilbert().get_index()];
  if (n == -1) return StateType{};
  return StateType{(*sub_spaces)[n]};
 }

 template <typename StateType> StateType get_target_st(StateType const &st, std::false_type use_map) const {
  return StateType(st.get_hilbert());
 }

 static bool parity_number_of_bits(uint64_t v) {
  // http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetNaive
  // v ^= v >> 16;
  // only ok until 16 orbitals ! assert this or put the >> 16
  v ^= v >> 8;
  v ^= v >> 4;
  v ^= v >> 2;
  v ^= v >> 1;
  return v & 0x01;
 }

  // Forward the call to the coefficient
#ifdef GCC_BUG_41933_WORKAROUND
 template<typename... Args>
 static auto apply_if_possible(scalar_t const& x, std::tuple<Args...> const& args_tuple) -> typename std::result_of<scalar_t(Args...)>::type {
  return triqs::tuple::apply(x,args_tuple);
 }
 static auto apply_if_possible(scalar_t const& x, std::tuple<> const&) -> scalar_t {
  return x;
 }
#else
 template<typename... Args>
 static auto apply_if_possible(scalar_t const& x, Args&&... args) -> typename std::result_of<scalar_t(Args...)>::type {
  return x(std::forward<Args>(args)...);
 }
 static auto apply_if_possible(scalar_t const& x) -> scalar_t {
  return x;
 }
#endif

 public:
 // act on a state and return a new state
 template <typename StateType, typename... Args>
 StateType operator()(StateType const &st, Args&&... args) const {

  StateType target_st = get_target_st(st, std::integral_constant<bool, UseMap>());
  auto const& hs = st.get_hilbert();

#ifdef GCC_BUG_41933_WORKAROUND
  auto args_tuple = std::make_tuple(args...);
#endif

  for (int i = 0; i < all_terms.size(); ++i) { // loop over monomials
   auto M = all_terms[i];
#ifdef GCC_BUG_41933_WORKAROUND
   foreach(st, [M, &target_st,hs,args_tuple](int i, typename StateType::value_type amplitude) {
#else
   foreach(st, [M, &target_st,hs,args...](int i, typename StateType::value_type amplitude) {
#endif
    fock_state_t f2 = hs.get_fock_state(i);
    if ((f2 & M.d_mask) != M.d_mask) return;
    f2 &= ~M.d_mask;
    if (((f2 ^ M.dag_mask) & M.dag_mask) != M.dag_mask) return;
    fock_state_t f3 = ~(~f2 & ~M.dag_mask);
    auto sign_is_minus = parity_number_of_bits((f2 & M.d_count_mask) ^ (f3 & M.dag_count_mask));
    // update state vector in target Hilbert space
    auto ind = target_st.get_hilbert().get_state_index(f3);
#ifdef GCC_BUG_41933_WORKAROUND
    target_st(ind) += amplitude * apply_if_possible(M.coeff,args_tuple) * (sign_is_minus ? -1.0 : 1.0);
#else
    target_st(ind) += amplitude * apply_if_possible(M.coeff,args...) * (sign_is_minus ? -1.0 : 1.0);
#endif
   }); // foreach
  }
  return target_st;
 }
};
}}
