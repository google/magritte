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
#include "magritte/api/internal/graph_runners.h"

#include <cstdint>

#include "mediapipe/framework/port/status.h"

namespace magritte {
namespace internal {

// GraphRunnerBase definitions

GraphRunnerBase::GraphRunnerBase(
    const mediapipe::CalculatorGraphConfig& graph_config)
    : graph_config_(graph_config) {}

absl::Status GraphRunnerBase::InitializeGraph() {
  return graph_.Initialize(graph_config_);
}

absl::Status GraphRunnerBase::Close() {
  MP_RETURN_IF_ERROR(graph_.CloseAllInputStreams());
  closed_ = true;
  return graph_.WaitUntilDone();
}

// Time in microseconds by how much the timestamp counter will be increased from
// the previously used timestamp in case no new timestamp is given. The value
// corresponds to 50fps.
constexpr int64_t kTimestampIncrease = 20000;

void GraphRunnerBase::Flush(int64_t last_timestamp) {
  next_timestamp_ = last_timestamp + kTimestampIncrease;
}

int64_t GraphRunnerBase::NextTimestamp() { return next_timestamp_; }

// GraphRunnerSync definitions

GraphRunnerSync::GraphRunnerSync(
    const mediapipe::CalculatorGraphConfig& graph_config)
    : GraphRunnerBase(graph_config) {}

absl::Status GraphRunnerSync::Preheat() {
  MP_RETURN_IF_ERROR(InitializeGraph());
  for (const auto& output_stream : graph_config_.output_stream()) {
    auto poller = graph_.AddOutputStreamPoller(output_stream);
    if (poller.ok()) {
      pollers_.emplace(output_stream, std::move(*poller));
    }
  }
  return graph_.StartRun({});
}

// GraphRunnerAsync definitions

GraphRunnerAsync::GraphRunnerAsync(
    const mediapipe::CalculatorGraphConfig& graph_config,
    absl::flat_hash_map<absl::string_view,
                        std::function<absl::Status(const mediapipe::Packet&)>>
        packet_callbacks)
    : GraphRunnerBase(graph_config), packet_callbacks_(packet_callbacks) {}

absl::Status GraphRunnerAsync::Preheat() {
  MP_RETURN_IF_ERROR(InitializeGraph());
  for (const std::pair<const std::string_view,
                       std::function<absl::Status(const mediapipe::Packet&)>>&
           callback : packet_callbacks_) {
    MP_RETURN_IF_ERROR(graph_.ObserveOutputStream(std::string(callback.first),
                                               callback.second));
  }
  return graph_.StartRun({});
}

}  // namespace internal
}  // namespace magritte
