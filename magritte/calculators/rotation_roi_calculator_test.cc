//
// Copyright 2021-2022 Google LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "magritte/calculators/rotation_roi_calculator.h"

#include "magritte/calculators/rotation_calculator_options.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_runner.h"
#include "mediapipe/framework/formats/rect.pb.h"
#include "mediapipe/framework/port/gmock.h"
#include "mediapipe/framework/port/gtest.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/status_matchers.h"

namespace magritte {
namespace {

constexpr char kRegionOfInterestTag[] = "ROI";

struct RotationRoiTestCase {
  const std::string test_name;
  const RotationMode_Mode rotation_mode;
  const bool clockwise;
  const float expected_rotation;
};

class RotationRoiTest : public testing::TestWithParam<RotationRoiTestCase> {};

TEST_P(RotationRoiTest, TestRotationRoi) {
  const RotationRoiTestCase& test_case = GetParam();

  mediapipe::NormalizedRect expected_rect;
  expected_rect.set_x_center(0.5);
  expected_rect.set_y_center(0.5);
  expected_rect.set_height(1.0);
  expected_rect.set_width(1.0);
  expected_rect.set_rotation(test_case.expected_rotation);

  mediapipe::CalculatorGraphConfig::Node node =
      mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig::Node>(
          std::string(R"pb(
            calculator: "RotationRoiCalculator"
            input_stream: "input_stream"
            output_stream: "ROI:roi"
          )pb"));
  node.mutable_options()
      ->MutableExtension(RotationCalculatorOptions::ext)
      ->set_rotation_mode(test_case.rotation_mode);
  node.mutable_options()
      ->MutableExtension(RotationCalculatorOptions::ext)
      ->set_clockwise(test_case.clockwise);
  mediapipe::CalculatorRunner runner(node);
  runner.MutableInputs()
      ->Get(runner.MutableInputs()->BeginId())
      .packets.push_back(mediapipe::MakePacket<int>(0).At(mediapipe::Timestamp(0)));
  const std::vector<mediapipe::Packet>& output =
      runner.Outputs().Tag(kRegionOfInterestTag).packets;

  MP_ASSERT_OK(runner.Run());
  ASSERT_EQ(output.size(), 1);
  EXPECT_THAT(output[0].Get<mediapipe::NormalizedRect>(),
              mediapipe::EqualsProto(expected_rect));
}

INSTANTIATE_TEST_SUITE_P(
    RotationRoiTests, RotationRoiTest,
    testing::ValuesIn<RotationRoiTestCase>({
        {.test_name = "anti_clockwise_rotation_0",
         .rotation_mode = RotationMode_Mode_ROTATION_0,
         .clockwise = false,
         .expected_rotation = 0.0},

        {.test_name = "anti_clockwise_rotation_90",
         .rotation_mode = RotationMode_Mode_ROTATION_90,
         .clockwise = false,
         .expected_rotation = 3 * M_PI / 2},

        {.test_name = "anti_clockwise_rotation_180",
         .rotation_mode = RotationMode_Mode_ROTATION_180,
         .clockwise = false,
         .expected_rotation = M_PI},

        {.test_name = "anti_clockwise_rotation_270",
         .rotation_mode = RotationMode_Mode_ROTATION_270,
         .clockwise = false,
         .expected_rotation = M_PI / 2},

        {.test_name = "clockwise_rotation_0",
         .rotation_mode = RotationMode_Mode_ROTATION_0,
         .clockwise = true,
         .expected_rotation = 0.0},

        {.test_name = "clockwise_rotation_90",
         .rotation_mode = RotationMode_Mode_ROTATION_90,
         .clockwise = true,
         .expected_rotation = M_PI / 2},

        {.test_name = "clockwise_rotation_180",
         .rotation_mode = RotationMode_Mode_ROTATION_180,
         .clockwise = true,
         .expected_rotation = M_PI},

        {.test_name = "clockwise_rotation_270",
         .rotation_mode = RotationMode_Mode_ROTATION_270,
         .clockwise = true,
         .expected_rotation = 3 * M_PI / 2},
    }),
    [](const testing::TestParamInfo<RotationRoiTest::ParamType>& info) {
      return info.param.test_name;
    }
);

}  // namespace
}  // namespace magritte
