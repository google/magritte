//
// Copyright 2022 Google LLC
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
#include "magritte/calculators/rois_to_sprite_list_calculator.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "mediapipe/framework/formats/image_frame_opencv.h"
#include  <opencv2/imgcodecs.hpp>
#include  <opencv2/imgproc.hpp>
#include "absl/memory/memory.h"
#include "magritte/calculators/rois_to_sprite_list_calculator.pb.h"
#include "magritte/calculators/sprite_list.h"
#include "magritte/calculators/sprite_pose.pb.h"
#include "magritte/calculators/sprite_pose.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/calculator_runner.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/rect.pb.h"
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
using ::mediapipe::NormalizedRect;
using SizeFloat = std::pair<float, float>;
using SizeInt = std::pair<int, int>;

constexpr char kImageSizeTag[] = "SIZE";
constexpr char kNormalizedRectsTag[] = "NORM_RECTS";
constexpr char kStickerImageTag[] = "STICKER_IMAGE_CPU";
constexpr char kStickerZoomTag[] = "STICKER_ZOOM";
constexpr char kSpriteListTag[] = "SPRITES";

constexpr char kSpritePremultipliedPath[] =
    "magritte/test_data/sprite_premultiplied.png";
constexpr char kSpriteTransparentPath[] =
    "magritte/test_data/sprite_transparent.png";

constexpr char kCalculatorGraphProto[] = R"pb(
  calculator: "RoisToSpriteListCalculator"
  input_stream: "SIZE:image_size"
  input_stream: "NORM_RECTS:rois"
  input_side_packet: "STICKER_IMAGE_CPU:sticker_image"
  input_side_packet: "STICKER_ZOOM:sticker_zoom"
  output_stream: "SPRITES:sprites"
)pb";

std::unique_ptr<ImageFrame> LoadRgbaPng(std::string filename) {
  cv::Mat mat = cv::imread(filename, cv::IMREAD_UNCHANGED);
  cv::cvtColor(mat, mat, cv::COLOR_BGRA2RGBA);
  auto image_frame =
      std::make_unique<ImageFrame>(ImageFormat::SRGBA, mat.cols, mat.rows);
  mat.copyTo(MatView(image_frame.get()));
  return image_frame;
}

// Test PremultiplyAlphaCpu with golden result.
TEST(PremultiplyCpuTest, TestPremultiply) {
  std::unique_ptr<ImageFrame> transparent_frame =
      LoadRgbaPng(kSpriteTransparentPath);
  cv::Mat transparent_frame_mat = MatView(transparent_frame.get());

  MP_EXPECT_OK(
      RoisToSpriteListCalculator::PremultiplyAlphaCpu(transparent_frame_mat));

  std::unique_ptr<ImageFrame> premultiplied_frame =
      LoadRgbaPng(kSpritePremultipliedPath);

  std::string comparison_error;
  EXPECT_TRUE(mediapipe::CompareImageFrames(
      *transparent_frame, *premultiplied_frame, /*max_color_diff=*/0.0,
      /*max_alpha_diff=*/0.0, /*max_avg_diff=*/0.0, &comparison_error))
      << comparison_error;
}

struct FindFitZoomTestCase {
  const std::string test_name;
  const SizeInt bg_size;
  const SizeInt sticker_size;
  const SizeFloat roi_normalized_size;
  const float expected_fit_zoom;
};

class FindFitZoomTest : public testing::TestWithParam<FindFitZoomTestCase> {};

TEST_P(FindFitZoomTest, TestFindFitZoom) {
  const FindFitZoomTestCase& test_case = GetParam();

  NormalizedRect roi;
  roi.set_width(test_case.roi_normalized_size.first);
  roi.set_height(test_case.roi_normalized_size.second);

  const float fit_zoom = RoisToSpriteListCalculator::FindFitZoom(
      test_case.bg_size, test_case.sticker_size, roi);
  EXPECT_EQ(fit_zoom, test_case.expected_fit_zoom);
}

