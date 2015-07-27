/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2015 by I. Krivenko
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
#include <type_traits>
#include <tuple>
#include <array>
#include <ostream>

#include <triqs/utility/exceptions.hpp>
#include <triqs/utility/c14.hpp>

namespace triqs {
namespace utility {

// Too bad, std::aligned_union is a part of C++11, but it is supported only by g++ 5.1
// This implementation has to be a bit messy, if we want to support g++ 4.8 (cf gcc bug 56859)
template <typename... Types>
struct aligned_union {

  // Find a type of the maximum size
  template<typename T0, typename... Tail>
  struct max_size_t {
    using type = std14::conditional_t<(sizeof(T0)>sizeof(typename max_size_t<Tail...>::type)),
                                      T0,
                                      typename max_size_t<Tail...>::type>;
  };
  template<typename T0> struct max_size_t<T0> { using type = T0; };

  // Find a type of the maximum alignment value
  template<typename T0, typename... Tail>
  struct max_alignment_t {
    using type = std14::conditional_t<(alignof(T0)>alignof(typename max_alignment_t<Tail...>::type)),
                                      T0,
                                      typename max_alignment_t<Tail...>::type>;
  };
  template<typename T0> struct max_alignment_t<T0> { using type = T0; };

  struct type {
    alignas(typename max_alignment_t<Types...>::type) char _[sizeof(typename max_size_t<Types...>::type)];
  };
};

template<typename... Types>
class variant {

public:

  using bound_types = std::tuple<Types...>;
  constexpr static std::size_t n_bound_types = sizeof...(Types);
  template<std::size_t n> using bound_type = std14::tuple_element_t<0,bound_types>;

private:

  static_assert(n_bound_types > 0,"triqs::utility::variant: list of types must not be empty!");
  typename aligned_union<Types...>::type data; // Storage
  std::size_t type_id; // Type ID of the stored value

  // Support for Boost.Serialization
  template<typename Archive, typename... Types_>
  friend void boost::serialization::save(Archive & ar, variant<Types_...> const& v, const unsigned int version);
  template<typename Archive, typename... Types_>
  friend void boost::serialization::load(Archive & ar, variant<Types_...> & v, const unsigned int version);

  // Find first of the Types... that T can be converted to
  template<typename T, typename T0, typename... Tail> struct find_matching_type {
    using type = std14::conditional_t<std::is_convertible<T,T0>::value,T0,
                                      typename find_matching_type<T,Tail...>::type>;
    constexpr static std::size_t id = std::is_convertible<T,T0>::value ?
                                 n_bound_types - sizeof...(Tail) - 1 :
                                 find_matching_type<T,Tail...>::id;
  };
  template<typename T, typename T0> struct find_matching_type<T,T0> {
    using type = std14::conditional_t<std::is_convertible<T,T0>::value,T0,void>;
    constexpr static std::size_t id = std::is_convertible<T,T0>::value ? n_bound_types - 1 : -1;
  };

  // Implementation: destructor
  template<typename T> void destroy() {
    if(!std::is_trivially_destructible<T>::value)
      reinterpret_cast<T*>(&data)->~T();
  }

  // Implementation: copy-constructor
  template<typename T> void copy(variant const& other) {
    ::new(&data) T(*reinterpret_cast<T const*>(&other.data));
  }

  // Implementation: move-constructor
  template<typename T> void move(variant && other) {
    ::new(&data) T(*reinterpret_cast<T*>(&other.data));
  }

  // Implementation: assignment
  template<typename T> void assign(variant const& other) {
    *reinterpret_cast<T*>(&data) = *reinterpret_cast<T const*>(&other.data);
  }

  // Implementation: equality comparison
  template<typename T> bool equal(variant const& other) const {
    return *reinterpret_cast<T const*>(&data) ==
           *reinterpret_cast<T const*>(&other.data);
  }

  // Implementation: less-than comparison
  template<typename T> bool less(variant const& other) const {
    return *reinterpret_cast<T const*>(&data) <
           *reinterpret_cast<T const*>(&other.data);
  }

  // Implementation: stream insertion
  template<typename T> void print(std::ostream & os) const {
    os << *reinterpret_cast<T const*>(&data);
  }

