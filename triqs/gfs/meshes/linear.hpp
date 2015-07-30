/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2012 by M. Ferrero, O. Parcollet
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
namespace triqs {
namespace gfs {

 /**
  * Linear mesh
  */ 
 template <typename Domain> struct linear_mesh {

  using domain_t = Domain;
  using index_t = long;
  using linear_index_t = long;
  using domain_pt_t = typename domain_t::point_t;
  using default_interpol_policy = interpol_t::Linear1d;

  static_assert(!std::is_base_of<std::complex<double>, domain_pt_t>::value,
                "Internal error : cannot use Linear Mesh in this case");

  linear_mesh() : _dom(), L(0), xmin(0), xmax(0), del(0) {}

  explicit linear_mesh(domain_t dom, double a, double b, long n_pts)
     : _dom(std::move(dom)), L(n_pts), xmin(a), xmax(b), del((b-a)/(L-1)) {}

  domain_t const &domain() const { return _dom; }
  long size() const { return L; }
  
  utility::mini_vector<size_t, 1> size_of_components() const {
   return {size_t(size())};
  }

  double delta() const { return del; }
  double x_max() const { return xmax; }
  double x_min() const { return xmin; }

  /// Conversions point <-> index <-> linear_index
  domain_pt_t index_to_point(index_t ind) const { return xmin + ind * del; }

  long index_to_linear(index_t ind) const { return ind; }

  /// Type of the mesh point
  using mesh_point_t = mesh_point<linear_mesh>;

  /// Accessing a point of the mesh
  mesh_point_t operator[](index_t i) const {
   return {*this, i};
  }

  /// Iterating on all the points...
  using const_iterator = mesh_pt_generator<linear_mesh>;
  const_iterator begin() const { return const_iterator(this); }
  const_iterator end() const { return const_iterator(this, true); }
  const_iterator cbegin() const { return const_iterator(this); }
  const_iterator cend() const { return const_iterator(this, true); }

  /// Mesh comparison
  bool operator==(linear_mesh const &M) const {
   return ((_dom == M._dom) && (size() == M.size()) && (std::abs(xmin - M.xmin) < 1.e-15) && (std::abs(xmax - M.xmax) < 1.e-15));
  }
  bool operator!=(linear_mesh const &M) const { return !(operator==(M)); }

  /// Is the point in mesh ?
  bool is_within_boundary(index_t const &p) const { return ((p >= x_min()) && (p <= x_max())); }
  // ADAPT for window
  //bool is_within_boundary(index_t const &p) const { return ((p >= first_index_window()) && (p <= last_index_window())); }

  // -------------- Evaluation of a function on the grid --------------------------
  struct interpol_data_t {
   double w0, w1;
   long i0, i1;
  };

  interpol_data_t get_interpolation_data(interpol_t::Linear1d, double x) const {
   double w;
   long i;
   bool in;
   std::tie(in, i, w) = windowing(*this, x);
   if (!in) TRIQS_RUNTIME_ERROR <<"out of window";
   return {1- w, w, i, i + 1};
  }

  template<typename F>
  auto evaluate(interpol_t::Linear1d, F const & f, double x) const {
   auto id = get_interpolation_data(default_interpol_policy{}, x);
   return id.w0 * f[id.i0] + id.w1 * f[id.i1];
  }

  // -------------- HDF5  --------------------------
  /// Write into HDF5
  friend void h5_write(h5::group fg, std::string subgroup_name, linear_mesh const &m) {
   h5::group gr = fg.create_group(subgroup_name);
   h5_write(gr, "domain", m.domain());
   h5_write(gr, "min", m.xmin);
   h5_write(gr, "max", m.xmax);
   h5_write(gr, "size", long(m.size()));
  }

  /// Read from HDF5
  friend void h5_read(h5::group fg, std::string subgroup_name, linear_mesh &m) {
   h5::group gr = fg.open_group(subgroup_name);
   typename linear_mesh::domain_t dom;
   double a, b;
   long L;
   h5_read(gr, "domain", dom);
   h5_read(gr, "min", a);
   h5_read(gr, "max", b);
   h5_read(gr, "size", L);
   m = linear_mesh(std::move(dom), a, b, L);
  }

  //  BOOST Serialization
  friend class boost::serialization::access;
  template <class Archive> void serialize(Archive &ar, const unsigned int version) {
   ar &TRIQS_MAKE_NVP("domain", _dom);
   ar &TRIQS_MAKE_NVP("xmin", xmin);
   ar &TRIQS_MAKE_NVP("xmax", xmax);
   ar &TRIQS_MAKE_NVP("del", del);
   ar &TRIQS_MAKE_NVP("size", L);
  }

  friend std::ostream &operator<<(std::ostream &sout, linear_mesh const &m) { return sout << "Linear Mesh of size " << m.L; }

  private:
  domain_t _dom;
  long L;
  double xmin, xmax, del;
 };

 // ---------------------------------------------------------------------------
 //                     The mesh point
 // ---------------------------------------------------------------------------

 template <typename Domain>
 struct mesh_point<linear_mesh<Domain>> : public utility::arithmetic_ops_by_cast<mesh_point<linear_mesh<Domain>>,
                                                                                 typename Domain::point_t> {
  using mesh_t = linear_mesh<Domain>;
  using index_t = typename mesh_t::index_t;
  mesh_t const *m;
  index_t _index;

  public:
  mesh_point() : m(nullptr) {}
  mesh_point(mesh_t const &mesh, index_t const &index_) : m(&mesh), _index(index_) {}
  mesh_point(mesh_t const &mesh) : mesh_point(mesh, 0) {}
  void advance() { ++_index; }
  using cast_t = typename Domain::point_t;
  operator cast_t() const { return m->index_to_point(_index); }
  long linear_index() const { return _index; }
  long index() const { return _index; }
  bool at_end() const { return (_index == m->size()); }
  void reset() { _index = 0; }
 };

 // UNUSED, only in deprecated code...
 /// Simple approximation of a point of the domain by a mesh point. No check
 //template <typename D> long get_closest_mesh_pt_index(linear_mesh<D> const &mesh, typename D::point_t const &x) {
 // double a = (x - mesh.x_min()) / mesh.delta();
//  return std::floor(a);
// }

 /// Approximation of a point of the domain by a mesh point
 template <typename D> std::tuple<bool, long, double> windowing(linear_mesh<D> const &mesh, typename D::point_t const &x) {
  double a = (x - mesh.x_min()) / mesh.delta();
  long i = std::floor(a), imax = long(mesh.size()) - 1;
  bool in = (i >= 0) && (i < imax);
  double w = a - i;
  if (i == imax) {
   --i;
   in = (std::abs(w) < 1.e-12);
   w = 1.0;
  }
  if (i == -1) {
   i = 0;
   in = (std::abs(1 - w) < 1.e-12);
   w = 1.0;
  }
  return std::make_tuple(in, i, w);
  // return std::make_tuple(in, (in ? i : 0),w);
 }
}
}

