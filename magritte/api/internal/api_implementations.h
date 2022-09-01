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
#ifndef MAGRITTE_API_INTERNAL_API_IMPLEMENTATIONS_H_
#define MAGRITTE_API_INTERNAL_API_IMPLEMENTATIONS_H_

// This library contains implementations of the interfaces defined in
// magritte_api.h (one level above), building on the graph runner classes
// defined in graph_runners.h.

#include <cstdint>

#include "mediapipe/framework/calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/packet.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "magritte/api/internal/graph_runners.h"
#include "magritte/api/magritte_api.h"
#include "mediapipe/framework/formats/detection.pb.h"

namespace magritte {
namespace internal {

constexpr absl::string_view kImageInputStreamTag = "input_video";
constexpr absl::string_view kImageOutputStreamTag = "output_video";

// An implementation of DeidentifierSync<T>.
template <typename T>
class DeidentifierSyncImpl final : public DeidentifierSync<T>,
                                   public GraphRunnerSync {
 public:
  DeidentifierSyncImpl(const mediapipe::CalculatorGraphConfig& graph_config)
      : GraphRunnerSync(graph_config) {}

  // Deidentifies a given frame using the methods defined by GraphRunnerSync.
  absl::StatusOr<std::unique_ptr<T>> Deidentify(std::unique_ptr<T> image,
                                                int64_t timestamp_us) override {
    timestamp_mutex_.Lock();
    MP_RETURN_IF_ERROR(
        AddToInputStream(kImageInputStreamTag, std::move(image), timestamp_us));
    Flush(timestamp_us);
    timestamp_mutex_.Unlock();
    absl::StatusOr<std::unique_ptr<T>> output =
        PollOutput<T>(kImageOutputStreamTag);
    return output;
  }

  // Deidentifies a given frame using the methods defined by GraphRunnerSync.
  absl::StatusOr<std::unique_ptr<T>> Deidentify(
      std::unique_ptr<T> image) override {
    return Deidentify(std::move(image), NextTimestamp());
  }

  absl::Status Close() override { return GraphRunnerBase::Close(); }
};

// An implementation of DeidentifierAsync<T>.
template <typename T>
class DeidentifierAsyncImpl final : public DeidentifierAsync<T>,
                                    public GraphRunnerAsync {
 public:
  DeidentifierAsyncImpl(const mediapipe::CalculatorGraphConfig& graph_config,
                        const std::function<absl::Status(const T&)>& callback)
      : GraphRunnerAsync(graph_config,
                         {{kImageOutputStreamTag,
                           [&callback](const mediapipe::Packet& packet) {
                             return callback(packet.Get<T>());
                           }}}) {}

  // Deidentifies a given frame using the methods defined by GraphRunnerAsync.
  absl::Status Deidentify(std::unique_ptr<T> image,
                          int64_t timestamp_us) override {
    timestamp_mutex_.Lock();
    absl::Status status = AddToInputStream(std::string(kImageInputStreamTag),
                                           std::move(image), timestamp_us);
    Flush(timestamp_us);
    timestamp_mutex_.Unlock();
    return status;
  }

  // Deidentifies a given frame using the methods defined by GraphRunnerAsync.
  absl::Status Deidentify(std::unique_ptr<T> image) override {
    return Deidentify(std::move(image), NextTimestamp());
  }

  absl::Status Close() override { return GraphRunnerBase::Close(); }
};

// TODO: Implement classes for detection only and redaction only.

}  // namespace internal
}  // namespace magritte

#endif  // MAGRITTE_API_INTERNAL_API_IMPLEMENTATIONS_H_
