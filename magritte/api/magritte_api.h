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

#ifndef MAGRITTE_API_MAGRITTE_API_H_
#define MAGRITTE_API_MAGRITTE_API_H_

// This header file defines interfaces for the Magritte API. All the classes are
// abstract. To obtain instances, use the factory methods in
// magritte_api_factory.h.

#include <cstdint>

#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace magritte {

// A class to deidentify frames synchronously with Magritte. Deidentifying means
// detecting and redacting sensitive content. The template T can refer to either
// mediapipe::GpuBuffer or mediapipe::ImageFrame, depending on whether or not a GPU
// is used.
// At time of creation of an instance of this class, processing threads will be
// started so that it is immediately ready to consume input.
template <typename T>
class DeidentifierSync {
 public:
  virtual ~DeidentifierSync() = default;

  // Deidentifies a given frame, i.e. detects and redacts sensitive content in
  // it and returns the resulting redacted frame. The method blocks until the
  // processing is complete.
  // The timestamp can be any positive integer. It is meant to indicate
  // sequential dependencies between frames, to make sure they are processed in
  // the right order. This is important if a processing graph is used that is
  // intended for videos. If no such graph is used, the timestamps don't matter
  // and we recommend using the method below instead.
  // Subsequent calls to Deidentify() must use strictly monotonically increasing
  // timestamps. If they don't, an invalid argument error is returned.
  virtual absl::StatusOr<std::unique_ptr<T>> Deidentify(
      std::unique_ptr<T> image, int64_t timestamp) = 0;

  // Deidentifies a given frame, i.e. detects and redacts sensitive content in
  // it and returns the resulting redacted frame. The method blocks until the
  // processing is complete.
  // In this method you don't have to specify a timestamp. Use this method only
  // if you are running a graph that is not intended for video processing (e.g.,
  // a graph without tracking), otherwise use the method above with the
  // timestamp.
  // Do not mix this function with the timestamped one above -- if you call this
  // one, do not call the timestamped one afterwards. Doing so might result in
  // an invalid argument error.
  virtual absl::StatusOr<std::unique_ptr<T>> Deidentify(
      std::unique_ptr<T> image) = 0;

  // Stops processing threads and cleans up data. After calling this,
  // Deidentify() should not be called any more (it will return a failed
  // precondition error if called anyway).
  // Note that the processing threads are started at the time when an instance
  // of this class is created.
  virtual absl::Status Close() = 0;
};

// A class to deidentify frames asynchronously with Magritte. Deidentifying
// means detecting and redacting sensitive content. The template T can refer to
// either mediapipe::GpuBuffer or mediapipe::ImageFrame, depending on whether or not
// a GPU is used.
// At time of creation of an instance of this class, processing threads will be
// started so that it is immediately ready to consume input.
template <typename T>
class DeidentifierAsync {
 public:
  virtual ~DeidentifierAsync() = default;

  // Deidentifies a given frame, i.e. detects and redacts sensitive content in
  // it and returns the resulting redacted frame. The method will return
  // immediately. Once the result is ready, a callback will be called.
  // The callback will be provided in the factory method for this class (see
  // magritte_api_factory.h).
  // The timestamp can be any positive integer. It is meant to indicate
  // sequential dependencies between frames, to make sure they are processed in
  // the right order. This is important if a processing graph is used that is
  // intended for videos. If no such graph is used, the timestamps don't matter
  // and we recommend using the method below instead.
  // Subsequent calls to Deidentify() must use strictly monotonically increasing
  // timestamps. If they don't, an invalid argument error is returned.
  virtual absl::Status Deidentify(std::unique_ptr<T> image,
                                  int64_t timestamp) = 0;

  // Deidentifies a given frame, i.e. detects and redacts sensitive content in
  // it and returns the resulting redacted frame. The method will return
  // immediately. Once the result is ready, a callback will be called.
  // The callback will be provided in the factory method for this class (see
  // magritte_api_factory.h).
  // In this method you don't have to specify a timestamp. Use this method only
  // if you are running a graph that is not intended for video processing (e.g.,
  // a graph without tracking), otherwise use the method above with the
  // timestamp.
  // Do not mix this function with the timestamped one above -- if you call this
  // one, do not call the timestamped one afterwards. Doing so might result in
  // an invalid argument error.
  virtual absl::Status Deidentify(std::unique_ptr<T> image) = 0;

  // Stops processing threads and cleans up data. After calling this,
  // Deidentify() should not be called any more (it will return a failed
  // precondition error if called anyway).
  // Note that the processing threads are started at the time when an instance
  // of this class is created.
  virtual absl::Status Close() = 0;
};

// TODO: Add API definitions for detection only and redaction only.

}  // namespace magritte

#endif  // MAGRITTE_API_MAGRITTE_API_H_