INSTANTIATE_TEST_SUITE_P(
    FindFitZoomTests, FindFitZoomTest,
    testing::ValuesIn<FindFitZoomTestCase>({
        {.test_name = "same_size_full_image",
         .bg_size = SizeInt(64, 64),
         .sticker_size = SizeInt(64, 64),
         .roi_normalized_size = SizeFloat(1.0f, 1.0f),
         .expected_fit_zoom = 1.0f},

        {.test_name = "same_size_half_image",
         .bg_size = SizeInt(64, 64),
         .sticker_size = SizeInt(32, 32),
         .roi_normalized_size = SizeFloat(0.5f, 0.5f),
         .expected_fit_zoom = 1.0f},

        {.test_name = "square_double_zoom",
         .bg_size = SizeInt(64, 64),
         .sticker_size = SizeInt(32, 32),
         .roi_normalized_size = SizeFloat(1.0f, 1.0f),
         .expected_fit_zoom = 2.0f},

        {.test_name = "square_half_zoom",
         .bg_size = SizeInt(64, 64),
         .sticker_size = SizeInt(64, 64),
         .roi_normalized_size = SizeFloat(0.5f, 0.5f),
         .expected_fit_zoom = 0.5f},

        {.test_name = "square_sprite_with_horizontal_rect_roi_requires_"
                      "scaling_for_horizontal_coverage",
         .bg_size = SizeInt(64, 64),
         .sticker_size = SizeInt(16, 16),
         .roi_normalized_size = SizeFloat(0.5f, 0.25f),
         .expected_fit_zoom = 2.0f},

        {.test_name = "square_sprite_with_vertical_rect_roi_requires_"
                      "scaling_for_vertical_coverage",
         .bg_size = SizeInt(64, 64),
         .sticker_size = SizeInt(16, 16),
         .roi_normalized_size = SizeFloat(0.25f, 0.5f),
         .expected_fit_zoom = 2.0f},

        {.test_name = "square_sprite_with_vertical_rect_roi_and_"
                      "horizontal_rect_background",
         .bg_size = SizeInt(64, 32),
         .sticker_size = SizeInt(8, 16),
         .roi_normalized_size = SizeFloat(0.125f, 0.5f),
         .expected_fit_zoom = 1.0f},

        {.test_name = "square_sprite_with_horizontal_rect_roi_and_"
                      "vertical_rect_background",
         .bg_size = SizeInt(32, 64),
         .sticker_size = SizeInt(16, 8),
         .roi_normalized_size = SizeFloat(0.5f, 0.125f),
         .expected_fit_zoom = 1.0f},
    }),
    [](const testing::TestParamInfo<FindFitZoomTest::ParamType>& info) {
      return info.param.test_name;
    });

// Sets up the input and output streams, runs the calculator on them, and
// returns a Packet containing the corresponding ImageFrame.
Packet RunCalculatorWithSticker(std::unique_ptr<ImageFrame> sticker,
                                bool sticker_is_premultiplied) {
  auto image_size = std::make_unique<SizeInt>(64, 64);
  Packet image_size_packet =
      mediapipe::Adopt(image_size.release()).At(Timestamp(0));

  auto rois = std::make_unique<std::vector<NormalizedRect>>();
  // Using a blank ROI so that at least one sprite is returned.
  rois->push_back(NormalizedRect());
  Packet rois_packet = mediapipe::Adopt(rois.release()).At(Timestamp(0));

  CalculatorGraphConfig::Node config_node =
      mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig::Node>(
          kCalculatorGraphProto);
  config_node.mutable_options()
      ->MutableExtension(RoisToSpriteListCalculatorOptions::ext)
      ->set_sticker_is_premultiplied(sticker_is_premultiplied);

  CalculatorRunner runner(config_node);
  runner.MutableInputs()
      ->Tag(kImageSizeTag)
      .packets.push_back(image_size_packet);
  runner.MutableInputs()
      ->Tag(kNormalizedRectsTag)
      .packets.push_back(rois_packet);
  runner.MutableSidePackets()->Tag(kStickerImageTag) =
      mediapipe::Adopt(sticker.release());
  runner.MutableSidePackets()->Tag(kStickerZoomTag) =
      mediapipe::Adopt(new float(1.0f));

  MP_EXPECT_OK(runner.Run());

  const std::vector<Packet>& output =
      runner.Outputs().Tag(kSpriteListTag).packets;

  EXPECT_EQ(output.size(), 1);
  auto& result_sprite_list = output[0].Get<SpriteList>();
  EXPECT_EQ(result_sprite_list.size(), 1);
  return result_sprite_list[0].image_packet;
}

// Tests calculator with a sprite that needs to be premultiplied.
TEST(RoisToSpriteListCalculatorCpuTest, PremultiplyStickerTest) {
  std::unique_ptr<ImageFrame> transparent_frame =
      LoadRgbaPng(kSpriteTransparentPath);
  std::unique_ptr<ImageFrame> premultiplied_frame =
      LoadRgbaPng(kSpritePremultipliedPath);

  Packet result_packet =
      RunCalculatorWithSticker(std::move(transparent_frame),
                               /*sticker_is_premultiplied=*/false);
  auto& result_frame = result_packet.Get<ImageFrame>();

  std::string comparison_error;
  EXPECT_TRUE(mediapipe::CompareImageFrames(
      result_frame, *premultiplied_frame, /*max_color_diff=*/0.0,
      /*max_alpha_diff=*/0.0, /*max_avg_diff=*/0.0, &comparison_error))
      << comparison_error;
}

