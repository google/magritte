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
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include "mediapipe/framework/formats/location_data.pb.h"
#include "magritte/calculators/rotation_calculator_options.pb.h"

namespace magritte {
using ::mediapipe::CalculatorBase;
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;
using ::mediapipe::Detection;
namespace {

typedef std::vector<Detection> Detections;
constexpr char kDetectionsTag[] = "DETECTIONS";
constexpr char kSizeTag[] = "SIZE";
constexpr char kRotationTag[] = "ROTATION_DEGREES";
}  // namespace

RotationMode::Mode DegreesToRotationMode(int degrees) {
  switch (degrees) {
    case 0:
      return RotationMode::ROTATION_0;
    case 90:
      return RotationMode::ROTATION_90;
    case 180:
      return RotationMode::ROTATION_180;
    case 270:
      return RotationMode::ROTATION_270;
    default:
      return RotationMode::UNKNOWN;
  }
}

// A calculator used to perform transformations on Detections, supports only
// relative-bounding-box based detections for now.
//
// Inputs:
//  1. DETECTIONS: Detections stream, containing detections in an image.
//                detection.location_data is assumed as relative_bouding_box
//                as only used like this for now.
//  2. SIZE: [Optional] Pair<int,int> containing original image size. Not used
//           yet since we don't need it to rotate relative_bounding_box.
//  3. ROTATION_DEGREES: [Optional] int representing the rotation in degrees to
//      apply to the detection. Only 0, 90, 180, 270 are considered valid input.
//      If no value is present, will default to the rotation value in option.
//      Counter-clockwise by default, but can be overwritten by the options.
//
// Output:
//  1. DETECTIONS: Detections stream, with bounding box and keypoints rotated.
//
// Options:
//  1. rotation_mode: Enum representing the angle of rotation, overwritten by
//        ROTATION_DEGREES packet if present.
//  2. clockwise: bool [default=false] direction of the rotation. anti-clockwise
//        by default. Doest apply to ROTATION_DEGREES packet and rotation_mode
//        option, whichever is relevent.
//
// Example config:
// node {
//   calculator: "DetectionTransformationCalculator"
//   input_stream: "DETECTIONS:detections_rotated"
//   input_stream: "SIZE:image_size"
//   output_stream: "DETECTIONS:output_detections"
//   options: {
//     [magritte.RotationCalculatorOptions.ext] {
//       rotation_mode: ROTATION_90  # anti-clockwise rotation
//     }
//   }
// }
class DetectionTransformationCalculator : public CalculatorBase {
 public:
  DetectionTransformationCalculator() = default;
  ~DetectionTransformationCalculator() override = default;

  static absl::Status GetContract(CalculatorContract* cc) {
    cc->Inputs().Tag(kDetectionsTag).Set<Detections>();
    if (cc->Inputs().HasTag(kSizeTag)) {
      cc->Inputs().Tag(kSizeTag).Set<std::pair<int, int>>();
    }
    cc->Outputs().Tag(kDetectionsTag).Set<Detections>();

    if (cc->Inputs().HasTag(kRotationTag)) {
      cc->Inputs().Tag(kRotationTag).Set<int>();
    }

    // No input side packets.
    return absl::OkStatus();
  }

  absl::Status Open(CalculatorContext* cc) override {
    return absl::OkStatus();
  }

  absl::Status Process(CalculatorContext* cc) override {
    const auto& options =
        cc->Options<RotationCalculatorOptions>();

    if (cc->Inputs().Tag(kDetectionsTag).Value().IsEmpty()) {
      LOG(WARNING) << "Empty detections at " << cc->InputTimestamp();
      cc->Outputs()
          .Tag(kDetectionsTag)
          .Add(new Detections(), cc->InputTimestamp());
      return absl::OkStatus();
    }
    if (cc->Inputs().Tag(kSizeTag).Value().IsEmpty()) {
      LOG(INFO) << "Empty size at " << cc->InputTimestamp();
    }

    Detections detections = cc->Inputs().Tag(kDetectionsTag).Get<Detections>();
    // TODO: Adapt to different types of location data.
    // Size is currently unused since relative bounding box don't need it.

    // auto size = cc->Inputs().Tag(kSizeTag).Get<std::pair<int,int>>();
    // auto image_width = size.first;
    // auto image_height = size.second;

    RotationMode_Mode rotation;
    if (cc->Inputs().HasTag(kRotationTag)) {
      rotation =
          DegreesToRotationMode(cc->Inputs().Tag(kRotationTag).Get<int>());
    } else {
      rotation = options.rotation_mode();
    }

    if (options.clockwise()) {
      switch (rotation) {
        case RotationMode::ROTATION_90:
          rotation = RotationMode::ROTATION_270;
          break;
        case RotationMode::ROTATION_270:
          rotation = RotationMode::ROTATION_90;
          break;
        default:
          break;
      }
    }

    Detections* output_detections = new Detections();
    for (Detection detection : detections) {
      // We assume the type to be relative bounding box, as it is the only one
      // used, completion of other types is welcome.
      RotateRelativeBoundingBox(
          *detection.mutable_location_data()->mutable_relative_bounding_box(),
          rotation);
      for (auto& keypoint :
           *detection.mutable_location_data()->mutable_relative_keypoints()) {
        RotateRelativeKeypoint(&keypoint, rotation);
      }
      output_detections->emplace_back(detection);
    }

    cc->Outputs()
        .Tag(kDetectionsTag)
        .Add(output_detections, cc->InputTimestamp());
    return absl::OkStatus();
  }

  static void RotateRelativeBoundingBox(
      mediapipe::LocationData_RelativeBoundingBox& box,
      const RotationMode_Mode rotation_mode) {
    switch (rotation_mode) {
      case RotationMode::UNKNOWN:
        break;
      case RotationMode::ROTATION_0:
        break;  // No rotation
      case RotationMode::ROTATION_90: {
        auto xmin = box.xmin();
        box.set_xmin(box.ymin());
        box.set_ymin(1.0 - xmin - box.width());

        auto height = box.height();
        box.set_height(box.width());
        box.set_width(height);
        break;
      }
      case RotationMode::ROTATION_180: {
        box.set_xmin(1.0 - box.xmin() - box.width());
        box.set_ymin(1.0 - box.ymin() - box.height());
        // width and height stay unchanged
        break;
      }
      case RotationMode::ROTATION_270: {
        auto ymin = box.ymin();
        box.set_ymin(box.xmin());
        box.set_xmin((1.0 - ymin - box.height()));

        auto height = box.height();
        box.set_height(box.width());
        box.set_width(height);
        break;
      }
    }
  }

  static void RotateRelativeKeypoint(
      mediapipe::LocationData_RelativeKeypoint* keypoint,
      const RotationMode_Mode rotation_mode) {
    mediapipe::LocationData_RelativeBoundingBox box;
    box.set_xmin(keypoint->x());
    box.set_ymin(keypoint->y());
    box.set_width(0);
    box.set_height(0);
    RotateRelativeBoundingBox(box, rotation_mode);
    keypoint->set_x(box.xmin());
    keypoint->set_y(box.ymin());
  }
};

REGISTER_CALCULATOR(DetectionTransformationCalculator);

}  // namespace magritte
