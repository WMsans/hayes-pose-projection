#include <doctest/doctest.h>
#include "skeleton.hpp"
#include "pose_io.hpp"
#include <set>

TEST_CASE("bones reference valid joints and form a tree") {
  const auto& bs = pose::bones();
  CHECK(bs.size() == 13);  // a 14-node tree has 13 edges
  std::set<int> touched;
  for (const auto& b : bs) {
    CHECK(b.a >= 0);
    CHECK(b.a < pose::kJointCount);
    CHECK(b.b >= 0);
    CHECK(b.b < pose::kJointCount);
    CHECK(b.a != b.b);
    touched.insert(b.a);
    touched.insert(b.b);
  }
  CHECK(touched.size() == static_cast<std::size_t>(pose::kJointCount));
}

TEST_CASE("left and right limbs are balanced") {
  int left = 0, right = 0;
  for (const auto& b : pose::bones()) {
    if (b.side == pose::Side::Left) ++left;
    if (b.side == pose::Side::Right) ++right;
  }
  CHECK(left == right);
  CHECK(left == 6);
}
