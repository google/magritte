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
#include <memory>
#include <string>

#include "mediapipe/framework/port/logging.h"
#include "absl/flags/parse.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "magritte/api/magritte_api.h"
#include "magritte/api/magritte_api_factory.h"
#include "magritte/examples/codelab/image_io_util.h"
#include "mediapipe/framework/calculator.pb.h"
#include "mediapipe/framework/port/status.h"

// Name of the Magritte graph that will be used for processing.
constexpr char kGraphName[] = "FacePixelizationOfflineCpu";

ABSL_FLAG(std::string, input_file, "", "input file path");
ABSL_FLAG(std::string, output_file, "", "output file path");
ABSL_FLAG(bool, async, false, "Deidentify asynchronously");

// Uses the synchronous Magritte API to deidentify an image file and save the
// result to an output file.
absl::Status RunSync(const std::string& graph_name,
                     const std::string& input_file,
                     const std::string& output_file) {
  ASSIGN_OR_RETURN(mediapipe::CalculatorGraphConfig graph_config,
                   magritte::MagritteGraphByName(graph_name));
  ASSIGN_OR_RETURN(std::unique_ptr<mediapipe::ImageFrame> image,
                   magritte::LoadFromFile(input_file));
  ASSIGN_OR_RETURN(
      std::unique_ptr<magritte::DeidentifierSync<mediapipe::ImageFrame>>
          deidentifier,
      magritte::CreateCpuDeidentifierSync(graph_config));
  ASSIGN_OR_RETURN(std::unique_ptr<mediapipe::ImageFrame> result,
                   deidentifier->Deidentify(std::move(image)));
  MP_RETURN_IF_ERROR(deidentifier->Close());
  return magritte::SaveToFile(output_file, *result);
}

// Uses the aynchronous Magritte API to deidentify an image file and save the
// result to an output file.
absl::Status RunAsync(const std::string& graph_name,
                      const std::string& input_file,
                      const std::string& output_file) {
  ASSIGN_OR_RETURN(mediapipe::CalculatorGraphConfig graph_config,
                   magritte::MagritteGraphByName(graph_name));
  ASSIGN_OR_RETURN(std::unique_ptr<mediapipe::ImageFrame> image,
                   magritte::LoadFromFile(input_file));
  ASSIGN_OR_RETURN(
      std::unique_ptr<magritte::DeidentifierAsync<mediapipe::ImageFrame>>
          deidentifier,
      magritte::CreateCpuDeidentifierAsync(
          graph_config, [&output_file](const mediapipe::ImageFrame& image) {
            return magritte::SaveToFile(output_file, image);
          }));
  MP_RETURN_IF_ERROR(deidentifier->Deidentify(std::move(image), 0));
  return deidentifier->Close();
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);

  std::string input_file = absl::GetFlag(FLAGS_input_file);
  std::string output_file = absl::GetFlag(FLAGS_output_file);
  absl::Status status;
  if (absl::GetFlag(FLAGS_async)) {
    status = RunAsync(kGraphName, input_file, output_file);
  } else {
    status = RunSync(kGraphName, input_file, output_file);
  }
  LOG(INFO) << status;
  return status.raw_code();
}
