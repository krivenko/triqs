#define TRIQS_ARRAYS_ENFORCE_BOUNDCHECK
#include <triqs/gfs.hpp>
#include "../common.hpp"

namespace h5 = triqs::h5;
using namespace triqs::gfs;
using namespace triqs::clef;
using namespace triqs::arrays;
using namespace triqs::lattice;

#define TEST(X) std::cout << BOOST_PP_STRINGIZE((X)) << " ---> " << (X) << std::endl << std::endl;

int main() {
 try {
  double beta = 1;
  auto bz = brillouin_zone{bravais_lattice{make_unit_matrix<double>(2)}};

  int n_freq = 100;
  int n_times = n_freq * 2 + 1;
  int n_bz = 50;
  auto gkw = gf<cartesian_product<brillouin_zone, imfreq>, matrix_valued, m_tail<brillouin_zone>>{
      {{bz, n_bz}, {beta, Fermion, n_freq}}, {1, 1}};
  auto gkt = gf<cartesian_product<brillouin_zone, imtime>, matrix_valued, m_tail<brillouin_zone>>{
      {{bz, n_bz}, {beta, Fermion, n_times}}, {1, 1}};

  placeholder<0> k_;
  placeholder<1> w_;
  //placeholder<2> tau_;

  auto eps_k = -2 * (cos(k_(0)) + cos(k_(1)));
  gkw(k_, w_) << 1 / (w_ - eps_k - 1 / (w_ + 2));

  auto gk_w = curry<0>(gkw);
  auto gk_t = curry<0>(gkt);

  gk_t[k_] << inverse_fourier(gk_w[k_]);
 
  // works also, but uses the evaluator which return to the same point
  // gk_t(k_) << inverse_fourier(gk_w(k_));
  // check last assertion
  for (auto k : gk_t.mesh()) assert_equal(k.linear_index(), gk_t.mesh().index_to_linear(gk_t.mesh().locate_neighbours(k)), "k location point");
 
  /// Testing the result
  auto gk_w_test = gf<imfreq>{{beta, Fermion, n_freq}, {1, 1}};
  auto gk_t_test = gf<imtime>{{beta, Fermion, n_times}, {1, 1}};
  assert_equal_array(gkt.singularity().data().data, gkw.singularity().data().data, "Error 05");
  for (auto & k : std::get<0>(gkw.mesh().components())) {
   gk_w_test(w_) << 1 / (w_ - eval(eps_k, k_ = k) - 1 / (w_ + 2));
   gk_t_test() = inverse_fourier(gk_w_test);
   assert_equal_array(gk_w_test.singularity().data(), gk_w[k].singularity().data(), "Error 0");
   assert_equal_array(gk_t_test.singularity().data(), gk_t[k].singularity().data(), "Error 0s");
   assert_equal_array(gk_w_test.data(), gk_w[k].data(), "Error 1");
   assert_equal_array(gk_t_test.data(), gk_t[k].data(), "Error 2");
  }

  // hdf5
  h5::file file("ess_g_k_om.h5", H5F_ACC_TRUNC);
  h5_write(file, "g", gkw);
 }
 TRIQS_CATCH_AND_ABORT;
}
