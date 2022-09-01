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
#ifndef MAGRITTE_CALCULATORS_ROIS_TO_SPRITE_LIST_CALCULATOR_H_
#define MAGRITTE_CALCULATORS_ROIS_TO_SPRITE_LIST_CALCULATOR_H_

#include <algorithm>

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "mediapipe/framework/formats/rect.pb.h"

#if !defined(MEDIAPIPE_DISABLE_GPU)
#include "mediapipe/gpu/gl_calculator_helper.h"
#endif  // !MEDIAPIPE_DISABLE_GPU

namespace magritte {

// A calculator that, given a list of regions of interest (ROIs) and a sticker
// image, generates a SpriteList, to be used in SpriteCalculator{Cpu|Gpu}.
//
// A SpritePose is generated for each ROI, with the corresponding center and
// rotation. The sticker is then zoomed so to cover the entire ROI while
// preserving aspect ratio. An extra default zoom given by the STICKER_ZOOM may
// be applied to ensure that, e.g. stickers with transaparency indeed redact
// the ROI.
//
// Inputs:
// - SIZE: The backgroud image size as a std::pair<int, int>.
// - NORM_RECTS: The ROIs as a std::vector<NormalizedReect>.
//
// Input side packets:
// - STICKER_IMAGE_CPU or STICKER_IMAGE_GPU: The sticker image as an ImageFrame
//   or as a GpuBuffer.
// - STICKER_ZOOM: The sticker default zoom as a float.
//
// Outputs:
// - SPRITES: the corresponding SpriteList.
//
// Options:
// - sticker_is_premultiplied: If the sticker has transparency, whether it is
//   premultiplied or straight. Default is false.
//
// Example config:
// node {
//   calculator: "RoisToSpriteListCalculator"
//   input_stream: "SIZE:image_size"
//   input_stream: "NORM_RECTS:rois"
//   input_side_packet: "STICKER_IMAGE_CPU:sticker_image"
//   input_side_packet: "STICKER_ZOOM:sticker_zoom"
//   output_stream: "SPRITES:sprites"
//   options: {
//     [magritte.RoisToSpriteListCalculatorOptions.ext] {
//       sticker_is_premultiplied: false
//     }
//   }
// }
class RoisToSpriteListCalculator : public mediapipe::CalculatorBase {
 public:
  RoisToSpriteListCalculator() = default;
  ~RoisToSpriteListCalculator() override = default;

  static absl::Status GetContract(mediapipe::CalculatorContract* cc);
  absl::Status Open(mediapipe::CalculatorContext* cc) override;
  absl::Status Process(mediapipe::CalculatorContext* cc) override;
  absl::Status Close(mediapipe::CalculatorContext* cc) override;

  // Premultiplies the alpha channel in the color channels.
  // If the input is (r, g, b, a), the output should be (r*a, g*a, b*a, a*1).
  static absl::Status PremultiplyAlphaCpu(cv::Mat& sticker_rgba);

  // Find the minimum zoom that, after applied to the sticker, covers the ROI.
  static float FindFitZoom(const std::pair<int, int>& bg_size,
                           const std::pair<int, int>& sticker_size,
                           const mediapipe::NormalizedRect& roi);

 private:
  float sticker_zoom_;
  mediapipe::Packet sticker_packet_;
#if !defined(MEDIAPIPE_DISABLE_GPU)
  mediapipe::GlCalculatorHelper helper_;
  GLuint premultiply_program_;
  // Vertex attributes (NOTE: this assumes newer GL versions).
  GLuint vao_;
#endif  //  !MEDIAPIPE_DISABLE_GPU

  absl::Status OpenCpu(mediapipe::CalculatorContext* cc);
  absl::Status OpenGpu(mediapipe::CalculatorContext* cc);
#if !defined(MEDIAPIPE_DISABLE_GPU)
  absl::Status GlSetup();
  mediapipe::GlTexture RenderPremultiply(const mediapipe::GlTexture& image);
#endif  //  !MEDIAPIPE_DISABLE_GPU
};

}  // namespace magritte

#endif  // MAGRITTE_CALCULATORS_ROIS_TO_SPRITE_LIST_CALCULATOR_H_
