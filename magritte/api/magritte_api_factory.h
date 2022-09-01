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
#ifndef MAGRITTE_API_MAGRITTE_API_FACTORY_H_
#define MAGRITTE_API_MAGRITTE_API_FACTORY_H_

// This library provides methods to create instances of the interfaces defined
// in magritte_api.h.

#include <functional>
#include <memory>

#include "mediapipe/framework/calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/packet.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "magritte/api/magritte_api.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/gpu/gpu_buffer.h"
#include "mediapipe/framework/port/status.h"

namespace magritte {

// Given a graph, creates a synchronous Deidentifier operating on ImageFrames
// (for CPU processing).
// Returns an error if the given graph is not a top-level graph.
absl::StatusOr<std::unique_ptr<DeidentifierSync<mediapipe::ImageFrame>>>
CreateCpuDeidentifierSync(const mediapipe::CalculatorGraphConfig& graph_config);

// Given a graph, creates an asynchronous Deidentifier operating on ImageFrames
// (for CPU processing). The Deidentifier will call the callback on each
// completed frame.
// Returns an error if the given graph is not a top-level graph.
absl::StatusOr<std::unique_ptr<DeidentifierAsync<mediapipe::ImageFrame>>>
CreateCpuDeidentifierAsync(
    const mediapipe::CalculatorGraphConfig& graph_config,
    std::function<absl::Status(const mediapipe::ImageFrame&)> callback);

#if !defined(MEDIAPIPE_DISABLE_GPU)

// Given a graph. creates a synchronous Deidentifier operating on GpuBuffers
// (for GPU processing).
// Returns an error if the given graph is not a top-level graph.
absl::StatusOr<std::unique_ptr<DeidentifierSync<mediapipe::GpuBuffer>>>
CreateGpuDeidentifierSync(const mediapipe::CalculatorGraphConfig& graph_config);

// Given a graph, creates an asynchronous Deidentifier operating on GpuBuffers
// (for GPU processing). The Deidentifier will call the callback on each
// completed frame.
// Returns an error if the given graph is not a top-level graph.
absl::StatusOr<std::unique_ptr<DeidentifierAsync<mediapipe::GpuBuffer>>>
CreateGpuDeidentifierAsync(
    const mediapipe::CalculatorGraphConfig& graph_config,
    std::function<absl::Status(const mediapipe::GpuBuffer&)> callback);

#endif  //  !MEDIAPIPE_DISABLE_GPU

// Returns the CalculatorGraphConfig for a Magritte graph, given its name. The
// name is also called type, and is the value of the "type" field in the
// CalculatorGraphConfig proto, and the register_as argument in the
// magritte_graph build macro.
// For this to work, the code calling this function must have a dependency to
// the graph's cc_library target. If this dependency is missing, an error is
// returned.
// See https://google.github.io/magritte/technical_guide/graphs.html
// for an overview of existing graphs, including their names and build targets.
absl::StatusOr<mediapipe::CalculatorGraphConfig> MagritteGraphByName(
    const std::string& graph_name);

}  // namespace magritte

#endif  // MAGRITTE_API_MAGRITTE_API_FACTORY_H_
