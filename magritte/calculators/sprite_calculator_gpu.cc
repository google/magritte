//
// Copyright 2016-2022 Google LLC
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
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/gpu/gl_calculator_helper.h"
#include "mediapipe/gpu/gl_simple_shaders.h"
#include "mediapipe/gpu/shader_util.h"
#include "magritte/calculators/sprite_list.h"
#include "magritte/calculators/sprite_pose.pb.h"

namespace magritte {
namespace {
enum { kAttributeVertex, kAttributeTexturePosition, kNumAttributes };
// Input/output stream tags.
constexpr char kGpuBufferTag[] = "IMAGE";
constexpr char kSpritesTag[] = "SPRITES";

using ::mediapipe::CalculatorBase;
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;
using ::mediapipe::GlCalculatorHelper;
using ::mediapipe::GlTexture;
using ::mediapipe::GpuBuffer;
}  // namespace

// Stamps the given textures onto the background image after transforming by the
// given vertex position matrices.
//
// Inputs:
// - IMAGE: The input GpuBuffer video frame to be overlaid with the sprites.
//   If it has transparency, it is assumed to be premultiplied.
// - SPRITES: A vector of pairs of sprite images as GpuBuffers and vertex
//   transformations as SpritePoses to be stamped onto the input video
//   (see sprite_list.h). The GpuBuffer must have a premultiplied alpha
//   channel.
//
// Outputs:
// - IMAGE: The output image with the sprites addded. If the input background
//   image has transparency, then the output will be premultiplied.
//
class SpriteCalculatorGpu : public CalculatorBase {
 public:
  SpriteCalculatorGpu() = default;
  ~SpriteCalculatorGpu() override = default;

  static absl::Status GetContract(CalculatorContract* cc);
  absl::Status Open(CalculatorContext* cc) override;
  absl::Status Process(CalculatorContext* cc) override;
  absl::Status Close(CalculatorContext* cc) override;

 private:
  GlCalculatorHelper helper_;

  // The GL handle for a shader program that simply copies the image.
  GLuint copy_program_ = 0;
  // Vertex attributes (NOTE: this assumes newer GL versions).
  GLuint vao_;
  GLuint vbo_[2];

  // Sets up the shader program and uniform locations.
  absl::Status GlSetup();

  // Applies a sprite transform to the corner vertices and reload vertex
  // attributes. Uses sprite_texture and bg_texture for aspect ratio correction.
  void ApplyTransformAndReloadVbo(const SpritePose sprite_pose,
                                  const GlTexture sprite_texture,
                                  const GlTexture bg_texture);

  // Draws the sprite on top of the background.
  GlTexture RenderSprite(const GlTexture& src, const GlTexture& sprite);
};

REGISTER_CALCULATOR(SpriteCalculatorGpu);

// static
absl::Status SpriteCalculatorGpu::GetContract(CalculatorContract* cc) {
  RET_CHECK(cc != nullptr) << "CalculatorContract is nullptr";
  RET_CHECK(cc->Inputs().HasTag(kGpuBufferTag))
      << "Missing input " << kGpuBufferTag << " tag.";
  cc->Inputs().Tag(kGpuBufferTag).Set<GpuBuffer>();

  RET_CHECK(cc->Inputs().HasTag(kSpritesTag))
      << "Missing input " << kSpritesTag << " tag.";
  cc->Inputs().Tag(kSpritesTag).Set<SpriteList>();

  RET_CHECK(cc->Outputs().HasTag(kGpuBufferTag))
      << "Missing output " << kGpuBufferTag << " tag.";
  cc->Outputs().Tag(kGpuBufferTag).Set<GpuBuffer>();

  return mediapipe::GlCalculatorHelper::UpdateContract(cc);
}

absl::Status SpriteCalculatorGpu::Open(CalculatorContext* cc) {
  MEDIAPIPE_CHECK_OK(helper_.Open(cc));
  MEDIAPIPE_CHECK_OK(GlSetup());
  return absl::OkStatus();
}

absl::Status SpriteCalculatorGpu::GlSetup() {
  const GLint attr_location[kNumAttributes] = {
      kAttributeVertex,
      kAttributeTexturePosition,
  };
  const GLchar* attr_name[kNumAttributes] = {
      "position",
      "texture_coordinate",
  };

  const std::string copy_shader =
      std::string(mediapipe::kMediaPipeFragmentShaderPreamble) + R"(
  DEFAULT_PRECISION(highp, float)

  varying mediump vec2 sample_coordinate;
  uniform sampler2D image;

  void main() {
    gl_FragColor = texture2D(image, sample_coordinate);
  }
  )";
  mediapipe::GlhCreateProgram(mediapipe::kBasicVertexShader, copy_shader.c_str(),
                            kNumAttributes, attr_name, attr_location,
                            &copy_program_);

  RET_CHECK(copy_program_) << "Problem initializing the shader program.";
  glUseProgram(copy_program_);
  glUniform1i(glGetUniformLocation(copy_program_, "image"), 0);

  glUseProgram(0);

  // Set up our VAO and bind empty VBOs.
  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

  glGenBuffers(2, vbo_);

  // Linking vertex position coords
  glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
  glVertexAttribPointer(kAttributeVertex, 2, GL_FLOAT, 0, 0, nullptr);
  glEnableVertexAttribArray(kAttributeVertex);

  // Linking texture coords
  glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);
  glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat),
               mediapipe::kBasicTextureVertices, GL_STATIC_DRAW);
  glVertexAttribPointer(kAttributeTexturePosition, 2, GL_FLOAT, 0, 0, nullptr);
  glEnableVertexAttribArray(kAttributeTexturePosition);

  // Unbind vbo handles
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  return absl::OkStatus();
}

