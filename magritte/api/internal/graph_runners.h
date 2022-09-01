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
#ifndef MAGRITTE_API_INTERNAL_GRAPH_RUNNERS_H_
#define MAGRITTE_API_INTERNAL_GRAPH_RUNNERS_H_

// This library defines graph runner classes that allow to run MediaPipe graphs
// synchronously or asynchronously. They are used as a common base for the API
// implementations in api_implementations.h

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

#include "mediapipe/framework/calculator.pb.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/packet.h"
#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "mediapipe/framework/formats/detection.pb.h"
#include "mediapipe/framework/output_stream_poller.h"

namespace magritte {
namespace internal {

// Graph runner base class. It is extended by more specialized synchronous and
// asynchronous graph runner classes below, and it contains the common logic
// that can be shared between these classes.
class GraphRunnerBase {
 protected:
  // Constructor that takes a graph config. Doesn't perform any initialization.
  GraphRunnerBase(const mediapipe::CalculatorGraphConfig& graph_config);

  // Initializes the graph_ field with the graph_config_. Does not yet start
  // running the graph.
  absl::Status InitializeGraph();

  // Closes the graphs's input streams and waits for it to be done.
  absl::Status Close();

  // Should be called when all packets that should be processed at once
  // (meaning, with the same timestamp) have been added to their respective
  // input streams. It increases next_timestamp_.
  void Flush(int64_t last_timestamp)
      ABSL_EXCLUSIVE_LOCKS_REQUIRED(timestamp_mutex_);

  // Returns the next available timestamp.
  int64_t NextTimestamp() ABSL_SHARED_LOCKS_REQUIRED(timestamp_mutex_);

  // Adds data to an input stream at the given timestamp.  This method returns
  // immediately, so it doesn't wait for the packet to be processed. This is to
  // allow adding other packets to other input streams at the same timestamp.
  template <typename T>
  absl::Status AddToInputStream(absl::string_view input_stream,
                                std::unique_ptr<T> input,
                                int64_t timestamp_us) {
    if (closed_) {
      return absl::FailedPreconditionError("graph runner has been closed");
    }
    return graph_.AddPacketToInputStream(
        std::string(input_stream), mediapipe::Adopt(input.release())
                                       .At(mediapipe::Timestamp(timestamp_us)));
  }

  // Graph config for the graph to be run. It needs to be stored in a field to
  // be able to query input and output streams.
  mediapipe::CalculatorGraphConfig graph_config_;

  // The graph that is to be run.
  mediapipe::CalculatorGraph graph_;

  // Whether the graph has been closed.
  bool closed_ = false;

  // A mutex to guard the internal timestamp.
  absl::Mutex timestamp_mutex_;

 private:
  // An internal timestamp counter. Stores the next available timestamp that
  // will be used in case of adding a packet without a timestamp.
  int64_t next_timestamp_ ABSL_GUARDED_BY(timestamp_mutex_);
};

// A synchronous graph runner. It allow adding packets to an input stream and
// query for a corresponding output packet. The latter will block until the
// packet is available.
class GraphRunnerSync : public GraphRunnerBase {
 public:
  // Adds output stream pollers to all existing output streams and then starts
  // running the graph.
  absl::Status Preheat();

 protected:
  GraphRunnerSync(const mediapipe::CalculatorGraphConfig& graph_config);

  // Polls output from the given output stream. This method blocks until the
  // output is available.
  template <typename T>
  absl::StatusOr<std::unique_ptr<T>> PollOutput(
      absl::string_view output_stream) {
    auto it = pollers_.find(output_stream);
    if (it == pollers_.end()) {
      return absl::NotFoundError(absl::Substitute(
          "no output stream found with name $0", output_stream));
    }
    mediapipe::OutputStreamPoller& poller = it->second;
    mediapipe::Packet packet;
    if (!poller.Next(&packet)) {
      return absl::NotFoundError(absl::Substitute(
          "no output stream found with name $0", output_stream));
    }
    return packet.Consume<T>();
  }

 private:
  // Stores the output stream pollers that are connected to the graph.
  absl::flat_hash_map<absl::string_view, mediapipe::OutputStreamPoller>
      pollers_;
};

// An asynchronous graph runner. It works with callbacks for output streams that
// are defined upfront.
class GraphRunnerAsync : public GraphRunnerBase {
 public:
  // Adds output stream observers and then starts running the graph.
  absl::Status Preheat();

 protected:
  GraphRunnerAsync(
      const mediapipe::CalculatorGraphConfig& graph_config,
      absl::flat_hash_map<absl::string_view,
                          std::function<absl::Status(const mediapipe::Packet&)>>
          packet_callbacks);

 private:
  // Stores the packet callbacks for each output stream.
  absl::flat_hash_map<absl::string_view,
                      std::function<absl::Status(const mediapipe::Packet&)>>
      packet_callbacks_;
};

}  // namespace internal
}  // namespace magritte

#endif  // MAGRITTE_API_INTERNAL_GRAPH_RUNNERS_H_
