#include "test_tools.hpp"
#include <triqs/utility/serialization.hpp>
#include <vector>

TEST(Serialization, Boost) {

 double x = 127;
 EXPECT_EQ( x, triqs::deserialize<double>(triqs::serialize(x)));

 auto v = std::vector<std::string> {"abc","3"};
 EXPECT_EQ( v, triqs::deserialize<std::vector<std::string>>(triqs::serialize(v)));

}
MAKE_MAIN;

