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
#include "magritte/calculators/rois_to_sprite_list_calculator.h"

#include <algorithm>
#include <memory>

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "absl/memory/memory.h"
#include "magritte/calculators/rois_to_sprite_list_calculator.pb.h"
#include "magritte/calculators/sprite_list.h"
#include "mediapipe/framework/formats/rect.pb.h"
#include  <opencv2/imgproc.hpp>

#if !defined(MEDIAPIPE_DISABLE_GPU)
#include "mediapipe/gpu/gl_calculator_helper.h"
#include "mediapipe/gpu/gl_simple_shaders.h"
#include "mediapipe/gpu/shader_util.h"
#endif  // !MEDIAPIPE_DISABLE_GPU

namespace magritte {

namespace {
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;
using ::mediapipe::ImageFrame;
#if !defined(MEDIAPIPE_DISABLE_GPU)
using ::mediapipe::GlCalculatorHelper;
using ::mediapipe::GlTexture;
using ::mediapipe::GpuBuffer;
#endif  // !MEDIAPIPE_DISABLE_GPU
using ::mediapipe::MakePacket;
using ::mediapipe::NormalizedRect;
using ::mediapipe::formats::MatView;
using ::mediapipe::ImageFormat;
using NormalizedRects = std::vector<NormalizedRect>;
using Size = std::pair<int, int>;

enum { kAttributeVertex, kAttributeTexturePosition, kNumAttributes };

constexpr char kImageSizeTag[] = "SIZE";
constexpr char kNormalizedRectsTag[] = "NORM_RECTS";
constexpr char kStickerImageCpuTag[] = "STICKER_IMAGE_CPU";
constexpr char kStickerImageGpuTag[] = "STICKER_IMAGE_GPU";
constexpr char kStickerZoomTag[] = "STICKER_ZOOM";
constexpr char kSpriteListTag[] = "SPRITES";
}  // namespace

// static
absl::Status RoisToSpriteListCalculator::GetContract(CalculatorContract* cc) {
  RET_CHECK(cc != nullptr) << "CalculatorContract is nullptr.";
  cc->Inputs().Tag(kImageSizeTag).Set<Size>();
  cc->Inputs().Tag(kNormalizedRectsTag).Set<NormalizedRects>();

  RET_CHECK(cc->InputSidePackets().HasTag(kStickerImageCpuTag) ^
            cc->InputSidePackets().HasTag(kStickerImageGpuTag))
      << "Calculator should have one and only one sticker image input side "
         "packet";

  if (cc->InputSidePackets().HasTag(kStickerImageCpuTag)) {
    cc->InputSidePackets().Tag(kStickerImageCpuTag).Set<ImageFrame>();
  }

#if !defined(MEDIAPIPE_DISABLE_GPU)
  if (cc->InputSidePackets().HasTag(kStickerImageGpuTag)) {
    cc->InputSidePackets().Tag(kStickerImageGpuTag).Set<GpuBuffer>();
  }
  MP_RETURN_IF_ERROR(mediapipe::GlCalculatorHelper::UpdateContract(cc));
#endif  // !MEDIAPIPE_DISABLE_GPU

  cc->InputSidePackets().Tag(kStickerZoomTag).Set<float>();
  cc->Outputs().Tag(kSpriteListTag).Set<SpriteList>();

  return absl::OkStatus();
}

absl::Status RoisToSpriteListCalculator::Open(CalculatorContext* cc) {
  RET_CHECK(cc != nullptr) << "CalculatorContext is nullptr.";
  sticker_zoom_ = cc->InputSidePackets().Tag(kStickerZoomTag).Get<float>();

  bool use_gpu = cc->InputSidePackets().HasTag(kStickerImageGpuTag);
  if (use_gpu) {
    return OpenGpu(cc);
  } else {
    return OpenCpu(cc);
  }
}

absl::Status RoisToSpriteListCalculator::OpenCpu(CalculatorContext* cc) {
  sticker_packet_ = cc->InputSidePackets().Tag(kStickerImageCpuTag);
  cv::Mat sticker_mat = MatView(&(sticker_packet_.Get<ImageFrame>()));
  bool sticker_has_alpha = sticker_mat.channels() == 4;

  if (!sticker_has_alpha) {
    // Prevent black rectangle around the sticker if it is 3-channel.
    sticker_packet_ = MakePacket<ImageFrame>(
        ImageFormat::SRGBA, sticker_mat.cols, sticker_mat.rows);
    cv::Mat sticker_rgba = MatView(&(sticker_packet_.Get<ImageFrame>()));
    cv::cvtColor(sticker_mat, sticker_rgba, cv::COLOR_RGB2RGBA);
  } else {
    const RoisToSpriteListCalculatorOptions& options =
        cc->Options<RoisToSpriteListCalculatorOptions>();
    if (!options.sticker_is_premultiplied()) {
      MP_RETURN_IF_ERROR(PremultiplyAlphaCpu(sticker_mat));
    }
  }

  return absl::OkStatus();
}

absl::Status RoisToSpriteListCalculator::OpenGpu(CalculatorContext* cc) {
  sticker_packet_ = cc->InputSidePackets().Tag(kStickerImageGpuTag);
#if !defined(MEDIAPIPE_DISABLE_GPU)
  MEDIAPIPE_CHECK_OK(helper_.Open(cc));
  const auto& options = cc->Options<RoisToSpriteListCalculatorOptions>();
  if (!options.sticker_is_premultiplied()) {
    MEDIAPIPE_CHECK_OK(GlSetup());
    return helper_.RunInGlContext([this]() -> absl::Status {
      glDisable(GL_BLEND);
      const GpuBuffer& sticker_buffer = sticker_packet_.Get<GpuBuffer>();
      GlTexture image = helper_.CreateSourceTexture(sticker_buffer);

      // All the render passes are going to draw to the whole buffer, so we can
      // just use the same VAO throughout.
      glBindVertexArray(vao_);

      GlTexture result = RenderPremultiply(image);

      glFlush();
      glBindVertexArray(0);

      auto output_buffer = result.GetFrame<GpuBuffer>();
      sticker_packet_ = mediapipe::Adopt(output_buffer.release());
      return absl::OkStatus();
    });
  }
#endif  // !MEDIAPIPE_DISABLE_GPU

  return absl::OkStatus();
}

absl::Status RoisToSpriteListCalculator::Process(CalculatorContext* cc) {
  RET_CHECK(cc != nullptr) << "CalculatorContext is nullptr.";
  auto sprite_list = std::make_unique<SpriteList>();

  if (cc->Inputs().Tag(kNormalizedRectsTag).IsEmpty()) {
    cc->Outputs()
        .Tag(kSpriteListTag)
        .Add(sprite_list.release(), cc->InputTimestamp());
    return absl::OkStatus();
  }

  const Size& bg_size = cc->Inputs().Tag(kImageSizeTag).Get<Size>();

  bool use_gpu = cc->InputSidePackets().HasTag(kStickerImageGpuTag);
  Size sticker_size;
  if (use_gpu) {
#if !defined(MEDIAPIPE_DISABLE_GPU)
    auto& sticker = sticker_packet_.Get<GpuBuffer>();
    sticker_size = Size(sticker.width(), sticker.height());
#endif  // !MEDIAPIPE_DISABLE_GPU
  } else {
    auto& sticker = sticker_packet_.Get<ImageFrame>();
    sticker_size = Size(sticker.Width(), sticker.Height());
  }

  NormalizedRects norm_rects =
      cc->Inputs().Tag(kNormalizedRectsTag).Get<NormalizedRects>();
  for (const NormalizedRect& roi : norm_rects) {
    SpritePose pose;
    pose.set_position_x(roi.x_center());
    pose.set_position_y(roi.y_center());
    pose.set_rotation_radians(roi.rotation());
    pose.set_scale(sticker_zoom_ * FindFitZoom(bg_size, sticker_size, roi));
    SpriteListElement element(sticker_packet_, pose);
    sprite_list->push_back(element);
  }

  cc->Outputs()
      .Tag(kSpriteListTag)
      .Add(sprite_list.release(), cc->InputTimestamp());
  return absl::OkStatus();
}

absl::Status RoisToSpriteListCalculator::Close(CalculatorContext* cc) {
#if !defined(MEDIAPIPE_DISABLE_GPU)
  helper_.RunInGlContext([this] {
    if (premultiply_program_) {
      glDeleteProgram(premultiply_program_);
      premultiply_program_ = 0;
    }
    if (vao_) {
      glDeleteVertexArrays(1, &vao_);
      vao_ = 0;
    }
  });
#endif  // !MEDIAPIPE_DISABLE_GPU
  return absl::OkStatus();
}

// static
absl::Status RoisToSpriteListCalculator::PremultiplyAlphaCpu(
    cv::Mat& sticker_rgba) {
  cv::Mat ones(sticker_rgba.rows, sticker_rgba.cols, CV_8UC1, cv::Scalar(255));

  // Multiplier = (a, a, a, 1).
  cv::Mat multiplier(sticker_rgba.rows, sticker_rgba.cols, CV_8UC4);
  cv::mixChannels(std::vector{sticker_rgba, ones}, {multiplier},
                  {3, 0, 3, 1, 3, 2, 4, 3});

  cv::multiply(sticker_rgba, multiplier, sticker_rgba, 1 / 255.0);
  return absl::OkStatus();
}

// static
float RoisToSpriteListCalculator::FindFitZoom(const Size& bg_size,
                                              const Size& sticker_size,
                                              const NormalizedRect& roi) {
  float roi_actual_width = roi.width() * bg_size.first;
  float roi_actual_height = roi.height() * bg_size.second;

  float min_scale_width = roi_actual_width / sticker_size.first;
  float min_scale_height = roi_actual_height / sticker_size.second;
  return std::max(min_scale_width, min_scale_height);
}

#if !defined(MEDIAPIPE_DISABLE_GPU)
absl::Status RoisToSpriteListCalculator::GlSetup() {
  const GLint attr_location[kNumAttributes] = {
      kAttributeVertex,
      kAttributeTexturePosition,
  };
  const GLchar* attr_name[kNumAttributes] = {
      "position",
      "texture_coordinate",
  };

  const std::string premultiply_shader =
      std::string(mediapipe::kMediaPipeFragmentShaderPreamble) + R"(
  DEFAULT_PRECISION(highp, float)
  varying vec2 sample_coordinate;

  uniform sampler2D image;

  void main() {
    vec4 color = texture2D(image, sample_coordinate);
    gl_FragColor = vec4(vec3(color.rgb) * color.a, color.a);
  }
  )";
  mediapipe::GlhCreateProgram(mediapipe::kBasicVertexShader,
                            premultiply_shader.c_str(), kNumAttributes,
                            attr_name, attr_location, &premultiply_program_);
  RET_CHECK(premultiply_program_)
      << "Problem initializing the alpha premultiply program.";

