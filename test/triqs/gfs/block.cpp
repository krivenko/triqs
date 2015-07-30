#include "./common.hpp"

TEST(Gf, Block) { 
  
  double beta = 1;
  auto G1 = gf<imfreq>({beta, Fermion}, {2, 2});
  auto G2 = G1;

  triqs::clef::placeholder<0> w_;
  G1(w_) << 1 / (w_ + 2);

  auto G3 = G2;

  // construct some block functions
  auto B0 = block_gf<imfreq>(3);

  auto B1 = make_block_gf<imfreq>(3, G1);
  auto B2 = make_block_gf<imfreq>({G1, G1, G1});
  auto B3 = make_block_gf<imfreq>({"a", "b", "c"}, {G1, G1, G1});
  auto B4 = block_gf<imfreq>(1);

  EXPECT_BLOCK_GF_NEAR(B1, rw_h5(B1));

  B1[0][0] = 98;
  // not implemented yet
  // B3["a"][0] = 98;

  auto View = make_block_gf_view(G1, G2, G3);
  EXPECT_EQ(View.mesh().size(), 3);
  
  auto g0 = View[0];
  auto g0v = View[0]();

  auto Gv = g0();

  Gv[0] = 20;
  EXPECT_ARRAY_NEAR (G1(0), matrix<double>{{20,0},{0,20}});

  Gv[0] = 0;
  g0v[0] = 3.2;
  EXPECT_ARRAY_NEAR (G1(0), matrix<double>{{3.2,0.0},{0.0,3.2}});

  // Operation
  g0[0] = 3.2;
  EXPECT_ARRAY_NEAR (View[0](0), matrix<double>{{3.2,0.0},{0.0,3.2}});
  View = View / 2;
  EXPECT_ARRAY_NEAR (View[0](0), matrix<double>{{1.6,0.0},{0.0,1.6}});

  // try the loop over the block.
  for (auto& g : View) {
   g[0] = 20;
  }
  for (auto& g : B1) {
   g[0] = 20;
  }

  // check chaining of clef
  clef::placeholder<0> b_;
  clef::placeholder<1> om_;
  B1[b_][om_] << b_ / (om_ + 2);

  auto B11 = B1;

  B1[b_](om_) << B11[b_](om_) * B1[b_](om_) * B11[b_](om_);

  // test reinterpretation
  // compile only, add more test here
  auto gs1 = gf<imfreq, scalar_valued>({beta, Fermion});
  auto bgs = make_block_gf<imfreq>(3, gs1);
  auto bg = reinterpret_scalar_valued_gf_as_matrix_valued(bgs);

  // inversion
  {
   auto inv_G1 = inverse(G1);
   auto B = make_block_gf<imfreq>(3, G1);
   auto inv_B1 = inverse(B);
   for (auto & g : inv_B1) EXPECT_GF_NEAR(g,inv_G1);
  }

 }
MAKE_MAIN;