  // Implementation: visitation
  template<typename T, typename F>
  std14::result_of_t<F(T&)> apply(F f) {
    return f(*reinterpret_cast<T*>(&data));
  }
  template<typename T, typename F>
  std14::result_of_t<F(T)> apply(F f) const { // const version
    return f(*reinterpret_cast<T const*>(&data));
  }

  // Declare dispatch tables
#define DECLARE_DT(NAME,RTYPE,ARGS) \
  constexpr static std::array<RTYPE(variant::*)ARGS,n_bound_types> \
  NAME##_dt = {&variant::NAME<Types>...};
  DECLARE_DT(destroy,      void,())
  DECLARE_DT(copy,         void,(variant<Types...> const&))
  DECLARE_DT(move,         void,(variant<Types...> &&))
  DECLARE_DT(assign,       void,(variant<Types...> const&))
  DECLARE_DT(equal,        bool,(variant<Types...> const&) const)
  DECLARE_DT(less,         bool,(variant<Types...> const&) const)
  DECLARE_DT(print,        void,(std::ostream & os) const)
#undef DECLARE_DT

public:

  // Constructor
  template<typename T, typename MatchingType = find_matching_type<T,Types...>,
           typename = std14::enable_if_t<MatchingType::id != -1>>
  variant(T v) {
    ::new(&data) typename MatchingType::type(v);
    type_id = MatchingType::id;
  }

  // Copy-constructor
  variant(variant const& other) : type_id(other.type_id) {
    (this->*copy_dt[type_id])(other);
  }

  // Move-constructor
  variant(variant && other) : type_id(other.type_id) {
    (this->*move_dt[type_id])(other);
  }

  // Destructor
  ~variant() { (this->*destroy_dt[type_id])(); }

  // Assignment
  variant & operator=(variant const& other) {
    if(type_id == other.type_id) {
      (this->*assign_dt[type_id])(other);
    } else {
      (this->*destroy_dt[type_id])();
      type_id = other.type_id;
      (this->*copy_dt[type_id])(other);
    }
    return *this;
  }

  // Visitation
  // Return type of f(v) must be the same for any stored type,
  // that is why we can use the first type.
  template <typename F, typename RType = std14::result_of_t<F(bound_type<0>&)>>
  friend RType apply_visitor(F &&f, variant & v) {
    constexpr static std::array<RType(variant::*)(F),n_bound_types> apply_dt = {&variant::apply<Types,F>...};
    return (v.*apply_dt[v.type_id])(f);
  }
  template <typename F, typename RType = std14::result_of_t<F(bound_type<0>)>>
  friend RType apply_visitor(F &&f, variant const& v) { // const version
    constexpr static std::array<RType(variant::*)(F) const,n_bound_types> apply_dt = {&variant::apply<Types,F>...};
    return (v.*apply_dt[v.type_id])(f);
  }

  // Comparisons
  bool operator==(variant const& other) const {
     if(type_id != other.type_id)
       TRIQS_RUNTIME_ERROR << "triqs::utility::variant: cannot compare stored values of different types";
     return (this->*equal_dt[type_id])(other);
  }
  bool operator!=(variant const& other) const { return !(operator==(other)); }

  bool operator<(variant const& other) const {
     if(type_id != other.type_id)
       TRIQS_RUNTIME_ERROR << "triqs::utility::variant: cannot compare stored values of different types";
     return (this->*less_dt[type_id])(other);
  }

  // Stream insertion
  friend std::ostream & operator<<(std::ostream & os, variant const &v) {
    (v.*print_dt[v.type_id])(os);
    return os;
  }

};

// Define dispatch tables
#define DEFINE_DT(NAME,RTYPE,ARGS) \
template<typename... Types> \
constexpr std::array<RTYPE(variant<Types...>::*)ARGS,variant<Types...>::n_bound_types> \
variant<Types...>::NAME##_dt;

DEFINE_DT(destroy,      void,())
DEFINE_DT(copy,         void,(variant<Types...> const&))
DEFINE_DT(move,         void,(variant<Types...> &&))
DEFINE_DT(assign,       void,(variant<Types...> const&))
DEFINE_DT(equal,        bool,(variant<Types...> const&) const)
DEFINE_DT(less,         bool,(variant<Types...> const&) const)
DEFINE_DT(print,        void,(std::ostream & os) const)
#undef DEFINE_DT

}
}
