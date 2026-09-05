#include "skeleton.hpp"

namespace pose {

const std::array<Bone, 13>& bones() {
  static const std::array<Bone, 13> kBones{{
      {0, 1, Side::Right},   {1, 2, Side::Right},   {2, 3, Side::Right},
      {0, 4, Side::Left},    {4, 5, Side::Left},    {5, 6, Side::Left},
      {0, 7, Side::Torso},
      {7, 8, Side::Left},    {8, 9, Side::Left},    {9, 10, Side::Left},
      {7, 11, Side::Right},  {11, 12, Side::Right}, {12, 13, Side::Right},
  }};
  return kBones;
}

}  // namespace pose