  glUseProgram(premultiply_program_);
  glUniform1i(glGetUniformLocation(premultiply_program_, "image"), 0);

  glUseProgram(0);

  // Now that the shader has been prepared, we set up our VAO.
  // Generate vbos and vao.
  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

  GLuint vbo[2];
  glGenBuffers(2, vbo);

  // Vertex position coords
  glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
  glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat),
                mediapipe::kBasicSquareVertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(kAttributeVertex);
  glVertexAttribPointer(kAttributeVertex, 2, GL_FLOAT, 0, 0, nullptr);

  // Texture coords
  glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
  glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat),
                mediapipe::kBasicTextureVertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(kAttributeTexturePosition);
  glVertexAttribPointer(kAttributeTexturePosition, 2, GL_FLOAT, 0, 0, nullptr);

  // Unbind and free vbo handles
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glDeleteBuffers(2, vbo);

  return absl::OkStatus();
}

GlTexture RoisToSpriteListCalculator::RenderPremultiply(
    const GlTexture& image) {
  GlTexture result =
      helper_.CreateDestinationTexture(image.width(), image.height());
  helper_.BindFramebuffer(result);

  glUseProgram(premultiply_program_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(image.target(), image.name());

  // Do the actual rendering.
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  return result;
}
#endif  // !MEDIAPIPE_DISABLE_GPU

REGISTER_CALCULATOR(RoisToSpriteListCalculator);

}  // namespace magritte
