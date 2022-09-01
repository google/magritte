//
// Copyright 2021-2022 Google LLC
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
#ifndef MAGRITTE_CALCULATORS_NEW_CANVAS_CALCULATOR_H_
#define MAGRITTE_CALCULATORS_NEW_CANVAS_CALCULATOR_H_

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/util/color.pb.h"
#include "magritte/calculators/new_canvas_calculator.pb.h"

#if !defined(MEDIAPIPE_DISABLE_GPU)
#include "mediapipe/gpu/gl_calculator_helper.h"
#include "mediapipe/gpu/gl_simple_shaders.h"
#include "mediapipe/gpu/shader_util.h"
#endif  //  !MEDIAPIPE_DISABLE_GPU

namespace magritte {
using ::mediapipe::CalculatorBase;
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;

// A calculator that creates a new image with uniform color (set in options)
// using the type, dimensions and format of the input image.
//
// Inputs:
// - IMAGE or IMAGE_GPU: An ImageFrame or GpuBuffer stream, containing the image
//   dimensions and format.
//
// Outputs:
// - IMAGE or IMAGE_GPU: An ImageFrame or GpuBuffer stream, containing the new
//   canvas.
//
// Options:
// - color defining the new canvas color.
// - scaling information (see proto file for details).
//
// Example config:
// node {
//   calculator: "NewCanvasCalculator"
//   input_stream: "IMAGE:input_video"
//   output_stream: "IMAGE:output_video"
//   options: {
//     [magritte.NewCanvasCalculatorOptions.ext] {
//       color { r: 0 g: 0 b: 0 }
//     }
//   }
// }
class NewCanvasCalculator : public CalculatorBase {
 public:
  NewCanvasCalculator() = default;
  ~NewCanvasCalculator() override = default;

  static absl::Status GetContract(CalculatorContract* cc);
  absl::Status Open(CalculatorContext* cc) override;
  absl::Status Process(CalculatorContext* cc) override;

  // Returns the size for the new canvas according to the calculator options.
  // If scaling information is present, the original size will be scaled,
  // otherwise the original size is returned.
  static std::pair<int, int> GetSizeFromOptions(
      const NewCanvasCalculatorOptions& options, const int original_width,
      const int original_height);

 private:
#if !defined(MEDIAPIPE_DISABLE_GPU)
  ::mediapipe::GlCalculatorHelper helper_;
#endif  //  !MEDIAPIPE_DISABLE_GPU
  absl::Status ProcessCpu(CalculatorContext* cc);
  absl::Status ProcessGpu(CalculatorContext* cc);
};

}  // namespace magritte

#endif  // MAGRITTE_CALCULATORS_NEW_CANVAS_CALCULATOR_H_