// Tests calculator with a sprite that *does not* need to be premultiplied.
TEST(RoisToSpriteListCalculatorCpuTest, NoPremultiplyStickerTest) {
  std::unique_ptr<ImageFrame> input_frame =
      LoadRgbaPng(kSpritePremultipliedPath);
  std::unique_ptr<ImageFrame> output_frame =
      LoadRgbaPng(kSpritePremultipliedPath);

  Packet result_packet =
      RunCalculatorWithSticker(std::move(input_frame),
                               /*sticker_is_premultiplied=*/true);
  auto& result_frame = result_packet.Get<ImageFrame>();

  std::string comparison_error;
  EXPECT_TRUE(mediapipe::CompareImageFrames(
      result_frame, *output_frame, /*max_color_diff=*/0.0,
      /*max_alpha_diff=*/0.0, /*max_avg_diff=*/0.0, &comparison_error))
      << comparison_error;
}

// Tests calculator with a sprite without transparency; should add an alpha
// channel.
TEST(RoisToSpriteListCalculatorCpuTest, NoAlphaStickerTest) {
  const cv::Scalar green_no_alpha(/*red=*/0.f, /*green=*/255.f, /*blue=*/0.f);
  const cv::Scalar green_with_alpha(/*red=*/0.f, /*green=*/255.f, /*blue=*/0.f,
                                    /*alpha=*/255.f);

  auto input_sticker = std::make_unique<ImageFrame>(
      ImageFormat::SRGB, /*width=*/16, /*height=*/16);
  MatView(input_sticker.get()).setTo(green_no_alpha);

  auto expected_sticker = std::make_unique<ImageFrame>(
      ImageFormat::SRGBA, /*width=*/16, /*height=*/16);
  MatView(expected_sticker.get()).setTo(green_with_alpha);

  Packet result_packet =
      RunCalculatorWithSticker(std::move(input_sticker),
                               /*sticker_is_premultiplied=*/false);
  auto& result_frame = result_packet.Get<ImageFrame>();

  std::string comparison_error;
  EXPECT_TRUE(mediapipe::CompareImageFrames(
      result_frame, *expected_sticker, /*max_color_diff=*/0.0,
      /*max_alpha_diff=*/0.0, /*max_avg_diff=*/0.0, &comparison_error))
      << comparison_error;
}

// Tests whether SpritePose is built correctly.
TEST(RoisToSpriteListCalculatorCpuTest, SpritePoseTest) {
  auto image_size = std::make_unique<SizeInt>(64, 64);
  Packet image_size_packet =
      mediapipe::Adopt(image_size.release()).At(Timestamp(0));

  auto sticker = std::make_unique<ImageFrame>(ImageFormat::SRGB, /*width=*/64,
                                              /*height=*/64);

  auto rois = std::make_unique<std::vector<NormalizedRect>>();
  rois->push_back(mediapipe::ParseTextProtoOrDie<NormalizedRect>(std::string(R"pb(
    width: 1.0f
    height: 1.0f
    x_center: 0.2f
    y_center: 0.3f
    rotation: 0.4f
  )pb")));
  Packet rois_packet = mediapipe::Adopt(rois.release()).At(Timestamp(0));

  CalculatorGraphConfig::Node config_node =
      mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig::Node>(
          kCalculatorGraphProto);

  CalculatorRunner runner(config_node);
  runner.MutableInputs()
      ->Tag(kImageSizeTag)
      .packets.push_back(image_size_packet);
  runner.MutableInputs()
      ->Tag(kNormalizedRectsTag)
      .packets.push_back(rois_packet);
  runner.MutableSidePackets()->Tag(kStickerImageTag) =
      mediapipe::Adopt(sticker.release());
  runner.MutableSidePackets()->Tag(kStickerZoomTag) =
      mediapipe::Adopt(new float(2.0f));

  MP_EXPECT_OK(runner.Run());

  const std::vector<Packet>& output =
      runner.Outputs().Tag(kSpriteListTag).packets;

  EXPECT_EQ(output.size(), 1);
  auto& result_sprite_list = output[0].Get<SpriteList>();
  EXPECT_EQ(result_sprite_list.size(), 1);

  EXPECT_THAT(result_sprite_list[0].pose,
              mediapipe::EqualsProto(
                  mediapipe::ParseTextProtoOrDie<SpritePose>(std::string(R"pb(
                    position_x: 0.2f
                    position_y: 0.3f
                    rotation_radians: 0.4f
                    scale: 2.0f
                  )pb"))));
}

}  // namespace
}  // namespace magritte
