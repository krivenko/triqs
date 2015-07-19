
/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2014 by O. Parcollet
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

namespace triqs {
namespace gfs {

 //------------------------------------------------------

 // A simple replacement of tail when there is none to maintain generic code simple...

 template <typename Target> struct tail_zero_impl;
 template <> struct tail_zero_impl<scalar_valued> {
  std::complex<double> zero;
  template <typename T> tail_zero_impl(T) { zero = 0; }
  tail_zero_impl() = default;
 };
 template <> struct tail_zero_impl<matrix_valued> {
  matrix<std::complex<double>> zero;
  template <typename S> tail_zero_impl(S const & s) : zero(s) { zero() = 0; }
  tail_zero_impl() = default;
 };
 template <int R> struct tail_zero_impl<tensor_valued<R>> {
  array<std::complex<double>,R> zero;
  template <typename S> tail_zero_impl(S const & s) : zero(s) { zero() = 0; }
  tail_zero_impl() = default;
 };

 template <typename Target> struct tail_zero : tail_zero_impl<Target> {
  using tail_zero_impl<Target>::template tail_zero_impl;
  using const_view_type = tail_zero;
  using view_type = tail_zero;
  using regular_type = tail_zero;
  void rebind(tail_zero) {}
  template <typename RHS> void operator=(RHS &&) {}
  friend void h5_write(h5::group, std::string subgroup_name, tail_zero) {}
  friend void h5_read(h5::group, std::string subgroup_name, tail_zero) {}
  //template <typename... A> friend tail_zero slice(tail_zero const &t, A...) { return t; }
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const unsigned int version) {}
  friend tail_zero operator+(tail_zero, tail_zero) { return tail_zero(); }
  template <typename RHS> friend void assign_singularity_from_function(tail_zero &, RHS) {}
  template<typename A> bool check_size(A) {return true;}
  bool is_empty() const { return false;}

  template<int ... pos, typename T, typename ...X> friend nothing partial_eval_linear_index(tail_zero<T>, X&&...) { return {};}
  template <typename T> friend tail_zero transpose(tail_zero const & t) { return t;}
  //template <typename T> friend tail_zero inverse(tail_zero const & t) { return t;}
  template <typename T> friend tail_zero conj(tail_zero const & t) { return t;}
  template <typename T> friend tail_zero compose(tail_zero const & t,T&) { return t;}
  //template <typename... X> friend tail_zero slice_target(tail_zero const & t, X...) { return t; }

  template<typename ... X> friend auto evaluate(tail_zero const &t, X...) { return t.zero;} 
  };

  // all operations are neutral
  template <typename T, typename X> tail_zero<T> operator+(tail_zero<T>const & t, X const &) { return t; }
  template <typename T, typename X> tail_zero<T> operator-(tail_zero<T>const & t, X const &) { return t; }
  template <typename T, typename X> tail_zero<T> operator*(tail_zero<T>const & t, X const &) { return t; }
  template <typename T, typename X> tail_zero<T> operator/(tail_zero<T>const & t, X const &) { return t; }
  template <typename T, typename X>
  TYPE_DISABLE_IF(tail_zero<T>, std::is_same<X, tail_zero<T>>) operator+(X const &, tail_zero<T> const &t) { return t; }
  template <typename T, typename X>
  TYPE_DISABLE_IF(tail_zero<T>, std::is_same<X, tail_zero<T>>) operator-(X const &, tail_zero<T> const & t) { return t; }
  template <typename T, typename X>
  TYPE_DISABLE_IF(tail_zero<T>, std::is_same<X, tail_zero<T>>) operator*(X const &, tail_zero<T>const & t) { return t;}
  template <typename T, typename X>
  TYPE_DISABLE_IF(tail_zero<T>, std::is_same<X, tail_zero<T>>) operator/(X const &, tail_zero<T>const & t) { return t;}
}

namespace mpi {

 // ---------------------------------------------------------------------------------------
 //  Do nothing for tail_zero...
 // ---------------------------------------------------------------------------------------
 template <typename T> struct mpi_impl<gfs::tail_zero<T>> {
  template <typename Tag> static void invoke2(gfs::tail_zero<T> &lhs, Tag, communicator c, gfs::tail_zero<T> const &a, int root) {}
  template <typename Tag> static gfs::tail_zero<T> invoke(Tag, communicator c, gfs::tail_zero<T> const &a, int root) { return a; }
  static void reduce_in_place(communicator c, gfs::tail_zero<T> &a, int root) {}
  static void broadcast(communicator c, gfs::tail_zero<T> &a, int root) {}
 };
}
}
