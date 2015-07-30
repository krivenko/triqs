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
#include "./mesh_tools.hpp"
#include "../domains/matsubara.hpp"

namespace triqs {
namespace gfs {

 struct imfreq {};

 template <> struct mesh_point<gf_mesh<imfreq>>; //forward

 // ---------------------------------------------------------------------------
 //                     The mesh point
 //  NB : the mesh point is also in this case a matsubara_freq.
 // ---------------------------------------------------------------------------
 
 template <> struct gf_mesh<imfreq> {
  using domain_t = matsubara_domain<true>;
  using index_t = long;
  using linear_index_t = long;
  using default_interpol_policy = interpol_t::None;
  using domain_pt_t = typename domain_t::point_t;

  // -------------------- Constructors -------------------

  gf_mesh(domain_t dom, long n_pts = 1025, bool positive_only = true)
     : _dom(std::move(dom)), _n_pts(n_pts), _positive_only(positive_only) {
   if (_positive_only) {
    _first_index = 0;
    _last_index = n_pts - 1; 
   } else {
    _last_index = (_n_pts - (_dom.statistic == Boson ? 1 : 2)) / 2;
    _first_index = -(_last_index + (_dom.statistic == Fermion));
   }
   _first_index_window = _first_index;
   _last_index_window = _last_index;
  }

  gf_mesh() : gf_mesh(domain_t(), 0, true){}

  gf_mesh(double beta, statistic_enum S, int n_pts = 1025, bool positive_only = true)
     : gf_mesh({beta, S}, n_pts, positive_only) {}

  bool operator==(gf_mesh const &M) const {
   return (std::tie(_dom, _n_pts, _positive_only) == std::tie(M._dom, M._n_pts, M._positive_only));
  }
  bool operator!=(gf_mesh const &M) const { return !(operator==(M)); }

  // -------------------- Accessors (from concept) -------------------

  /// The corresponding domain
  domain_t const &domain() const { return _dom; }

  /// Size (linear) of the mesh of the window
  long size() const { return _last_index_window - _first_index_window + 1; }
  
  /// Size (linear) of the mesh of the window
  long full_size() const { return _last_index - _first_index + 1; }

  /// 
  utility::mini_vector<size_t, 1> size_of_components() const {
   return {size_t(size())};
  }

  /// From an index of a point in the mesh, returns the corresponding point in the domain
  domain_pt_t index_to_point(index_t ind) const { return 1_j * M_PI * (2 * ind + (_dom.statistic == Fermion)) / _dom.beta; }

  /// Flatten the index in the positive linear index for memory storage (almost trivial here).
  long index_to_linear(index_t ind) const { return ind - first_index_window(); }

  // -------------------- Accessors (other) -------------------

  /// first Matsubara index
  int first_index() const { return _first_index;}
  
  /// last Matsubara index 
  int last_index() const { return _last_index;}

  /// first Matsubara index of the window
  int first_index_window() const { return _first_index_window;}
  
  /// last Matsubara index of the window 
  int last_index_window() const { return _last_index_window;}

 /// Is the mesh only for positive omega_n (G(tau) real))
  bool positive_only() const { return _positive_only;}

  // -------------------- mesh_point -------------------

  /// Type of the mesh point
  using mesh_point_t = mesh_point<gf_mesh>;

  /// Accessing a point of the mesh from its index
  inline mesh_point_t operator[](index_t i) const; //impl below

  /// Iterating on all the points...
  using const_iterator = mesh_pt_generator<gf_mesh>;
  inline const_iterator begin() const;  // impl below
  inline const_iterator end() const;    
  inline const_iterator cbegin() const; 
  inline const_iterator cend() const;   

 // -------------- Evaluation of a function on the grid --------------------------

  /// Is the point in mesh ?
  bool is_within_boundary(long n) const { return ((n >= first_index_window()) && (n <= last_index_window())); }
  bool is_within_boundary(matsubara_freq const &f) const { return is_within_boundary(f.n);}

  long get_interpolation_data(interpol_t::None, long n) const { return n;}
  long get_interpolation_data(interpol_t::None, matsubara_freq n) const { return n.n;}
 
#ifndef TRIQS_CPP11 
  template <typename F> auto evaluate(interpol_t::None, F const &f, long n) const { return f[n]; }
  template <typename F> auto evaluate(interpol_t::None, F const &f, matsubara_freq n) const { return f[n.n]; }
#else
  template <typename F> auto evaluate(interpol_t::None, F const &f, long n) const RETURN(f[n]);
  template <typename F> auto evaluate(interpol_t::None, F const &f, matsubara_freq n) const RETURN(f[n.n]);
#endif

  // -------------------- MPI -------------------

  /// Scatter a mesh over the communicator c
  //In practice, the same mesh, with a different window.
  //the window can only be set by these 2 operations
  friend gf_mesh mpi_scatter(gf_mesh m, mpi::communicator c, int root) {
   auto m2 = gf_mesh{m.domain(), m.size(), m.positive_only()};
   std::tie(m2._first_index_window, m2._last_index_window) = mpi::slice_range(m2._first_index, m2._last_index, c.size(), c.rank());
   return m2;
  }

  /// Opposite of scatter
  friend gf_mesh mpi_gather(gf_mesh m, mpi::communicator c, int root) {
   return gf_mesh{m.domain(), m.full_size(), m.positive_only()};
  }

  // -------------------- HDF5 -------------------
 
  /// Write into HDF5
  friend void h5_write(h5::group fg, std::string subgroup_name, gf_mesh const &m) {
   h5::group gr = fg.create_group(subgroup_name);
   h5_write(gr, "domain", m.domain());
   h5_write(gr, "size", long(m.size()));
   h5_write(gr, "positive_freq_only", (m._positive_only?1:0));
  }

  /// Read from HDF5
  friend void h5_read(h5::group fg, std::string subgroup_name, gf_mesh &m) {
   h5::group gr = fg.open_group(subgroup_name);
   typename gf_mesh::domain_t dom;
   long L;
   int s = 1;
   h5_read(gr, "domain", dom);
   h5_read(gr, "size", L);
   // backward compatibility : older file do not have this flags, default is true. 
   if (gr.has_key("positive_freq_only")) h5_read(gr, "positive_freq_only", s);
   if (gr.has_key("start_at_0")) h5_read(gr, "start_at_0", s);
   m = gf_mesh{std::move(dom), L, (s==1)};
  }

  // -------------------- boost serialization -------------------

  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const unsigned int version) {
   ar &TRIQS_MAKE_NVP("beta", _dom.beta);
   ar &TRIQS_MAKE_NVP("statistic", _dom.statistic);
   ar &TRIQS_MAKE_NVP("size", _n_pts);
   ar &TRIQS_MAKE_NVP("kind", _positive_only);
   ar &TRIQS_MAKE_NVP("_first_index", _first_index);
   ar &TRIQS_MAKE_NVP("_last_index", _last_index);
   ar &TRIQS_MAKE_NVP("_first_index_window", _first_index_window);
   ar &TRIQS_MAKE_NVP("_last_index_window", _last_index_window);
  }

