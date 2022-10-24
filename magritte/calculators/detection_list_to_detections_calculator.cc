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
#include <algorithm>
#include <memory>
#include <vector>

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/detection.pb.h"

namespace magritte {
namespace {
using ::mediapipe::CalculatorBase;
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;
using ::mediapipe::Detection;
using ::mediapipe::DetectionList;
using Detections = std::vector<Detection>;
constexpr char kDetectionListTag[] = "DETECTION_LIST";
constexpr char kDetectionsTag[] = "DETECTIONS";
}  // namespace

// A calculator that takes DetectionList and converts to std::vector<Detection>.
//
// Inputs:
// - DETECTION_LIST: A DetectionList containing a list of detections.
//
// Outputs:
// - DETECTIONS: An std::vector<Detection> containing the same data.
class DetectionListToDetectionsCalculator : public CalculatorBase {
 public:
  DetectionListToDetectionsCalculator() = default;
  ~DetectionListToDetectionsCalculator() override = default;

  static absl::Status GetContract(CalculatorContract* cc) {
    cc->Inputs().Tag(kDetectionListTag).Set<DetectionList>();
    cc->Outputs().Tag(kDetectionsTag).Set<Detections>();

    // No input side packets.
    return absl::OkStatus();
  }

  absl::Status Open(CalculatorContext* cc) override {
    return absl::OkStatus();
  }

  absl::Status Process(CalculatorContext* cc) override {
    if (cc->Inputs().Tag(kDetectionListTag).Value().IsEmpty()) {
      LOG(INFO) << "Empty detections at " << cc->InputTimestamp();
      return absl::OkStatus();
    }

    DetectionList detectionList =
        cc->Inputs().Tag(kDetectionListTag).Get<DetectionList>();
    auto output_detections = std::make_unique<Detections>();
    for (const Detection& detection : detectionList.detection()) {
      output_detections->emplace_back(detection);
    }

    cc->Outputs()
        .Tag(kDetectionsTag)
        .Add(output_detections.release(), cc->InputTimestamp());
    return absl::OkStatus();
  }
};

REGISTER_CALCULATOR(DetectionListToDetectionsCalculator);

}  // namespace magritte
