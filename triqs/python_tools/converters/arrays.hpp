#pragma once
#include "../wrapper_tools.hpp"
#include <triqs/arrays.hpp>

// in the generated wrapper, we only "import_array" if this macro is defined.
#define TRIQS_IMPORTED_CONVERTERS_ARRAYS

namespace triqs {
namespace py_tools {

 // --- mini_vector<T,N>---
 // via std::vector
 template <typename T, int N> struct py_converter<triqs::utility::mini_vector<T, N>> {
  using conv = py_converter<std::vector<T>>;
  static PyObject *c2py(triqs::utility::mini_vector<T, N> const &v) { return conv::c2py(v.to_vector()); }
  static bool is_convertible(PyObject *ob, bool raise_exception) { return conv::is_convertible(ob, raise_exception); }
  static triqs::utility::mini_vector<T, N> py2c(PyObject *ob) { return conv::py2c(ob); }
 };

 // --- array

 inline static void import_numpy() {
  static bool init = false;
  if (!init) {
   _import_array();
   // std::cerr << "importing array"<<std::endl;
   init = true;
  }
 }

 template <typename ArrayType> struct py_converter_array {
  static PyObject *c2py(ArrayType const &x) {
   import_numpy();
   return x.to_python();
  }
  static ArrayType py2c(PyObject *ob) {
   import_numpy();
   return ArrayType(ob);
  }
  static bool is_convertible(PyObject *ob, bool raise_exception) {
   import_numpy();
   try {
    py2c(ob);
    return true;
   }
   catch (std::exception const &e) {
    if (raise_exception) {
     auto mess = std::string("Cannot convert to array/matrix/vector : the error was : \n") + e.what();
     PyErr_SetString(PyExc_TypeError, mess.c_str());
    }
    return false;
   }
  }
 };

 template <typename T, int R>
 struct py_converter<triqs::arrays::array_view<T, R>> : py_converter_array<triqs::arrays::array_view<T, R>> {};
 template <typename T> struct py_converter<triqs::arrays::matrix_view<T>> : py_converter_array<triqs::arrays::matrix_view<T>> {};
 template <typename T> struct py_converter<triqs::arrays::vector_view<T>> : py_converter_array<triqs::arrays::vector_view<T>> {};

 template <typename T, int R>
 struct py_converter<triqs::arrays::array_const_view<T, R>> : py_converter_array<triqs::arrays::array_const_view<T, R>> {};
 template <typename T>
 struct py_converter<triqs::arrays::matrix_const_view<T>> : py_converter_array<triqs::arrays::matrix_const_view<T>> {};
 template <typename T>
 struct py_converter<triqs::arrays::vector_const_view<T>> : py_converter_array<triqs::arrays::vector_const_view<T>> {};

 template <typename T, int R> struct py_converter<triqs::arrays::array<T, R>> : py_converter_array<triqs::arrays::array<T, R>> {};
 template <typename T> struct py_converter<triqs::arrays::matrix<T>> : py_converter_array<triqs::arrays::matrix<T>> {};
 template <typename T> struct py_converter<triqs::arrays::vector<T>> : py_converter_array<triqs::arrays::vector<T>> {};

 // --- range

 // range can not be directly converted from slice (slice is more complex)
 // convert from python slice and int (interpreted are slice(i,i+1,1))
 triqs::arrays::range range_from_slice(PyObject *ob, long len) {
  if (PyInt_Check(ob)) {
   long i = PyInt_AsLong(ob);
   if ((i < -len) || (i >= len)) TRIQS_RUNTIME_ERROR << "Integer index out of range : expected [0," << len << "], got " << i;
   if (i < 0) i += len;
   // std::cerr  << " range int "<< i << std::endl;
   return {i, i + 1, 1};
  }
  Py_ssize_t start, stop, step, slicelength;
  if (!PySlice_Check(ob) || (PySlice_GetIndicesEx((PySliceObject *)ob, len, &start, &stop, &step, &slicelength) < 0))
   TRIQS_RUNTIME_ERROR << "Can not converted the slice to C++";
  // std::cerr  << "range ( "<< start << " "<< stop << " " << step<<std::endl;
  return {start, stop, step};
 }

 template <> struct py_converter<triqs::arrays::range> {
  static PyObject *c2py(triqs::arrays::range const &r) {
   return PySlice_New(convert_to_python(r.first()), convert_to_python(r.last()), convert_to_python(r.step()));
  }
  static triqs::arrays::range py2c(PyObject *ob) = delete;
  static bool is_convertible(PyObject *ob, bool raise_exception) = delete;
 };
}
}

