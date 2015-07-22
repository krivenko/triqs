/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2013-2014 by O. Parcollet
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
#include "../tools.hpp"
#include "../gf.hpp"
#include "./tail.hpp"

namespace triqs {
namespace gfs {

 //struct no_tail {};
 using no_tail = nothing;

 template <typename Variable, typename Target, typename S, typename Evaluator, bool V, bool C>
 gf_view<Variable, Target, no_tail, Evaluator, C> make_gf_view_without_tail(gf_impl<Variable, Target, S, Evaluator, V, C> const &g) {
  return {g.mesh(), g.data(), {}, g.symmetry(), g.indices(), g.name};
 }

 namespace details { // dispatch the test for scalar_valued and matrix_valued
  using arrays::mini_vector;

  inline void _equal_or_throw(mini_vector<size_t, 2> const &s_t, mini_vector<size_t, 2> const &g_t) {
   if (s_t != g_t) TRIQS_RUNTIME_ERROR << "make_gf_from_g_and_tail: Shape of the gf target and of the tail mismatch";
  }

  inline void _equal_or_throw(mini_vector<size_t, 2> const &s_t, mini_vector<size_t, 0> const &g_t) {
   if (s_t != mini_vector<size_t, 2>{1, 1})
    TRIQS_RUNTIME_ERROR << "make_gf_from_g_and_tail: tail shape must be 1x1 for a scalar gf";
  }
 }

 template <typename Variable, typename Target, typename S, typename Evaluator, bool V, bool C>
 gf_view<Variable, Target> make_gf_from_g_and_tail(gf_impl<Variable, Target, S, Evaluator, V, C> const &g, tail t) {
  details::_equal_or_throw(t.shape(), get_target_shape(g));
  auto g2 = gf<Variable, Target, no_tail>{g}; // copy the function without tail
  return {std::move(g2.mesh()), std::move(g2.data()), std::move(t), g2.symmetry()};
 }

 template <typename Variable, typename Target, typename Evaluator, bool V, bool C>
 gf_view<Variable, Target, tail, Evaluator, C> make_gf_view_from_g_and_tail(gf_impl<Variable, Target, no_tail, Evaluator, V, C> const &g,
                                                                 tail_view t) {
  details::_equal_or_throw(t.shape(), get_target_shape(g));
  return {g.mesh(), g.data(), t, g.symmetry()};
 }
}
}
