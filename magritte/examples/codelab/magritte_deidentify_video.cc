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
#include <cstdint>
#include <memory>
#include <string>

#include "mediapipe/framework/port/logging.h"
#include "absl/flags/parse.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include  <opencv2/highgui.hpp>
#include  <opencv2/imgproc.hpp>
#include  <opencv2/video.hpp>
#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "magritte/api/magritte_api.h"
#include "magritte/api/magritte_api_factory.h"
#include "mediapipe/framework/calculator.pb.h"
#include "mediapipe/framework/port/status.h"

// Name of the Magritte graph that will be used for processing.
constexpr char kGraphName[] = "FacePixelizationOfflineCpu";

ABSL_FLAG(std::string, input_file, "", "input file path");
ABSL_FLAG(std::string, output_file, "", "output file path");

// Uses the synchronous Magritte API to deidentify a video file and save the
// result to an output file.
absl::Status Run(const std::string& graph_name, const std::string& input_file,
                 const std::string& output_file) {
  // Open video.
  cv::VideoCapture capture(input_file);
  if (!capture.isOpened()) {
    return absl::NotFoundError(
        absl::StrCat("Cannot open video file ", input_file));
  }

  // Calculate duration of a single frame in microseconds.
  double fps = capture.get(cv::CAP_PROP_FPS);
  int64_t frame_duration_us = 1e6 / fps;

  // Load graph and create Deidentifier.
  ASSIGN_OR_RETURN(mediapipe::CalculatorGraphConfig graph_config,
                   magritte::MagritteGraphByName(graph_name));
  ASSIGN_OR_RETURN(
      std::unique_ptr<magritte::DeidentifierSync<mediapipe::ImageFrame>>
          deidentifier,
      magritte::CreateCpuDeidentifierSync(graph_config));

  // Read, process and write frames from the video until reaching the end.
  cv::VideoWriter writer;
  cv::Mat frame_raw;
  int frame_number = 0;
  for (capture >> frame_raw; !frame_raw.empty(); capture >> frame_raw) {
    ++frame_number;

    // Convert raw frame into an ImageFrame with the right format.
    auto input_frame = std::make_unique<mediapipe::ImageFrame>(
        mediapipe::ImageFormat::SRGB, frame_raw.cols, frame_raw.rows,
        mediapipe::ImageFrame::kDefaultAlignmentBoundary);
    cv::cvtColor(frame_raw, mediapipe::formats::MatView(input_frame.get()),
                 cv::COLOR_BGR2RGB);

    // Send the ImageFrame to the Deidentifier.
    ASSIGN_OR_RETURN(
        std::unique_ptr<mediapipe::ImageFrame> deidentified_frame,
        deidentifier->Deidentify(std::move(input_frame),
                                 frame_number * frame_duration_us));

    // Convert the result back to the format required for writing.
    cv::Mat deidentified_mat;
    cv::cvtColor(mediapipe::formats::MatView(deidentified_frame.get()),
                 deidentified_mat, cv::COLOR_RGB2BGR);

    // Write the output frame.
    if (!writer.isOpened()) {
      writer.open(output_file,
                  cv::VideoWriter::fourcc('a', 'v', 'c', '1'),  // .mp4
                  capture.get(cv::CAP_PROP_FPS), deidentified_mat.size());
    }
    writer.write(deidentified_mat);
  }
  capture.release();
  writer.release();
  return deidentifier->Close();
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);

  std::string input_file = absl::GetFlag(FLAGS_input_file);
  std::string output_file = absl::GetFlag(FLAGS_output_file);
  absl::Status status = Run(kGraphName, input_file, output_file);
  LOG(INFO) << status;
  return status.raw_code();
}
