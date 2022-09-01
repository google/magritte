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
#include "magritte/api/magritte_api_factory.h"

#include <memory>

#include "mediapipe/framework/calculator.pb.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "magritte/api/internal/api_implementations.h"
#include "mediapipe/framework/subgraph.h"
#include "mediapipe/framework/port/status.h"

namespace magritte {

namespace {
// Checks whether the given graph can be used as a deidentifaction graph.
absl::Status CheckValidDeidentificationGraph(
    const mediapipe::CalculatorGraphConfig& graph_config) {
  if (graph_config.input_stream_size() != 1) {
    return absl::InvalidArgumentError(
        "graph must have exactly one input stream");
  }
  if (graph_config.output_stream_size() != 1) {
    return absl::InvalidArgumentError(
        "graph must have exactly one output stream");
  }
  if (graph_config.output_side_packet_size() != 0) {
    return absl::InvalidArgumentError(
        "graph must not have output side packets");
  }
  if (graph_config.input_stream(0) != internal::kImageInputStreamTag) {
    return absl::InvalidArgumentError(absl::StrCat(
        "input stream must be tagged ", internal::kImageInputStreamTag));
  }
  if (graph_config.output_stream(0) != internal::kImageOutputStreamTag) {
    return absl::InvalidArgumentError(absl::StrCat(
        "output stream must be tagged ", internal::kImageOutputStreamTag));
  }
  return absl::OkStatus();
}

constexpr char kMagritteGraphNamespace[] = "magritte";
}  // namespace

absl::StatusOr<std::unique_ptr<DeidentifierSync<mediapipe::ImageFrame>>>
CreateCpuDeidentifierSync(const mediapipe::CalculatorGraphConfig& graph_config) {
  MP_RETURN_IF_ERROR(CheckValidDeidentificationGraph(graph_config));
  auto Deidentifier =
      std::make_unique<internal::DeidentifierSyncImpl<mediapipe::ImageFrame>>(
          graph_config);
  MP_RETURN_IF_ERROR(Deidentifier->Preheat());
  return Deidentifier;
}

absl::StatusOr<std::unique_ptr<DeidentifierAsync<mediapipe::ImageFrame>>>
CreateCpuDeidentifierAsync(
    const mediapipe::CalculatorGraphConfig& graph_config,
    std::function<absl::Status(const mediapipe::ImageFrame&)> callback) {
  MP_RETURN_IF_ERROR(CheckValidDeidentificationGraph(graph_config));
  auto Deidentifier =
      std::make_unique<internal::DeidentifierAsyncImpl<mediapipe::ImageFrame>>(
          graph_config, callback);
  MP_RETURN_IF_ERROR(Deidentifier->Preheat());
  return Deidentifier;
}

#if !defined(MEDIAPIPE_DISABLE_GPU)

absl::StatusOr<std::unique_ptr<DeidentifierSync<mediapipe::GpuBuffer>>>
CreateGpuDeidentifierSync(const mediapipe::CalculatorGraphConfig& graph_config) {
  MP_RETURN_IF_ERROR(CheckValidDeidentificationGraph(graph_config));
  auto Deidentifier =
      std::make_unique<internal::DeidentifierSyncImpl<mediapipe::GpuBuffer>>(
          graph_config);
  MP_RETURN_IF_ERROR(Deidentifier->Preheat());
  return Deidentifier;
}

absl::StatusOr<std::unique_ptr<DeidentifierAsync<mediapipe::GpuBuffer>>>
CreateGpuDeidentifierAsync(
    const mediapipe::CalculatorGraphConfig& graph_config,
    std::function<absl::Status(const mediapipe::GpuBuffer&)> callback) {
  MP_RETURN_IF_ERROR(CheckValidDeidentificationGraph(graph_config));
  auto Deidentifier =
      std::make_unique<internal::DeidentifierAsyncImpl<mediapipe::GpuBuffer>>(
          graph_config, callback);
  MP_RETURN_IF_ERROR(Deidentifier->Preheat());
  return Deidentifier;
}

#endif  //  !MEDIAPIPE_DISABLE_GPU

absl::StatusOr<mediapipe::CalculatorGraphConfig> MagritteGraphByName(
    const std::string& graph_name) {
  mediapipe::GraphRegistry graph_registry;
  return graph_registry.CreateByName(kMagritteGraphNamespace, graph_name);
}

}  // namespace magritte
