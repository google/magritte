//
// Copyright 2020-2022 Google LLC
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
#include "magritte/calculators/blend_calculator.h"

#include <algorithm>
#include <memory>
#include <vector>

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "absl/status/status.h"
#include  <opencv2/core.hpp>
#include  <opencv2/imgproc.hpp>

namespace magritte {
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;
using ::mediapipe::ImageFrame;
using ::mediapipe::formats::MatView;
namespace {
constexpr char kForegroundFrameTag[] = "FRAMES_FG";
constexpr char kBackgroundFrameTag[] = "FRAMES_BG";
constexpr char kMaskTag[] = "MASK";
constexpr char kOutputFrameTag[] = "FRAMES";
}  // namespace

absl::Status BlendCalculator::GetContract(CalculatorContract* cc) {
  cc->Inputs().Tag(kForegroundFrameTag).Set<ImageFrame>();
  cc->Inputs().Tag(kBackgroundFrameTag).Set<ImageFrame>();
  cc->Inputs().Tag(kMaskTag).Set<ImageFrame>();
  cc->Outputs().Tag(kOutputFrameTag).Set<ImageFrame>();

  // No input side packets.
  return absl::OkStatus();
}

absl::Status BlendCalculator::Open(CalculatorContext* cc) {
  return absl::OkStatus();
}

absl::Status BlendCalculator::Process(CalculatorContext* cc) {
  if (cc->Inputs().Tag(kBackgroundFrameTag).Value().IsEmpty()) {
    LOG(WARNING) << "No background image frame at " << cc->InputTimestamp();
    return absl::OkStatus();
  }
  if (cc->Inputs().Tag(kForegroundFrameTag).Value().IsEmpty()) {
    LOG(WARNING) << "No foreground image frame at " << cc->InputTimestamp();
    return absl::OkStatus();
  }
  if (cc->Inputs().Tag(kMaskTag).Value().IsEmpty()) {
    LOG(INFO) << "No mask frame at " << cc->InputTimestamp();
    cc->Outputs()
        .Tag(kOutputFrameTag)
        .AddPacket(cc->Inputs().Tag(kForegroundFrameTag).Value());
    return absl::OkStatus();
  }

  const auto& frame_bg =
      cc->Inputs().Tag(kBackgroundFrameTag).Get<ImageFrame>();
  const auto& frame_fg =
      cc->Inputs().Tag(kForegroundFrameTag).Get<ImageFrame>();
  const auto& mask = cc->Inputs().Tag(kMaskTag).Get<ImageFrame>();

  std::unique_ptr<ImageFrame> output_frame(
      new ImageFrame(frame_bg.Format(), frame_bg.Width(), frame_bg.Height()));
  output_frame->CopyFrom(frame_bg, ImageFrame::kDefaultAlignmentBoundary);
  RET_CHECK_OK(
      blend(MatView(output_frame.get()), MatView(&frame_fg), MatView(&mask)));
  cc->Outputs()
      .Tag(kOutputFrameTag)
      .Add(output_frame.release(), cc->InputTimestamp());
  return absl::OkStatus();
}

absl::Status BlendCalculator::blend(cv::Mat bg, cv::Mat fg, cv::Mat mask) {
  RET_CHECK(bg.size().width == fg.size().width &&
            bg.size().height == fg.size().height);
  cv::Mat resized_mask(bg.size(), mask.type());
  cv::resize(mask, resized_mask, resized_mask.size(), 0, 0, cv::INTER_NEAREST);
  int from_to[] = {0, 0, 0, 1, 0, 2};
  mixChannels(&resized_mask, 1, &resized_mask, 1, from_to, 3);
  if (resized_mask.type() == CV_32FC3) {
    resized_mask.convertTo(resized_mask, CV_8UC3, 255);
  } else if (resized_mask.type() == CV_32FC4) {
    resized_mask.convertTo(resized_mask, CV_8UC4, 255);
  }
  cv::Mat bg_result;
  cv::Mat fg_result;
  multiply(resized_mask, fg, fg_result, 1.0 / 255);
  multiply(cv::Scalar::all(255) - resized_mask, bg, bg_result, 1.0 / 255);
  // Add the masked foreground and background.
  add(fg_result, bg_result, bg);
  return absl::OkStatus();
}

REGISTER_CALCULATOR(BlendCalculator);
}  // namespace magritte
