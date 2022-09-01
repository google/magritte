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
#ifndef MAGRITTE_CALCULATORS_DETECTION_TRANSFORMATION_CALCULATOR_H_
#define MAGRITTE_CALCULATORS_DETECTION_TRANSFORMATION_CALCULATOR_H_

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include "mediapipe/framework/formats/location_data.pb.h"
#include "magritte/calculators/rotation_calculator_options.pb.h"

namespace magritte {
using ::mediapipe::CalculatorBase;
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;

// A calculator used to perform transformations on Detections, supports only
// relative-bounding-box based detections for now.
//
// Inputs:
// - DETECTIONS: Detections stream, containing detections in an image.
//   detection.location_data is assumed as relative_bouding_box as only used
//   like this for now.
// - SIZE: [Optional] Pair<int,int> containing original image size. Not used yet
//   since we don't need it to rotate relative_bounding_box.
//
// Outputs:
// - DETECTIONS: Detections stream, with bouding box and keypoints rotated.
//
// Example config:
// node {
//   calculator: "DetectionTransformationCalculator"
//   input_stream: "DETECTIONS:detections_rotated"
//   input_stream: "SIZE:image_size"
//   output_stream: "DETECTIONS:output_detections"
//   options: {
//     [magritte.RotationCalculatorOptions.ext] {
//       rotation_mode: 2 #90 degree anti-clockwise rotation
//     }
//   }
// }

RotationMode::Mode DegreesToRotationMode(int degrees);

class DetectionTransformationCalculator : public CalculatorBase {
 public:
  static absl::Status GetContract(CalculatorContract* cc);

  absl::Status Open(CalculatorContext* cc);

  absl::Status Process(CalculatorContext* cc);

  static void RotateRelativeBoundingBox(
      mediapipe::LocationData_RelativeBoundingBox& box,
      const RotationMode_Mode rotation_mode);

  static void RotateRelativeKeypoint(
      mediapipe::LocationData_RelativeKeypoint* keypoint,
      const RotationMode_Mode rotation_mode);
};
}  // namespace magritte

#endif  // MAGRITTE_CALCULATORS_DETECTION_TRANSFORMATION_CALCULATOR_H_
