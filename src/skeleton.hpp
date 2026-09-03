#pragma once
#include <array>

namespace pose {

enum class Side { Left, Right, Torso };

struct Bone {
  int a;
  int b;
  Side side;
};

// Joint indices, from joint-names.txt:
// 0 Hip, 1 RHip, 2 RKnee, 3 RAnkle, 4 LHip, 5 LKnee, 6 LAnkle, 7 Neck,
// 8 LUpperArm, 9 LElbow, 10 LWrist, 11 RUpperArm, 12 RElbow, 13 RWrist
const std::array<Bone, 13>& bones();

}  // namespace pose
