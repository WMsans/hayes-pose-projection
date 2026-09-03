#include <doctest/doctest.h>
#include "skeleton.hpp"
#include "pose_io.hpp"
#include <array>
#include <functional>
#include <set>
#include <utility>
#include <vector>

TEST_CASE("bones reference valid joints and form a tree") {
  const auto& bs = pose::bones();
  CHECK(bs.size() == 13);  // a 14-node tree has 13 edges
  std::set<int> touched;
  std::array<std::vector<std::pair<int, int>>, pose::kJointCount> graph;
  bool valid = true;
  for (int edge = 0; edge < static_cast<int>(bs.size()); ++edge) {
    const auto& b = bs[edge];
    CHECK(b.a >= 0);
    CHECK(b.a < pose::kJointCount);
    CHECK(b.b >= 0);
    CHECK(b.b < pose::kJointCount);
    CHECK(b.a != b.b);
    if (b.a < 0 || b.a >= pose::kJointCount || b.b < 0 ||
        b.b >= pose::kJointCount || b.a == b.b) {
      valid = false;
      continue;
    }
    touched.insert(b.a);
    touched.insert(b.b);
    graph[b.a].emplace_back(b.b, edge);
    graph[b.b].emplace_back(b.a, edge);
  }
  CHECK(touched.size() == static_cast<std::size_t>(pose::kJointCount));

  if (valid) {
    std::set<int> visited;
    bool acyclic = true;
    std::function<void(int, int)> visit = [&](int joint, int parent_edge) {
      visited.insert(joint);
      for (const auto [neighbor, edge] : graph[joint]) {
        if (edge == parent_edge) continue;
        if (visited.contains(neighbor)) {
          acyclic = false;
        } else {
          visit(neighbor, edge);
        }
      }
    };
    visit(0, -1);
    CHECK(visited.size() == static_cast<std::size_t>(pose::kJointCount));
    CHECK(acyclic);
  }
}

TEST_CASE("left and right limbs are balanced and correctly classified") {
  constexpr std::array<pose::Bone, 13> expected{{
      {0, 1, pose::Side::Right},   {1, 2, pose::Side::Right},
      {2, 3, pose::Side::Right},   {0, 4, pose::Side::Left},
      {4, 5, pose::Side::Left},    {5, 6, pose::Side::Left},
      {0, 7, pose::Side::Torso},   {7, 8, pose::Side::Left},
      {8, 9, pose::Side::Left},    {9, 10, pose::Side::Left},
      {7, 11, pose::Side::Right},  {11, 12, pose::Side::Right},
      {12, 13, pose::Side::Right},
  }};
  const auto& bs = pose::bones();
  CHECK(bs.size() == expected.size());
  for (std::size_t i = 0; i < expected.size() && i < bs.size(); ++i) {
    CHECK(bs[i].a == expected[i].a);
    CHECK(bs[i].b == expected[i].b);
    CHECK(bs[i].side == expected[i].side);
  }

  int left = 0, right = 0, torso = 0;
  for (const auto& b : bs) {
    if (b.side == pose::Side::Left) ++left;
    if (b.side == pose::Side::Right) ++right;
    if (b.side == pose::Side::Torso) ++torso;
  }
  CHECK(left == right);
  CHECK(left == 6);
  CHECK(torso == 1);
  CHECK(bs[6].side == pose::Side::Torso);
}
