#include "../common.hpp"
#include <triqs/gfs/block.hpp>

using namespace triqs::clef;
using namespace triqs::lattice;

using triqs::clef::placeholder;
// scalar valued gf_vertex
using gf_vertex_t = gf<cartesian_product<imfreq, imfreq, imfreq>, scalar_valued>;
using gf_vertex_tensor_t = gf<cartesian_product<imfreq, imfreq, imfreq>, tensor_valued<3>>;

using bgf_t = gf <cartesian_product<block_index,block_index>, gf_vertex_t>;

// -----------------------------------------------------

TEST(Gf, BlockOfVertexScalar) {

 double beta = 10.0;
 int n_im_freq = 10;

 auto m = gf_mesh<imfreq>{beta, Fermion, n_im_freq};

 auto vertex = gf_vertex_t{{m, m, m}};

 placeholder<0> w0_;
 placeholder<1> w1_;
 placeholder<2> w2_;
 placeholder<3> s1_;
 placeholder<4> s2_;

 // Now make the block of vertices:
 auto B = make_block2_gf(2, 2, vertex);
 EXPECT_EQ( n_blocks(B), 2);

 // assign a function
 B[{0,0}] = vertex;
 
 // assign expression
 B(s1_,s2_)(w0_, w1_, w2_) << w0_ + (s1_ + 2.3) * w1_ + (s2_*2*3.1) * w2_;

 // checking the result
 auto check = [](int s1, int s2, int i1, int i2, int i3) {
  return (M_PI * (2 * i1 + 1) / 10.0 + (s1 + 2.3) * M_PI * (2 * i2 + 1) / 10.0 + 2 * s2 * 3.1 * M_PI * (2 * i3 + 1) / 10.0) * 1_j;
 };

 for (int u = 0; u < 2; ++u)
  for (int v = 0; v < 2; ++v) EXPECT_CLOSE((B(u, v)[{1, 6, 3}]), check(u, v, 1, 6, 3));

 // Testing map in a very simple case
 std::vector<std::vector<dcomplex>> rr = map([](auto const& g) { return g(0, 0, 0); }, B);

 // checking
 for (int u = 0; u < 2; ++u)
  for (int v = 0; v < 2; ++v) EXPECT_CLOSE(rr[u][v], check(u, v, 0, 0, 0));

 // iterator
 for (auto & x : B) std::cout  << x(0,0,0) <<std::endl;

 // h5
 rw_h5(B, "vertexBlockS", "B");
}

// -----------------------------------------------------

TEST(Gf, VertexTensor) {

 double beta = 10.0;
 int n_im_freq = 10;

 auto m = gf_mesh<imfreq>{beta, Fermion, n_im_freq};

 // now with indices
 auto vertex = gf_vertex_tensor_t{{m, m, m}, {2, 2, 2}};

 vertex[{0, 0, 0}](0, 0, 0) = 10;
 EXPECT_CLOSE((vertex[{0, 0, 0}](0, 0, 0)), 10);

 // Now make the block of vertices:
 auto B = make_block2_gf(2, 2, vertex);
 EXPECT_EQ( n_blocks(B), 2);

 rw_h5(B, "vertexBlockT", "B");

}
MAKE_MAIN;