  // -------------------- print  -------------------
  
  friend std::ostream &operator<<(std::ostream &sout, gf_mesh const &m) {
   return sout << "Matsubara Freq Mesh of size " << m.size();
  }

  // ------------------------------------------------
  private:
  domain_t _dom;
  int _n_pts;
  bool _positive_only;
  long _first_index, _last_index, _first_index_window, _last_index_window;
 };

 // ---------------------------------------------------------------------------
 //                     The mesh point
 //  NB : the mesh point is also in this case a matsubara_freq.
 // ---------------------------------------------------------------------------
 
 template <> struct mesh_point<gf_mesh<imfreq>> : matsubara_freq {
  using index_t = typename gf_mesh<imfreq>::index_t;
  mesh_point() = default;
  mesh_point(gf_mesh<imfreq> const &mesh, index_t const &index_)
     : matsubara_freq(index_, mesh.domain().beta, mesh.domain().statistic)
     , first_index_window(mesh.first_index_window())
     , last_index_window(mesh.last_index_window()) {}
  mesh_point(gf_mesh<imfreq> const &mesh) : mesh_point(mesh, mesh.first_index_window()) {}
  void advance() { ++n; }
  long linear_index() const { return n - first_index_window; }
  long index() const { return n; }
  bool at_end() const { return (n == last_index_window + 1); } // at_end means " one after the last one", as in STL
  void reset() { n = first_index_window; }

  private:
  index_t first_index_window, last_index_window;
 };

 // ------------------- implementations -----------------------------
 inline mesh_point<gf_mesh<imfreq>> gf_mesh<imfreq>::operator[](index_t i) const {
  return {*this, i};
 }

 inline gf_mesh<imfreq>::const_iterator gf_mesh<imfreq>::begin() const { return const_iterator(this); }
 inline gf_mesh<imfreq>::const_iterator gf_mesh<imfreq>::end() const { return const_iterator(this, true); }
 inline gf_mesh<imfreq>::const_iterator gf_mesh<imfreq>::cbegin() const { return const_iterator(this); }
 inline gf_mesh<imfreq>::const_iterator gf_mesh<imfreq>::cend() const { return const_iterator(this, true); }

 //-------------------------------------------------------

 /** \brief foreach for this mesh
  *
  *  @param m : a mesh
  *  @param F : a function of synopsis auto F (matsubara_freq_mesh::mesh_point_t)
  *
  *  Calls F on each point of the mesh, in arbitrary order.
  **/
 template <typename Lambda> void foreach(gf_mesh<imfreq> const &m, Lambda F) {
  for (auto const &w : m) F(w);
 }
}
}

