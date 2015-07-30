/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2011 by M. Ferrero, O. Parcollet
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
#include "fourier_matsubara.hpp"
#include <fftw3.h>

namespace triqs {
namespace gfs {

 template <typename GfElementType> GfElementType convert_green(dcomplex const& x) { return x; }
 template <> double convert_green<double>(dcomplex const& x) { return real(x); }

 //--------------------------------------------------------------------------------------

 struct impl_worker {

  arrays::vector<dcomplex> g_in, g_out;

  dcomplex oneFermion(dcomplex a, double b, double tau, double beta) {
   return -a * (b >= 0 ? exp(-b * tau) / (1 + exp(-beta * b)) : exp(b * (beta - tau)) / (1 + exp(beta * b)));
  }

  dcomplex oneBoson(dcomplex a, double b, double tau, double beta) {
   return a * (b >= 0 ? exp(-b * tau) / (exp(-beta * b) - 1) : exp(b * (beta - tau)) / (1 - exp(b * beta)));
  }

  //-------------------------------------

  void direct(gf_view<imfreq, scalar_valued> gw, gf_const_view<imtime, scalar_valued> gt) {
   auto ta = gt.singularity();
   direct_impl(make_gf_view_without_tail(gw), make_gf_view_without_tail(gt), ta);
   gw.singularity() = gt.singularity(); // set tail
  }

  void direct(gf_view<imfreq, scalar_valued, no_tail> gw, gf_const_view<imtime, scalar_valued, no_tail> gt) {
   auto ta = tail{1,1};
   direct_impl(gw, gt, ta);
  }
  
  //-------------------------------------

  private:
  void direct_impl(gf_view<imfreq, scalar_valued, no_tail> gw, gf_const_view<imtime, scalar_valued, no_tail> gt,
                   tail const& ta) {
   // TO BE MODIFIED AFTER SCALAR IMPLEMENTATION TODO
   dcomplex d = ta(1)(0, 0), A = ta.get_or_zero(2)(0, 0), B = ta.get_or_zero(3)(0, 0);
   double b1 = 0, b2 = 0, b3 = 0;
   dcomplex a1, a2, a3;
   double beta = gt.mesh().domain().beta;
   auto L = gt.mesh().size() - 1;
   if (L < 2*gw.mesh().size()) TRIQS_RUNTIME_ERROR << "The time mesh mush be at least twice as long as the freq mesh";
   double fact = beta / L;
   dcomplex iomega = dcomplex(0.0, 1.0) * std::acos(-1) / beta;
   g_in.resize(L);
   g_out.resize(gw.mesh().size());
   if (gw.domain().statistic == Fermion) {
    b1 = 0;
    b2 = 1;
    b3 = -1;
    a1 = d - B;
    a2 = (A + B) / 2;
    a3 = (B - A) / 2;
   } else {
    b1 = -0.5;
    b2 = -1;
    b3 = 1;
    a1 = 4 * (d - B) / 3;
    a2 = B - (d + A) / 2;
    a3 = d / 6 + A / 2 + B / 3;
   }
   if (gw.domain().statistic == Fermion) {
    for (auto& t : gt.mesh())
     if(t.index() < L) {
       g_in[t.index()] = fact * exp(iomega * t) *
                         (gt[t] - (oneFermion(a1, b1, t, beta) + oneFermion(a2, b2, t, beta) + oneFermion(a3, b3, t, beta)));
     }
   } else {
    for (auto& t : gt.mesh())
     if(t.index() < L) {
       g_in[t.index()] = fact * (gt[t] - (oneBoson(a1, b1, t, beta) + oneBoson(a2, b2, t, beta) + oneBoson(a3, b3, t, beta)));
     }
   }
   details::fourier_base(g_in, g_out, L, true);

   // We manually remove half of the first time point contribution and add half
   // of the last time point contribution. This is necessary to make sure that
   // no symmetry is lost
   if (gw.domain().statistic == Fermion) {
     for (auto& w : gw.mesh()) {
       g_out(w.index()) -= 0.5*fact*(   gt[0] - (oneFermion(a1, b1, 0, beta) + oneFermion(a2, b2, 0, beta) + oneFermion(a3, b3, 0, beta))
                                      + gt[L] - (oneFermion(a1, b1, beta, beta) + oneFermion(a2, b2, beta, beta) + oneFermion(a3, b3, beta, beta)) );
     }
   } else {
     for (auto& w : gw.mesh()) {
       g_out(w.index()) += 0.5*fact*(   gt[L] - (oneBoson(a1, b1, beta, beta) + oneBoson(a2, b2, beta, beta) + oneBoson(a3, b3, beta, beta))
                                      - gt[0] + (oneBoson(a1, b1, 0, beta) + oneBoson(a2, b2, 0, beta) + oneBoson(a3, b3, 0, beta)) );
     }
   }

   for (auto& w : gw.mesh()) {
    gw[w] = g_out(w.index()) + a1 / (w - b1) + a2 / (w - b2) + a3 / (w - b3);
   }



  }