void SpriteCalculatorGpu::ApplyTransformAndReloadVbo(
    const SpritePose sprite_pose, const GlTexture sprite_texture,
    const GlTexture bg_texture) {
  // We transform the four corner vertices ((-1, -1), (-1, 1), (1, -1), (1, 1))
  // using sprite_pose. The sequence of transformations is:
  // 1. First we scale the bounding box using sprite_pose.scale() and the sprite
  //    size, so it is in an orthonormal space.
  // 2. Then we rotate the box by sprite_pose.rotation_radians(). The rotation
  //    by an angle θ is given by the formula
  //        (x', y') = (x * cos(θ) - y * sin(θ), x * sin(θ) + y * cos(θ)).
  // 3. We divide the box dimensions by the background size, so we are back to
  //    [-1, 1]² coordinates.
  // 4. Finally, we translate the box to
  //        (sprite_pose.position_x(), sprite_pose.position_y()),
  //    compensating for the GL viewport coordinates ranging from -1.0 to 1.0.
  //
  // We are using an optimized formula to avoid recomputig sin/cos and so the
  // reader does not have to build it back from the individual transformations.
  const float scaled_x = sprite_texture.width() * sprite_pose.scale();
  const float scaled_y = sprite_texture.height() * sprite_pose.scale();

  float coordinate_system_top_of_screen = 1.f;
#ifdef __APPLE__
  coordinate_system_top_of_screen = -1.f;
#endif
  const float rotation =
      sprite_pose.rotation_radians() * coordinate_system_top_of_screen;

  const float cos_angle = std::cos(rotation);
  const float sin_angle = std::sin(rotation);
  const float cosx = cos_angle * scaled_x;
  const float cosy = cos_angle * scaled_y;
  const float sinx = sin_angle * scaled_x;
  const float siny = sin_angle * scaled_y;

  const float bg_width = bg_texture.width();
  const float bg_height = bg_texture.height();
  // Compensating for the GL viewport coordinates ranging from -1.0 to 1.0.
  const float center_x = 2.f * (sprite_pose.position_x() - 0.5f);
  const float center_y =
      coordinate_system_top_of_screen * 2.f * (sprite_pose.position_y() - 0.5f);

  const GLfloat all_vertices[] = {
      (-cosx + siny)/bg_width + center_x, (-sinx - cosy)/bg_height + center_y,
      ( cosx + siny)/bg_width + center_x, ( sinx - cosy)/bg_height + center_y,
      (-cosx - siny)/bg_width + center_x, (-sinx + cosy)/bg_height + center_y,
      ( cosx - siny)/bg_width + center_x, ( sinx + cosy)/bg_height + center_y,
  };

  // Set up the vertex arrays.
  glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
  glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat), all_vertices,
               GL_STREAM_DRAW);

  // Unbind vbo handles
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GlTexture SpriteCalculatorGpu::RenderSprite(const GlTexture& src,
                                            const GlTexture& sprite) {
  // Treat the input background as output for optimization.
  helper_.BindFramebuffer(src);

  glUseProgram(copy_program_);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(sprite.target(), sprite.name());

  // Do the actual rendering.
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  return src;
}

absl::Status SpriteCalculatorGpu::Process(CalculatorContext* cc) {
  return helper_.RunInGlContext([this, cc]() -> absl::Status {
    if (cc->Inputs().Tag(kGpuBufferTag).Value().IsEmpty()) {
      return absl::OkStatus();
    }
    if (cc->Inputs().Tag(kSpritesTag).IsEmpty()) {
      cc->Outputs()
          .Tag(kGpuBufferTag)
          .AddPacket(cc->Inputs().Tag(kGpuBufferTag).Value());
    }

    GpuBuffer input_buffer = cc->Inputs().Tag(kGpuBufferTag).Get<GpuBuffer>();
    GlTexture src = helper_.CreateSourceTexture(input_buffer);

    glEnable(GL_BLEND);
    // Use GL_ONE as sprite is premultiplied.
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // All the render passes are going to draw to the whole buffer, so we can
    // just use the same VAO throughout.
    glBindVertexArray(vao_);

    // Render all the sprites.
    const auto& all_sprites = cc->Inputs().Tag(kSpritesTag).Get<SpriteList>();
    for (const auto& sprite : all_sprites) {
      GlTexture sprite_texture =
          helper_.CreateSourceTexture(sprite.image_packet.Get<GpuBuffer>());
      const SpritePose& sprite_pose = sprite.pose;

      ApplyTransformAndReloadVbo(sprite_pose, sprite_texture, src);
      src = RenderSprite(src, sprite_texture);
    }

    glDisable(GL_BLEND);
    glFlush();
    glBindVertexArray(0);

    auto output_buffer = src.GetFrame<GpuBuffer>();
    cc->Outputs()
        .Tag(kGpuBufferTag)
        .Add(output_buffer.release(), cc->InputTimestamp());
    return absl::OkStatus();
  });
  return absl::OkStatus();
}

absl::Status SpriteCalculatorGpu::Close(CalculatorContext* cc) {
  helper_.RunInGlContext([this] {
    if (copy_program_) {
      glDeleteProgram(copy_program_);
      copy_program_ = 0;
    }
    if (vao_) {
      glDeleteVertexArrays(1, &vao_);
      vao_ = 0;
    }
    if (vbo_[0]) {
      glDeleteBuffers(2, vbo_);
      vbo_[0] = 0;
      vbo_[1] = 0;
    }
  });
  return absl::OkStatus();
}
}  // namespace magritte
