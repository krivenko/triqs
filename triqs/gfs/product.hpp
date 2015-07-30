/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2013 by O. Parcollet
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
#include "./gf.hpp"
#include "./meshes/product.hpp"
#include "./singularity/tail_zero.hpp"

namespace triqs {
namespace gfs {

 // special case : we need to pop for all the variables
 template <typename... Ms, typename Target, typename Singularity, typename Evaluator, bool IsView, bool IsConst>
 auto get_target_shape(gf_impl<cartesian_product<Ms...>, Target, Singularity, Evaluator, IsView, IsConst> const &g) {
  return g.data().shape().template front_mpop<sizeof...(Ms)>();
 }

 // The default singularity, for each Variable.
 template <typename... Ms> struct gf_default_singularity<cartesian_product<Ms...>, scalar_valued> {
  using type = tail_zero<dcomplex>;
 };
 template <typename... Ms> struct gf_default_singularity<cartesian_product<Ms...>, matrix_valued> {
  using type = tail_zero<matrix<dcomplex>>;
 };
 template <typename... Ms, int R> struct gf_default_singularity<cartesian_product<Ms...>, tensor_valued<R>> {
  using type = tail_zero<array<dcomplex,R>>;
 };

 // forward declaration, Cf m_tail
 template <typename Variable, typename... Args> auto evaluate(gf_const_view<Variable, tail> const &g, Args const &... args);
 template <typename Variable, typename... Args> auto evaluate(gf<Variable, tail> const &g, Args const &... args);
 template <typename Variable, typename... Args> auto evaluate(gf_view<Variable, tail> const &g, Args const &... args);

 namespace gfs_implementation {

  /// ---------------------------  data access  ---------------------------------

  template <typename... Ms>
  struct data_proxy<cartesian_product<Ms...>, scalar_valued> : data_proxy_array_multivar<std::complex<double>, sizeof...(Ms)> {};

  template <typename... Ms>
  struct data_proxy<cartesian_product<Ms...>, matrix_valued> : data_proxy_array_multivar_matrix_valued<std::complex<double>,
                                                                                                       2 + sizeof...(Ms)> {};

  template <int R, typename... Ms>
  struct data_proxy<cartesian_product<Ms...>, tensor_valued<R>> : data_proxy_array_multivar<std::complex<double>,
                                                                                            R + sizeof...(Ms)> {};

  // special case ? Or make a specific container....
  template <typename M0>
  struct data_proxy<cartesian_product<M0, imtime>, matrix_valued> : data_proxy_array_multivar_matrix_valued<double, 2 + 2> {};

  /// ---------------------------  hdf5 ---------------------------------

  // h5 name : name1_x_name2_.....
  template <typename S, typename... Ms> struct h5_name<cartesian_product<Ms...>, matrix_valued, S> {
   static std::string invoke() {
    return triqs::tuple::fold([](std::string a, std::string b) { return a + std::string(b.empty() ? "" : "_x_") + b; },
                              reverse(std::make_tuple(h5_name<Ms, matrix_valued, nothing>::invoke()...)), std::string());
   }
  };
  template <typename S, int R, typename... Ms>
  struct h5_name<cartesian_product<Ms...>, tensor_valued<R>, S> : h5_name<cartesian_product<Ms...>, matrix_valued, S> {
  };

  /// ---------------------------  evaluator ---------------------------------

  using triqs::make_const_view;
  inline dcomplex make_const_view(dcomplex z) { return z; }
 
  // now the multi d evaluator itself.
  template <typename Target, typename Sing, typename... Ms> struct evaluator<cartesian_product<Ms...>, Target, Sing> {

   static constexpr int arity = sizeof...(Ms); // METTRE ARITY DANS LA MESH ! 
   template <typename G> evaluator(G *) {};

   template <typename G, typename... Args> auto operator()(G const &g, Args &&... args) const {
    static_assert(sizeof...(Args) == arity, "Wrong number of arguments in gf evaluation");
    if (g.mesh().is_within_boundary(args...))
     return make_const_view(g.mesh().evaluate(typename G::mesh_t::default_interpol_policy{}, g, std::forward<Args>(args)...));
    using rt = std14::decay_t<decltype(
        make_const_view(g.mesh().evaluate(typename G::mesh_t::default_interpol_policy{}, g, std::forward<Args>(args)...)))>;
    return rt{evaluate(g.singularity(), args...)};
   }
  };

  // special case when the tail is nothing
  template <typename Target, typename... Ms> struct evaluator<cartesian_product<Ms...>, Target, nothing> {

   static constexpr int arity = sizeof...(Ms); // METTRE ARITY DANS LA MESH !
   template <typename G> evaluator(G *) {};

   template <typename G, typename... Args> auto operator()(G const &g, Args &&... args) const {
    static_assert(sizeof...(Args) == arity, "Wrong number of arguments in gf evaluation");
    if (!g.mesh().is_within_boundary(args...)) TRIQS_RUNTIME_ERROR << "Evaluation out of the mesh";
    return g.mesh().evaluate(typename G::mesh_t::default_interpol_policy{}, g, std::forward<Args>(args)...);
   }
  };

 } // gf_implementation
}
}

