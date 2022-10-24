//
// Copyright 2016-2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <memory>
#include <vector>

#include "mediapipe/framework/formats/image_frame_opencv.h"
#include  <opencv2/imgcodecs.hpp>
#include  <opencv2/imgproc.hpp>
#include "absl/memory/memory.h"
#include "magritte/calculators/sprite_list.h"
#include "magritte/calculators/sprite_pose.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_runner.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/port/gmock.h"
#include "mediapipe/framework/port/gtest.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/status_matchers.h"
#include "mediapipe/framework/tool/test_util.h"

namespace magritte {
namespace {

using ::mediapipe::CalculatorGraphConfig;
using ::mediapipe::CalculatorRunner;
using ::mediapipe::ImageFormat;
using ::mediapipe::ImageFrame;
using ::mediapipe::Packet;
using ::mediapipe::Timestamp;
using ::mediapipe::formats::MatView;

constexpr char kImageFrameTag[] = "IMAGE";
constexpr char kSpritesTag[] = "SPRITES";

constexpr char kSpriteBackgroundPath[] =
    "magritte/test_data/sprite_background.png";
constexpr char kSpriteCompositedPath[] =
    "magritte/test_data/sprite_composited.png";
constexpr char kSpritePremultipliedPath[] =
    "magritte/test_data/sprite_premultiplied.png";

constexpr char kCalculatorGraphProto[] = R"pb(
  calculator: "SpriteCalculatorCpu"
  input_stream: "IMAGE:background_video"
  input_stream: "SPRITES:sprites"
  output_stream: "IMAGE:composited_result"
)pb";

std::unique_ptr<ImageFrame> LoadRgbaPng(std::string filename) {
  cv::Mat mat = cv::imread(filename, cv::IMREAD_UNCHANGED);
  cv::cvtColor(mat, mat, cv::COLOR_BGRA2RGBA);
  auto image_frame =
      std::make_unique<ImageFrame>(ImageFormat::SRGBA, mat.cols, mat.rows);
  mat.copyTo(MatView(image_frame.get()));
  return image_frame;
}

// Sets up the input and output streams, runs the calculator on them, and
// returns a Packet containing the corresponding ImageFrame.
Packet RunCalculatorWithInput(std::unique_ptr<ImageFrame> background,
                              std::unique_ptr<ImageFrame> sprite,
                              const std::vector<SpritePose>& poses) {
  Packet background_packet =
      mediapipe::Adopt(background.release()).At(Timestamp(0));
  Packet sprite_frame_packet =
      mediapipe::Adopt(sprite.release()).At(Timestamp(0));

  auto sprite_list = std::make_unique<SpriteList>();
  for (const auto& pose : poses) {
    sprite_list->push_back(SpriteListElement(sprite_frame_packet, pose));
  }
  const Packet sprite_list_packet =
      mediapipe::Adopt(sprite_list.release()).At(Timestamp(0));

  CalculatorGraphConfig::Node config_node =
      mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig::Node>(
          kCalculatorGraphProto);

  CalculatorRunner runner(config_node);
  runner.MutableInputs()
      ->Tag(kImageFrameTag)
      .packets.push_back(background_packet);
  runner.MutableInputs()
      ->Tag(kSpritesTag)
      .packets.push_back(sprite_list_packet);

  MP_EXPECT_OK(runner.Run());

  const std::vector<Packet>& output =
      runner.Outputs().Tag(kImageFrameTag).packets;

  EXPECT_EQ(output.size(), 1);
  return output[0];
}

// Tests stamping a pattern of sprites with alpha over a background with scaling
// and 90 degree rotations and compares it to a golden result.
TEST(SpriteCpuCalculatorTest, FourSpriteStamp) {
  std::unique_ptr<ImageFrame> background_frame =
      LoadRgbaPng(kSpriteBackgroundPath);
  std::unique_ptr<ImageFrame> sprite_frame =
      LoadRgbaPng(kSpritePremultipliedPath);
  std::unique_ptr<ImageFrame> expected_result_frame =
      LoadRgbaPng(kSpriteCompositedPath);

  std::vector<SpritePose> poses;
  SpritePose pose;
  pose.set_scale(2.0f);
  // Top left sprite.
  pose.set_position_x(0.25f);
  pose.set_position_y(0.25f);
  poses.push_back(pose);
  // Top right sprite.
  pose.set_position_x(0.75f);
  pose.set_position_y(0.25f);
  pose.set_rotation_radians(3 * M_PI / 2);
  poses.push_back(pose);
  // Bottom right sprite.
  pose.set_position_x(0.75f);
  pose.set_position_y(0.75f);
  pose.set_rotation_radians(M_PI);
  poses.push_back(pose);
  // Bottom left sprite.
  pose.set_position_x(0.25f);
  pose.set_position_y(0.75f);
  pose.set_rotation_radians(M_PI / 2);
  poses.push_back(pose);

  Packet result_packet = RunCalculatorWithInput(std::move(background_frame),
                                                std::move(sprite_frame), poses);
  auto& result_frame = result_packet.Get<ImageFrame>();

  std::string comparison_error;
  EXPECT_TRUE(mediapipe::CompareImageFrames(
      result_frame, *expected_result_frame, /*max_color_diff=*/0.0,
      /*max_alpha_diff=*/0.0, /*max_avg_diff=*/0.0, &comparison_error))
      << comparison_error;
}

// Tests stamping one big sprite to cover the background. This is testing that
// off screen pixels don't break anything.
TEST(SpriteCpuCalculatorTest, OneReallyBigStamp) {
  std::unique_ptr<ImageFrame> background_frame =
      LoadRgbaPng(kSpriteBackgroundPath);

  const cv::Scalar green_with_alpha(/*red=*/0.f, /*green=*/255.f, /*blue=*/0.f,
                                    /*alpha=*/255.f);

  auto sprite_frame = std::make_unique<ImageFrame>(ImageFormat::SRGBA,
                                                   /*width=*/64, /*height=*/64);
  MatView(sprite_frame.get()).setTo(green_with_alpha);

  auto expected_result_frame = std::make_unique<ImageFrame>(
      ImageFormat::SRGBA, /*width=*/64, /*height=*/64);
  MatView(expected_result_frame.get()).setTo(green_with_alpha);

  std::vector<SpritePose> poses;
  SpritePose pose;
  pose.set_scale(2.0f);
  // One big sprite.
  pose.set_position_x(0.5f);
  pose.set_position_y(0.5f);
  poses.push_back(pose);

  Packet result_packet = RunCalculatorWithInput(std::move(background_frame),
                                                std::move(sprite_frame), poses);
  auto& result_frame = result_packet.Get<ImageFrame>();

  std::string comparison_error;
  EXPECT_TRUE(mediapipe::CompareImageFrames(
      result_frame, *expected_result_frame, /*max_color_diff=*/0.0,
      /*max_alpha_diff=*/0.0, /*max_avg_diff=*/0.0, &comparison_error))
      << comparison_error;
}

}  // namespace
}  // namespace magritte
