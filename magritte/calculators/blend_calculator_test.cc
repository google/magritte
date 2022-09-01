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

#include "magritte/calculators/blend_calculator.h"

#include <memory>
#include <string>

#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "mediapipe/framework/port/gmock.h"
#include  <opencv2/core.hpp>
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_runner.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/packet.h"
#include "mediapipe/framework/port/gtest.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/status_matchers.h"
#include "mediapipe/framework/tool/test_util.h"

namespace magritte {
namespace {
using ::mediapipe::ImageFrame;

struct BlendTestCase {
  const std::string test_name;
  const std::string foreground;
  const std::string background;
  const std::string mask;
  const std::string expected;
};

class BlendCalculatorTest : public testing::TestWithParam<BlendTestCase> {
 protected:
  // Some named image frames to be used as test images
  std::map<std::string, ImageFrame> frames_by_name;

  void SetUp() override {
    frames_by_name = std::map<std::string, ImageFrame>();
    add_frame_by_name("all_zero", 3, 2,
                      {
                          // clang-format off
                        0, 0, 0,    0, 0, 0,
                        0, 0, 0,    0, 0, 0,
                        0, 0, 0,    0, 0, 0,
                      });  // clang-format on
    add_frame_by_name("all_255", 3, 2,
                      {
                          // clang-format off
                        255, 255, 255,    255, 255, 255,
                        255, 255, 255,    255, 255, 255,
                        255, 255, 255,    255, 255, 255,
        });  // clang-format on
    add_frame_by_name("random1", 3, 2,
                      {
                          // clang-format off
                        163, 7,   163,    134, 27,  31,
                        181, 222, 7,      238, 238, 160,
                        43,  101, 117,    106, 101, 26,
        });  // clang-format on
    add_frame_by_name("random2", 3, 2,
                      {
                          // clang-format off
                        230, 193, 31,     182, 213, 233,
                        63,  15, 6,       141, 89,  176,
                        19,  81, 200,     22,  14,  195,
                      });  // clang-format on
    add_frame_by_name("all_127", 2, 2,
                      {
                          // clang-format off
                        127, 127, 127,    127, 127, 127,
                        127, 127, 127,    127, 127, 127,
                      });  // clang-format on
    add_frame_by_name("some_values1", 2, 2,
                      {
                          // clang-format off
                        10, 20, 30,    40,  50,  60,
                        70, 80, 90,    100, 110, 120,
                      });  // clang-format on
    add_frame_by_name("some_values2", 2, 2,
                      {
                          // clang-format off
                        60, 60, 60,    120, 100, 80,
                        0,  0,  0,     140, 130, 120,
                      });  // clang-format on
    add_frame_by_name("some_values_mean", 2, 2,
                      {
                          // clang-format off
                        35, 40, 45,    80,  75,  70,
                        35, 40, 45,    120, 120, 120,
                      });  // clang-format on
    add_frame_by_name("non_trivial_mask", 2, 2,
                      {
                          // clang-format off
                        63, 63, 63,    127, 127, 127,
                        31, 31, 31,    191, 191, 191,
                      });  // clang-format on
    add_frame_by_name("non_trivial_result", 2, 2,
                      {
                          // clang-format off
                        47, 50, 52,    80,  75,  70,
                        9,  10, 11,    110, 115, 120,
              });  // clang-format on
  }

 private:
  // Adds a new entry to the frames_by_name map that points to an ImageFrame
  // whose underlying image data is copied from the initializer list.
  void add_frame_by_name(std::string name, int width, int height,
                         std::initializer_list<uint8> data) {
    frames_by_name[name] =
        ImageFrame(mediapipe::ImageFormat::SRGB, width, height,
                   /* alignment_boundary= */ 1);
    std::copy(data.begin(), data.end(),
              frames_by_name[name].MutablePixelData());
  }
};

TEST_P(BlendCalculatorTest, TestBlend) {
  const BlendTestCase& test_case = GetParam();
  ImageFrame* foreground = &frames_by_name[test_case.foreground];
  ImageFrame* background = &frames_by_name[test_case.background];
  ImageFrame* mask = &frames_by_name[test_case.mask];
  ImageFrame* expected = &frames_by_name[test_case.expected];

  mediapipe::CalculatorRunner runner(
      mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig::Node>(R"pb(
        calculator: "BlendCalculator"
        input_stream: "FRAMES_BG:frames_bg"
        input_stream: "FRAMES_FG:frames_fg"
        input_stream: "MASK:mask"
        output_stream: "FRAMES:output_video"
      )pb"));
  runner.MutableInputs()
      ->Tag("FRAMES_BG")
      .packets.push_back(
          mediapipe::PointToForeign(background).At(mediapipe::Timestamp(0)));
  runner.MutableInputs()
      ->Tag("FRAMES_FG")
      .packets.push_back(
          mediapipe::PointToForeign(foreground).At(mediapipe::Timestamp(0)));
  runner.MutableInputs()->Tag("MASK").packets.push_back(
      mediapipe::PointToForeign(mask).At(mediapipe::Timestamp(0)));

  MP_ASSERT_OK(runner.Run());

  const std::vector<mediapipe::Packet>& actual_output =
      runner.Outputs().Tag("FRAMES").packets;
  ASSERT_EQ(actual_output.size(), 1);
  EXPECT_EQ(actual_output[0].Timestamp(), mediapipe::Timestamp(0));
  const mediapipe::ImageFrame& output_frame =
      actual_output[0].Get<mediapipe::ImageFrame>();
  EXPECT_EQ(output_frame.Format(), expected->Format());
  auto diff_image = absl::make_unique<mediapipe::ImageFrame>();
  MP_EXPECT_OK(mediapipe::CompareImageFrames(
      *expected, output_frame, /* max_color_diff= */ 0.0,
      /* max_alpha_diff= */ 0.0, /* max_avg_diff= */ 0.0, diff_image))
      << "diff image:\n"
      << mediapipe::formats::MatView(diff_image.get());
}

INSTANTIATE_TEST_SUITE_P(
    BlendCalculatorTests, BlendCalculatorTest,
    testing::ValuesIn<BlendTestCase>({
        {.test_name = "all_zero",
         .foreground = "all_zero",
         .background = "all_zero",
         .mask = "all_zero",
         .expected = "all_zero"},

        {.test_name = "all_255_mask_yields_foreground",
         .foreground = "random1",
         .background = "random2",
         .mask = "all_255",
         .expected = "random1"},

        {.test_name = "all_zero_mask_yields_background",
         .foreground = "random1",
         .background = "random2",
         .mask = "all_zero",
         .expected = "random2"},

        {.test_name = "all_127_mask_yields_mean",
         .foreground = "some_values1",
         .background = "some_values2",
         .mask = "all_127",
         .expected = "some_values_mean"},

        {.test_name = "all_127_mask_yields_mean_reverse",
         .foreground = "some_values2",
         .background = "some_values1",
         .mask = "all_127",
         .expected = "some_values_mean"},
         
        {.test_name = "non_trivial_mask_case",
         .foreground = "some_values1",
         .background = "some_values2",
         .mask = "non_trivial_mask",
         .expected = "non_trivial_result"},
    }),
    [](const testing::TestParamInfo<BlendCalculatorTest::ParamType>& info) {
      return info.param.test_name;
    });

}  // namespace
}  // namespace magritte
