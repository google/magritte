//
// Copyright 2018-2022 Google LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// An example of sending video frames into a Magritte graph.
#include <cstdlib>
#include <map>
#include <memory>

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include  <opencv2/highgui.hpp>
#include  <opencv2/imgproc.hpp>
#include  <opencv2/video.hpp>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/status/status.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/subgraph.h"

constexpr char kMagritteGraphNamespace[] = "magritte";
constexpr char kInputStream[] = "input_video";
constexpr char kOutputStream[] = "output_video";
constexpr char kStickerImageSidePacket[] = "sticker_image";
constexpr char kStickerZoomSidePacket[] = "sticker_zoom";
constexpr char kWindowName[] = "Magritte";

ABSL_FLAG(std::string, graph_type, "",
          "Type of the top level graph to be loaded from the dependencies.");
ABSL_FLAG(std::string, input_video, "",
          "Full path of video file to load. "
          "If not provided, attempt to use a webcam.");
ABSL_FLAG(std::string, output_video, "",
          "Full path of output video file (.mp4 only). "
          "If not provided, show result in a window.");
ABSL_FLAG(std::string, sticker_image, "",
          "Full path of sticker image file (.png only). "
          "Only used in the sticker redaction graphs. "
          "If not provided, a default smile emoji is used.");
ABSL_FLAG(float, sticker_zoom, 1.0,
          "The default extra zoom applied to the sticker. "
          "Only used in the sticker redaction graphs. "
          "If not provided, the default is 1.0.");

absl::Status RunMediaPipeGraph() {
  std::string calculator_graph_config_contents;

  std::map<std::string, mediapipe::Packet> input_side_packets;
  const std::string sticker_image = absl::GetFlag(FLAGS_sticker_image);
  if (!sticker_image.empty()) {
    input_side_packets[kStickerImageSidePacket] =
        mediapipe::MakePacket<std::string>(sticker_image);
  }
  input_side_packets[kStickerZoomSidePacket] =
      mediapipe::MakePacket<float>(absl::GetFlag(FLAGS_sticker_zoom));

  mediapipe::GraphRegistry graph_registry;
  ASSIGN_OR_RETURN(
      mediapipe::CalculatorGraphConfig config,
      graph_registry.CreateByName(kMagritteGraphNamespace,
                                  absl::GetFlag(FLAGS_graph_type)));

  LOG(INFO) << "Initialize the graph.";
  mediapipe::CalculatorGraph graph;
  MP_RETURN_IF_ERROR(graph.Initialize(config, input_side_packets));

  LOG(INFO) << "Initialize the camera or load the video.";
  cv::VideoCapture capture;
  const bool load_video = !absl::GetFlag(FLAGS_input_video).empty();
  if (load_video) {
    capture.open(absl::GetFlag(FLAGS_input_video));
    if (!capture.isOpened()) {
      return absl::NotFoundError(absl::StrCat(
          "Cannot open video file ", absl::GetFlag(FLAGS_input_video)));
    }
  } else {
    capture.open(0);
    if (!capture.isOpened()) {
      return absl::NotFoundError("Cannot open video capture device.");
    }
  }
  RET_CHECK(capture.isOpened());

  cv::VideoWriter writer;
  const bool save_video = !absl::GetFlag(FLAGS_output_video).empty();
  if (!save_video) {
    cv::namedWindow(kWindowName, /*flags=WINDOW_AUTOSIZE*/ 1);
#if (CV_MAJOR_VERSION >= 3) && (CV_MINOR_VERSION >= 2)
    capture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    capture.set(cv::CAP_PROP_FPS, 30);
#endif
  } else if (!load_video) {
    return absl::UnavailableError(
        "Command line flag 'output_video' requires 'input_video'");
  }

  LOG(INFO) << "Start running the graph.";
  ASSIGN_OR_RETURN(mediapipe::OutputStreamPoller poller,
                   graph.AddOutputStreamPoller(kOutputStream));
  MP_RETURN_IF_ERROR(graph.StartRun({}));

  LOG(INFO) << "Start grabbing and processing frames.";
  bool grab_frames = true;
  while (grab_frames) {
    // Capture opencv camera or video frame.
    cv::Mat camera_frame_raw;
    capture >> camera_frame_raw;
    if (camera_frame_raw.empty()) break;  // End of video.
    cv::Mat camera_frame;
    cv::cvtColor(camera_frame_raw, camera_frame, cv::COLOR_BGR2RGB);
    if (!load_video) {
      cv::flip(camera_frame, camera_frame, /*flipcode=HORIZONTAL*/ 1);
    }

    // Wrap Mat into an ImageFrame.
    auto input_frame = std::make_unique<mediapipe::ImageFrame>(
        mediapipe::ImageFormat::SRGB, camera_frame.cols, camera_frame.rows,
        mediapipe::ImageFrame::kDefaultAlignmentBoundary);
    cv::Mat input_frame_mat = mediapipe::formats::MatView(input_frame.get());
    camera_frame.copyTo(input_frame_mat);

    // Send image packet into the graph.
    size_t frame_timestamp_us =
        (double)cv::getTickCount() / (double)cv::getTickFrequency() * 1e6;
    MP_RETURN_IF_ERROR(graph.AddPacketToInputStream(
        kInputStream, mediapipe::Adopt(input_frame.release())
                          .At(mediapipe::Timestamp(frame_timestamp_us))));

    // Get the graph result packet, or stop if that fails.
    mediapipe::Packet packet;
    if (!poller.Next(&packet)) break;
    auto& output_frame = packet.Get<mediapipe::ImageFrame>();

    // Convert back to opencv for display or saving.
    cv::Mat output_frame_mat = mediapipe::formats::MatView(&output_frame);
    cv::cvtColor(output_frame_mat, output_frame_mat, cv::COLOR_RGB2BGR);
    if (save_video) {
      if (!writer.isOpened()) {
        LOG(INFO) << "Prepare video writer.";
        writer.open(absl::GetFlag(FLAGS_output_video),
                    cv::VideoWriter::fourcc('a', 'v', 'c', '1'),  // .mp4
                    capture.get(cv::CAP_PROP_FPS), output_frame_mat.size());
        RET_CHECK(writer.isOpened());
      }
      writer.write(output_frame_mat);
    } else {
      cv::imshow(kWindowName, output_frame_mat);
      // Press any key to exit.
      const int pressed_key = cv::waitKey(5);
      if (pressed_key >= 0 && pressed_key != 255) grab_frames = false;
    }
  }

  LOG(INFO) << "Shutting down.";
  if (writer.isOpened()) writer.release();
  MP_RETURN_IF_ERROR(graph.CloseInputStream(kInputStream));
  return graph.WaitUntilDone();
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);
  absl::Status run_status = RunMediaPipeGraph();
  if (!run_status.ok()) {
    LOG(ERROR) << "Failed to run the graph: " << run_status.message();
    return EXIT_FAILURE;
  } else {
    LOG(INFO) << "Success!";
  }
  return EXIT_SUCCESS;
}