  public:
  //-------------------------------------

  void inverse(gf_view<imtime, scalar_valued> gt, gf_const_view<imfreq, scalar_valued> gw) {
   static bool Green_Function_Are_Complex_in_time = false;
   // If the Green function are NOT complex, then one use the symmetry property
   // fold the sum and get a factor 2
   auto ta = gw.singularity();
   // TO BE MODIFIED AFTER SCALAR IMPLEMENTATION TODO
   dcomplex d = ta(1)(0, 0), A = ta.get_or_zero(2)(0, 0), B = ta.get_or_zero(3)(0, 0);
   double b1, b2, b3;
   dcomplex a1, a2, a3;

   double beta = gw.domain().beta;
   size_t L = gt.mesh().size() - 1;
   if (L < 2*gw.mesh().size()) TRIQS_RUNTIME_ERROR << "The time mesh mush be at least twice as long as the freq mesh";
   dcomplex iomega = dcomplex(0.0, 1.0) * std::acos(-1) / beta;
   double fact = (Green_Function_Are_Complex_in_time ? 1 : 2) / beta;
   g_in.resize(gw.mesh().size());
   g_out.resize(L);

   if (gw.domain().statistic == Fermion) {
    b1 = 0;
    b2 = 1;
    b3 = -1;
    a1 = d - B;
    a2 = (A + B) / 2;
    a3 = (B - A) / 2;
   } else {
    b1 = -0.5;
    b2 = -1;
    b3 = 1;
    a1 = 4 * (d - B) / 3;
    a2 = B - (d + A) / 2;
    a3 = d / 6 + A / 2 + B / 3;
   }
   g_in() = 0;
   for (auto& w : gw.mesh()) {
    g_in[w.index()] = fact * (gw[w] - (a1 / (w - b1) + a2 / (w - b2) + a3 / (w - b3)));
   }
   // for bosons GF(w=0) is divided by 2 to avoid counting it twice
   if (gw.domain().statistic == Boson && !Green_Function_Are_Complex_in_time) g_in(0) *= 0.5;

   details::fourier_base(g_in, g_out, L, false);

   // CORRECT FOR COMPLEX G(tau) !!!
   typedef double gt_result_type;
   // typedef typename gf<imtime>::mesh_type::gf_result_type gt_result_type;
   if (gw.domain().statistic == Fermion) {
    for (auto& t : gt.mesh()) {
     if (t.index() < L) {
       gt[t] =
         convert_green<gt_result_type>(g_out(t.index()) * exp(-iomega * t) + oneFermion(a1, b1, t, beta) +
                                       oneFermion(a2, b2, t, beta) + oneFermion(a3, b3, t, beta));
     }
    }
   } else {
    for (auto& t : gt.mesh())
     if (t.index() < L) {
       gt[t] = convert_green<gt_result_type>(g_out(t.index()) + oneBoson(a1, b1, t, beta) +
                                           oneBoson(a2, b2, t, beta) + oneBoson(a3, b3, t, beta));
     }
   }
   double pm = (gw.domain().statistic == Fermion ? -1.0 : 1.0);
   gt.on_mesh(L) = pm * (gt.on_mesh(0) + convert_green<gt_result_type>(ta(1)(0, 0)));
   // set tail
   gt.singularity() = gw.singularity();
  }

 }; // class worker

 //--------------------------------------------

 // Direct transformation imtime -> imfreq, with a tail
 void _fourier_impl(gf_view<imfreq, scalar_valued, tail> gw, gf_const_view<imtime, scalar_valued, tail> gt) {
  impl_worker w;
  w.direct(gw, gt);
 }

 void _fourier_impl(gf_view<imfreq, scalar_valued, no_tail> gw, gf_const_view<imtime, scalar_valued, no_tail> gt) {
  impl_worker w;
  w.direct(gw, gt);
 }

 // Inverse transformation imfreq -> imtime: tail is mandatory
 void _fourier_impl(gf_view<imtime, scalar_valued, tail> gt, gf_const_view<imfreq, scalar_valued, tail> gw) {
  impl_worker w;
  w.inverse(gt, gw);
 }
}
}

