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
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "magritte/calculators/sprite_list.h"
#include "magritte/calculators/sprite_pose.pb.h"
#include "mediapipe/framework/formats/image_frame.h"
#include  <opencv2/imgproc.hpp>

namespace magritte {
namespace {
// Input/output stream tags.
constexpr char kImageFrameTag[] = "IMAGE";
constexpr char kSpritesTag[] = "SPRITES";

using ::mediapipe::CalculatorBase;
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;
using ::mediapipe::formats::MatView;
using ::mediapipe::ImageFrame;
}  // namespace

// Stamps the given textures onto the background image after transforming by the
// given vertex position matrices.
//
// Inputs:
// - IMAGE: The input ImageFrame video frame to be overlaid with the sprites.
//   If it has transparency, it is assumed to be premultiplied.
// - SPRITES: A vector of pairs of sprite images as ImageFrames and vertex
//   transformations as SpritePoses to be stamped onto the input video
//   (see sprite_list.h). The ImageFrame must have a premultiplied alpha
//   channel.
//
// Outputs:
// - IMAGE: The output image with the sprites addded. If the input background
//   image has transparency, then the output will be premultiplied.
//
class SpriteCalculatorCpu : public CalculatorBase {
 public:
  SpriteCalculatorCpu() = default;
  ~SpriteCalculatorCpu() override = default;

  static absl::Status GetContract(CalculatorContract* cc);
  absl::Status Open(CalculatorContext* cc) override;
  absl::Status Process(CalculatorContext* cc) override;

 protected:
  // Draws the provided sprite at the position and orientation specified into
  // the target image.
  absl::Status RenderSingleSprite(const cv::Mat& sprite,
                                  const SpritePose& orientation,
                                  cv::Mat* target);

