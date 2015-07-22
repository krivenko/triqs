/*******************************************************************************
 *
 * TRIQS: a Toolbox for Research in Interacting Quantum Systems
 *
 * Copyright (C) 2015 by O. Parcollet
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
#include <iostream>
#include <sstream>
#include "gtest/gtest.h"

// print a vector ?
//template <typename T> 
//std::ostream & operator << (std::ostream & out, std::vector<T> const & v) { 
//for (size_t i =0; i<v.size(); ++i) out<< v[i];
//return out;
//}

#define MAKE_MAIN \
 int main(int argc, char **argv) {\
  ::testing::InitGoogleTest(&argc, argv);\
  return RUN_ALL_TESTS();\
}

// Arrays are equal 
template<typename X, typename Y>
::testing::AssertionResult array_are_equal(X const &x, Y const &y) {
 if (max_element(abs(x - y)) ==0)
  return ::testing::AssertionSuccess();
 else
  return ::testing::AssertionFailure() << "max_element(abs(x-y)) = " << max_element(abs(x - y)) << "\n X = "<<  x << "\n Y = "<< y;
}

#define EXPECT_EQ_ARRAY(X, Y) EXPECT_TRUE(array_are_equal(X,Y));

// Arrays are close 
template<typename X, typename Y>
::testing::AssertionResult array_are_close(X const &x, Y const &y) {
 double precision = 1.e-10;
 if (max_element(abs(x - y)) < precision)
  return ::testing::AssertionSuccess();
 else
  return ::testing::AssertionFailure() << "max_element(abs(x-y)) = " << max_element(abs(x - y)) << "\n X = "<<  x << "\n Y = "<< y;
}

#define EXPECT_CLOSE_ARRAY(X, Y) EXPECT_TRUE(array_are_close(X,Y));

