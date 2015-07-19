/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2012-2013 by O. Parcollet
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
#include "./tools.hpp"
#include "./gf.hpp"
#include "./local/tail.hpp"
#include "./local/no_tail.hpp"
#include "./meshes/matsubara_freq.hpp"
namespace triqs {
namespace gfs {

 // singularity
 template <> struct gf_default_singularity<imfreq, matrix_valued> {
  using type = tail;
 };
 template <> struct gf_default_singularity<imfreq, scalar_valued> {
  using type = tail;
 };

 namespace gfs_implementation {

  /// ---------------------------  hdf5 ---------------------------------
  
  template <typename S> struct h5_name<imfreq, matrix_valued, S> {
   static std::string invoke() { return "ImFreq"; }
  };

  /// ---------------------------  data access  ---------------------------------

  template <> struct data_proxy<imfreq, matrix_valued> : data_proxy_array<std::complex<double>, 3> {};
  template <> struct data_proxy<imfreq, scalar_valued> : data_proxy_array<std::complex<double>, 1> {};

  /// ---------------------------  evaluator ---------------------------------

  // handle the case where the matsu. freq is out of grid...

  struct _eval_imfreq_base_impl {
   static constexpr int arity = 1;
   template <typename G> int sh(G const * g) const { return (g->mesh().domain().statistic == Fermion ? 1 : 0);}

   // int -> replace by matsubara_freq
   template <typename G>
   AUTO_DECL operator()(G const *g, int n) const
       RETURN((*g)(matsubara_freq(n, g->mesh().domain().beta, g->mesh().domain().statistic)));

   template <typename G> typename G::singularity_t operator()(G const *g, tail_view t) const {
    return compose(g->singularity(),t);
    //return g->singularity();
   }
  };
  // --- various 4 specializations

  // scalar_valued, tail
  template <> struct evaluator<imfreq, scalar_valued, tail> : _eval_imfreq_base_impl {
 
   template <typename G> evaluator(G *) {};
   using _eval_imfreq_base_impl::operator();

   template <typename G> std::complex<double> operator()(G const *g, matsubara_freq const &f) const {
    if (g->mesh().positive_only()) { // only positive Matsubara frequencies
     if ((f.n >= 0) && (f.n < g->mesh().size())) return (*g)[f.n];
     if ((f.n < 0) && ((-f.n - this->sh(g)) < g->mesh().size())) return conj((*g)[-f.n - this->sh(g)]);
    } else {
     if ((f.n >= g->mesh().first_index()) && (f.n < g->mesh().size() + g->mesh().first_index())) return (*g)[f.n];
    }
    return evaluate(g->singularity(),f)(0, 0);
   }
  };

  // scalar_valued, no tail
  template <> struct evaluator<imfreq, scalar_valued, nothing> : _eval_imfreq_base_impl {

   template <typename G> evaluator(G *) {};
   using _eval_imfreq_base_impl::operator();

   template <typename G> std::complex<double> operator()(G const *g, matsubara_freq const &f) const {
    if (g->mesh().positive_only()) { // only positive Matsubara frequencies
     if ((f.n >= 0) && (f.n < g->mesh().size())) return (*g)[f.n];
     if ((f.n < 0) && ((-f.n - this->sh(g)) < g->mesh().size())) return conj((*g)[-f.n - this->sh(g)]);
    } else {
     if ((f.n >= g->mesh().first_index()) && (f.n < g->mesh().size() + g->mesh().first_index())) return (*g)[f.n];
    }
    TRIQS_RUNTIME_ERROR<< "evaluation out of mesh";
    return 0;
   }
  };

  // matrix_valued, tail
  template <> struct evaluator<imfreq, matrix_valued, tail> : _eval_imfreq_base_impl {

   template <typename G> evaluator(G *) {};
   using _eval_imfreq_base_impl::operator();

   template <typename G> arrays::matrix_const_view<std::complex<double>> operator()(G const *g, matsubara_freq const &f) const {
    if (g->mesh().positive_only()) { // only positive Matsubara frequencies
     if ((f.n >= 0) && (f.n < g->mesh().size())) return (*g)[f.n]();
     if ((f.n < 0) && ((-f.n - this->sh(g)) < g->mesh().size()))
      return arrays::matrix<std::complex<double>>{conj((*g)[-f.n - this->sh(g)]())};
    } else {
     if ((f.n >= g->mesh().first_index()) && (f.n < g->mesh().size() + g->mesh().first_index())) return (*g)[f.n];
    }
    return evaluate(g->singularity(), f);
   }
  };

  // matrix_valued, no tail
  template <> struct evaluator<imfreq, matrix_valued, nothing> : _eval_imfreq_base_impl {

   template <typename G> evaluator(G *) {};
   using _eval_imfreq_base_impl::operator();

   template <typename G> arrays::matrix_const_view<std::complex<double>> operator()(G const *g, matsubara_freq const &f) const {
    if (g->mesh().positive_only()) { // only positive Matsubara frequencies
     if ((f.n >= 0) && (f.n < g->mesh().size())) return (*g)[f.n]();
     if ((f.n < 0) && ((-f.n - this->sh(g)) < g->mesh().size()))
      return arrays::matrix<std::complex<double>>{conj((*g)[-f.n - this->sh(g)]())};
    } else {
     if ((f.n >= g->mesh().first_index()) && (f.n < g->mesh().size() + g->mesh().first_index())) return (*g)[f.n];
    }
    TRIQS_RUNTIME_ERROR<< "evaluation out of mesh";
    auto r = arrays::matrix<std::complex<double>>{get_target_shape(*g)};
    r() = 0;
    return r;
   }
  };

 } // gfs_implementation

}
}