  // Draws src atop dst using normal alpha blending. src and dst must have the
  // same width and height, but may have different numbers of channels.
  // NOTE: src is assumed to have a premultiplied alpha channel.
  absl::Status ComposeNormal(const cv::Mat& src, cv::Mat& dst);
};

REGISTER_CALCULATOR(SpriteCalculatorCpu);

// static
absl::Status SpriteCalculatorCpu::GetContract(CalculatorContract* cc) {
  RET_CHECK(cc != nullptr) << "CalculatorContract is nullptr";
  RET_CHECK(cc->Inputs().HasTag(kImageFrameTag))
      << "Missing input " << kImageFrameTag << " tag.";
  cc->Inputs().Tag(kImageFrameTag).Set<ImageFrame>();

  RET_CHECK(cc->Inputs().HasTag(kSpritesTag))
      << "Missing input " << kSpritesTag << " tag.";
  cc->Inputs().Tag(kSpritesTag).Set<SpriteList>();

  RET_CHECK(cc->Outputs().HasTag(kImageFrameTag))
      << "Missing output " << kImageFrameTag << " tag.";
  cc->Outputs().Tag(kImageFrameTag).Set<ImageFrame>();

  return absl::OkStatus();
}

absl::Status SpriteCalculatorCpu::Open(CalculatorContext* cc) {
  return absl::OkStatus();
}

absl::Status SpriteCalculatorCpu::ComposeNormal(const cv::Mat& src,
                                                cv::Mat& dst) {
  RET_CHECK_EQ(src.rows, dst.rows)
      << "src rows: " << src.rows << " does not equal dst rows: " << dst.rows;
  RET_CHECK_EQ(src.cols, dst.cols)
      << "src cols: " << src.cols << " does not equal dst cols: " << dst.cols;
  RET_CHECK_EQ(src.type(), CV_8UC4) << "src should have an alpha channel.";
  RET_CHECK(dst.type() == CV_8UC3 || dst.type() == CV_8UC4)
      << "dst should be an RGB or RGBA Mat.";

  // Create a Mat with as many copies of the src alpha as the channels in dst.
  cv::Mat alpha_copies(src.rows, src.cols, dst.type());
  static int from_to[] = {3, 0, 3, 1, 3, 2, 3, 3};
  cv::mixChannels(&src, /*nsrcs=*/1, &alpha_copies, /*ndsts=*/1, from_to,
                  /*npairs=*/dst.channels());

  // Make sure we have a copy of src with a number of channels matching dst.
  cv::Mat src_matching_channels;
  if (dst.type() == CV_8UC3) {
    cv::cvtColor(src, src_matching_channels, cv::COLOR_RGBA2RGB);
  } else if (dst.type() == CV_8UC4) {
    src_matching_channels = src;
  }

  cv::multiply(cv::Scalar::all(255) - alpha_copies, dst, dst, 1 / 255.0);
  cv::add(dst, src_matching_channels, dst);

  return absl::OkStatus();
}

absl::Status SpriteCalculatorCpu::RenderSingleSprite(
    const cv::Mat& sprite, const SpritePose& orientation, cv::Mat* target) {
  const int sprite_width = sprite.cols;
  const int sprite_height = sprite.rows;
  const float rotation_in_degrees_counterclockwise =
      - orientation.rotation_radians() * 180.0f / M_PI;
  const float center_x = orientation.position_x() * target->cols;
  const float center_y = orientation.position_y() * target->rows;
  const float scale = orientation.scale();

  const cv::RotatedRect rotated_sprite_bounds(
      cv::Point2f(0.5f * (sprite_width - 1), 0.5f * (sprite_height - 1)),
      cv::Size2f(scale * sprite_width, scale * sprite_height),
      rotation_in_degrees_counterclockwise);

  cv::Rect target_roi_rect = rotated_sprite_bounds.boundingRect();
  cv::Size warped_sprite_size(target_roi_rect.width, target_roi_rect.height);

  cv::Mat transform_mat = cv::getRotationMatrix2D(
      cv::Point2f(0.5f * (sprite_width - 1), 0.5f * (sprite_height - 1)),
      rotation_in_degrees_counterclockwise, scale);

  // The rotation from getRotationMatrix2D doesn't offset to keep everything in
  // the positive quadrant, so we need to bake it in manually here to avoid
  // clipping during the warp.
  transform_mat.at<double>(0, 2) -= target_roi_rect.x;
  transform_mat.at<double>(1, 2) -= target_roi_rect.y;

  // Translate the rotated rect to put it where the sprite is going to be drawn.
  target_roi_rect.x = std::round(-target_roi_rect.width * 0.5f + center_x);
  target_roi_rect.y = std::round(-target_roi_rect.height * 0.5f + center_y);

  // Figure out how much (if any) of the sprite is supposed to be rendered
  // outside the bounds of the target image.
  const int needed_left_margin = std::max(-target_roi_rect.tl().x, 0);
  const int needed_right_margin =
      std::max(target_roi_rect.br().x - target->cols, 0);
  const int needed_top_margin = std::max(-target_roi_rect.tl().y, 0);
  const int needed_bottom_margin =
      std::max(target_roi_rect.br().y - target->rows, 0);

  // Adjust the position of the target ROI to be the intersection of the
  // computed target ROI and the whole target image.
  target_roi_rect.x += needed_left_margin;
  target_roi_rect.width -= needed_left_margin + needed_right_margin;
  target_roi_rect.y += needed_top_margin;
  target_roi_rect.height -= needed_top_margin + needed_bottom_margin;

  // If the intersection is empty, there is nothing to draw.
  if (target_roi_rect.width <= 0 || target_roi_rect.height <= 0) {
    return absl::OkStatus();
  }

  cv::Mat target_roi = (*target)(target_roi_rect);

  // Trim the warped sprite rect to match the dimensions of the target ROI.
  cv::Rect warped_sprite_rect(0, 0, warped_sprite_size.width,
                              warped_sprite_size.height);
  warped_sprite_rect.x += needed_left_margin;
  warped_sprite_rect.width -= needed_left_margin + needed_right_margin;
  warped_sprite_rect.y += needed_top_margin;
  warped_sprite_rect.height -= needed_top_margin + needed_bottom_margin;

  // Translate the transform so that we can evaluate only the pixels that show
  // up in the target ROI.
  transform_mat.at<double>(0, 2) -= warped_sprite_rect.x;
  transform_mat.at<double>(1, 2) -= warped_sprite_rect.y;

  // Perform the actual warp.
  cv::Mat warped_sprite(warped_sprite_rect.height, warped_sprite_rect.width,
                        sprite.type());
  cv::warpAffine(sprite, warped_sprite, transform_mat, warped_sprite.size());

  MP_RETURN_IF_ERROR(ComposeNormal(warped_sprite, target_roi));
  return absl::OkStatus();
}

absl::Status SpriteCalculatorCpu::Process(CalculatorContext* cc) {
  const ImageFrame& input_frame =
      cc->Inputs().Tag(kImageFrameTag).Get<ImageFrame>();
  std::unique_ptr<ImageFrame> output_frame(new ImageFrame(
      input_frame.Format(), input_frame.Width(), input_frame.Height()));

  cv::Mat output_mat = MatView(output_frame.get());
  MatView(&input_frame).copyTo(output_mat);

  // Render all the sprites.
  const auto& all_sprites = cc->Inputs().Tag(kSpritesTag).Get<SpriteList>();
  for (const auto& sprite : all_sprites) {
    MP_RETURN_IF_ERROR(sprite.image_packet.ValidateAsType<ImageFrame>());
    const auto& sprite_frame = sprite.image_packet.Get<ImageFrame>();
    const auto& pose = sprite.pose;
    MP_RETURN_IF_ERROR(
        RenderSingleSprite(MatView(&sprite_frame), pose, &output_mat));
  }

  cc->Outputs()
      .Tag(kImageFrameTag)
      .Add(output_frame.release(), cc->InputTimestamp());
  return absl::OkStatus();
}
}  // namespace magritte
