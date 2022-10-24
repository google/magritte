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
#include "magritte/calculators/new_canvas_calculator.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "mediapipe/util/color.pb.h"
#include "absl/memory/memory.h"
#include "magritte/calculators/new_canvas_calculator.pb.h"
#include  <opencv2/core.hpp>

#if !defined(MEDIAPIPE_DISABLE_GPU)
#include "mediapipe/gpu/gl_calculator_helper.h"
#include "mediapipe/gpu/gl_simple_shaders.h"
#include "mediapipe/gpu/shader_util.h"
#endif  //  !MEDIAPIPE_DISABLE_GPU

namespace magritte {
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;
using ::mediapipe::ImageFrame;
#if !defined(MEDIAPIPE_DISABLE_GPU)
using ::mediapipe::GlCalculatorHelper;
using ::mediapipe::GpuBuffer;
#endif  //  !MEDIAPIPE_DISABLE_GPU
using ::mediapipe::Color;
using ::mediapipe::formats::MatView;
namespace {
constexpr char kGpuBufferTag[] = "IMAGE_GPU";
constexpr char kImageFrameTag[] = "IMAGE";
}  // namespace

absl::Status NewCanvasCalculator::GetContract(CalculatorContract* cc) {
  RET_CHECK(cc != nullptr) << "CalculatorContract is nullptr";
  RET_CHECK(cc->Inputs().HasTag(kImageFrameTag) ^
            cc->Inputs().HasTag(kGpuBufferTag))
      << "Calculator can have one and only one input";
  RET_CHECK(cc->Outputs().HasTag(kImageFrameTag) ^
            cc->Outputs().HasTag(kGpuBufferTag))
      << "Calculator can have one and only one output";

  if (cc->Inputs().HasTag(kImageFrameTag)) {
    cc->Inputs().Tag(kImageFrameTag).Set<ImageFrame>();
    RET_CHECK(cc->Outputs().HasTag(kImageFrameTag))
        << "Input and output format must be identical";
    cc->Outputs().Tag(kImageFrameTag).Set<ImageFrame>();
  }

#if !defined(MEDIAPIPE_DISABLE_GPU)
  if (cc->Inputs().HasTag(kGpuBufferTag)) {
    cc->Inputs().Tag(kGpuBufferTag).Set<GpuBuffer>();
    RET_CHECK(cc->Outputs().HasTag(kGpuBufferTag))
        << "Input and output format must be identical";
    cc->Outputs().Tag(kGpuBufferTag).Set<GpuBuffer>();
  }

  MP_RETURN_IF_ERROR(mediapipe::GlCalculatorHelper::UpdateContract(cc));
#endif  //  !MEDIAPIPE_DISABLE_GPU

  // No input side packets.
  return absl::OkStatus();
}

absl::Status NewCanvasCalculator::Open(CalculatorContext* cc) {
#if !defined(MEDIAPIPE_DISABLE_GPU)
  return helper_.Open(cc);
#endif  //  !MEDIAPIPE_DISABLE_GPU
  return absl::OkStatus();
}

absl::Status NewCanvasCalculator::Process(CalculatorContext* cc) {
  RET_CHECK(cc != nullptr) << "CalculatorContext is nullptr";
  bool use_gpu = cc->Inputs().HasTag(kGpuBufferTag);
  if (use_gpu) {
    return ProcessGpu(cc);
  } else {
    return ProcessCpu(cc);
  }
}

absl::Status NewCanvasCalculator::ProcessCpu(CalculatorContext* cc) {
  const NewCanvasCalculatorOptions& options =
      cc->Options<NewCanvasCalculatorOptions>();

  if (cc->Inputs().Tag(kImageFrameTag).Value().IsEmpty()) {
    LOG(WARNING) << "No image frame at " << cc->InputTimestamp();
    return absl::OkStatus();
  }

  const auto& frame = cc->Inputs().Tag(kImageFrameTag).Get<ImageFrame>();
  std::pair<int, int> size =
      GetSizeFromOptions(options, frame.Width(), frame.Height());
  auto output_frame =
      std::make_unique<ImageFrame>(frame.Format(), size.first, size.second);

  cv::Mat src = MatView(output_frame.get());
  const mediapipe::Color color = options.color();
  src.setTo(cv::Scalar(color.r(), color.g(), color.b()));

  cc->Outputs()
      .Tag(kImageFrameTag)
      .Add(output_frame.release(), cc->InputTimestamp());
  return absl::OkStatus();
}

absl::Status NewCanvasCalculator::ProcessGpu(CalculatorContext* cc) {
  if (cc->Inputs().Tag(kGpuBufferTag).Value().IsEmpty()) {
    LOG(WARNING) << "No image frame at " << cc->InputTimestamp();
    return absl::OkStatus();
  }
#if !defined(MEDIAPIPE_DISABLE_GPU)
  const auto& input = cc->Inputs().Tag(kGpuBufferTag).Get<GpuBuffer>();
  helper_.RunInGlContext([this, &input, &cc] {
    const auto& options = cc->Options<NewCanvasCalculatorOptions>();

    std::pair<int, int> size =
        GetSizeFromOptions(options, input.width(), input.height());
    mediapipe::GlTexture src = helper_.CreateDestinationTexture(
        size.first, size.second, input.format());

    helper_.BindFramebuffer(src);
    mediapipe::Color color = options.has_color() ? options.color() : Color();
    glClearColor(color.r() / 255.0f, color.g() / 255.0f, color.b() / 255.0f,
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    std::unique_ptr<mediapipe::GpuBuffer> output = src.GetFrame<GpuBuffer>();
    glFlush();
    cc->Outputs()
        .Tag(kGpuBufferTag)
        .Add(output.release(), cc->InputTimestamp());
    src.Release();
  });
#endif  //  !MEDIAPIPE_DISABLE_GPU
  return absl::OkStatus();
}

std::pair<int, int> NewCanvasCalculator::GetSizeFromOptions(
    const NewCanvasCalculatorOptions& options, const int original_width,
    const int original_height) {
  if (options.has_scale_factor()) {
    return {options.scale_factor() * original_width,
            options.scale_factor() * original_height};
  }
  if (options.has_target_width() && options.has_target_height()) {
    return {options.target_width(), options.target_height()};
  }
  if (options.has_target_width()) {
    return {options.target_width(),
            original_height * options.target_width() / original_width};
  }
  if (options.has_target_height()) {
    return {original_width * options.target_height() / original_height,
            options.target_height()};
  }
  return {original_width, original_height};
}

REGISTER_CALCULATOR(NewCanvasCalculator);

}  // namespace magritte
