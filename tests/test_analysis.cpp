#include <doctest/doctest.h>
#include "analysis.hpp"
#include <fstream>
#include <sstream>

TEST_CASE("joint_error reports mean, max and the worst joint") {
  const std::vector<glm::dvec2> a{{0, 0}, {0, 0}, {0, 0}};
  const std::vector<glm::dvec2> b{{3, 4}, {0, 0}, {0, 1}};  // distances 5, 0, 1
  const auto e = pose::joint_error(a, b);
  CHECK(e.mean_px == doctest::Approx(2.0));
  CHECK(e.max_px == doctest::Approx(5.0));
  CHECK(e.worst_joint == 0);
}

TEST_CASE("angular_error is zero for identical rotations") {
  const glm::dmat3 I(1.0);
  CHECK(pose::angular_error_degrees(I, I) == doctest::Approx(0.0));
}

TEST_CASE("angular_error measures a known 90 degree rotation") {
  glm::dmat3 R(0.0);
  R[0][1] = 1.0;   // x -> y
  R[1][0] = -1.0;  // y -> -x
  R[2][2] = 1.0;
  CHECK(pose::angular_error_degrees(glm::dmat3(1.0), R) == doctest::Approx(90.0).epsilon(1e-9));
}

TEST_CASE("limb_lengths are positive and stable under translation") {
  pose::Frame f;
  for (int i = 0; i < pose::kJointCount; ++i) f.joints[i] = {i * 10.0, 0.0, 0.0};
  const auto a = pose::limb_lengths(f);
  for (auto& j : f.joints) j += glm::dvec3{1000.0, -500.0, 25.0};
  const auto b = pose::limb_lengths(f);
  for (std::size_t i = 0; i < a.size(); ++i) {
    CHECK(a[i] > 0.0);
    CHECK(a[i] == doctest::Approx(b[i]));
  }
}

TEST_CASE("fixed formats deterministically") {
  CHECK(pose::fixed(1.0 / 3.0, 3) == "0.333");
  CHECK(pose::fixed(-0.0004, 3) == "-0.000");
}

TEST_CASE("write_csv emits a header and quoted-free rows") {
  pose::write_csv("build/test.csv", {"a", "b"}, {{"1", "2"}, {"3", "4"}});
  std::ifstream in("build/test.csv");
  std::stringstream ss;
  ss << in.rdbuf();
  CHECK(ss.str() == "a,b\n1,2\n3,4\n");
}
