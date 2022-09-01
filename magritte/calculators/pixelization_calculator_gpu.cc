//
// Copyright 2020-2021 Google LLC
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

// Apply pixelization to an input image. The target region is given by the mask.
// It first pixelizes the image to the number of pixels specified in parameter.
// The pixelized image is then blended with the input image; if no mask is
// found, the calculator defaults to applying it on the whole image.
//
// Inputs:
// - MASK: Target region to pixelize (GpuBuffer).
// - IMAGE_GPU: Image to pixelize (GpuBuffer).
//
// Outputs:
// - IMAGE_GPU: Resulting pixelized image (GpuBuffer).
//
// Example config:
// node {
//   calculator: "PixelizationCalculatorGpu"
//   input_stream: "IMAGE_GPU:input_video"
//   input_stream: "MASK:blur_mask_gpu"
//   output_stream: "IMAGE_GPU:output_video"
//   options: {
//     [magritte.PixelizationCalculatorOptions.ext] {
//       total_nb_pixels: 576
//       ignore_mask: false # Debug option to apply to whole picture
//       blend_method: PIXELIZATION
//     }
//   }
// }

#include <memory>
#include <utility>
#include <vector>

#include "mediapipe/framework/calculator_framework.h"
#include "magritte/calculators/pixelization_calculator.pb.h"
#include "mediapipe/gpu/gl_calculator_helper.h"
#include "mediapipe/gpu/gl_simple_shaders.h"
#include "mediapipe/gpu/shader_util.h"

namespace magritte {

constexpr char kMaskTag[] = "MASK";
constexpr char kImageGpuTag[] = "IMAGE_GPU";

using ::mediapipe::CalculatorBase;
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;
using ::mediapipe::GlCalculatorHelper;
using ::mediapipe::GlTexture;
using ::mediapipe::GpuBuffer;

enum { ATTRIB_VERTEX, ATTRIB_TEXTUREPOSITION, NUM_ATTRIBUTES };

class PixelizationCalculatorGpu : public CalculatorBase {
 public:
  PixelizationCalculatorGpu() = default;
  ~PixelizationCalculatorGpu() override = default;

  static absl::Status GetContract(CalculatorContract* cc) {
    const auto& options = cc->Options<PixelizationCalculatorOptions>();
    cc->Inputs().Tag(kImageGpuTag).Set<GpuBuffer>();
    cc->Inputs().Tag(kMaskTag).Set<GpuBuffer>();
    cc->Outputs().Tag(kImageGpuTag).Set<GpuBuffer>();

    // Check if Median filter options are set correctly
    RET_CHECK(!options.median_filter_enabled() ||
              options.median_filter_ksize() % 2 == 1 &&
                  options.median_filter_ksize() >= 3 &&
                  options.median_filter_ksize() <= 11)
        << "ksize option is expected to be positive odd number with "
           "3 <= ksize <= 11";
    return GlCalculatorHelper::UpdateContract(cc);
  }

  absl::Status Open(CalculatorContext* cc) override {
    MEDIAPIPE_CHECK_OK(helper_.Open(cc));
    const auto& options = cc->Options<PixelizationCalculatorOptions>();
    MEDIAPIPE_CHECK_OK(GlSetup(options));
    return absl::OkStatus();
  }

  absl::Status Process(CalculatorContext* cc) override {
    return helper_.RunInGlContext([this, cc]() -> absl::Status {
      const auto& options = cc->Options<PixelizationCalculatorOptions>();
      glDisable(GL_BLEND);

      if (cc->Inputs().Tag(kImageGpuTag).Value().IsEmpty()) {
        LOG(INFO) << "Empty Image at " << cc->InputTimestamp();
        return absl::OkStatus();
      }

      GpuBuffer image_buffer = cc->Inputs().Tag(kImageGpuTag).Get<GpuBuffer>();
      GlTexture image = helper_.CreateSourceTexture(image_buffer);

      // All the render passes are going to draw to the whole buffer, so we can
      // just use the same VAO throughout.
      glBindVertexArray(vao_);

      std::pair<float, float> scaled_down_size =
          getScaledDownSize(image.width(), image.height(), options);
      const int pixelized_width = static_cast<int>(scaled_down_size.first);
      const int pixelized_height = static_cast<int>(scaled_down_size.second);
      GlTexture image_pixelized =
          RenderPixelisation(image, pixelized_width, pixelized_height);

      if (options.median_filter_enabled()) {
        image_pixelized = RenderMedian(image_pixelized);
      }
      GlTexture result;
      if (cc->Inputs().Tag(kMaskTag).Value().IsEmpty() ||
          options.ignore_mask()) {
        LOG(INFO) << "Empty Mask at " << cc->InputTimestamp();

        // If there is no mask, return the picture fully pixelized
        result = image_pixelized;
      } else {
        GpuBuffer mask_buffer = cc->Inputs().Tag(kMaskTag).Get<GpuBuffer>();
        GlTexture mask = helper_.CreateSourceTexture(mask_buffer);

        result = RenderBlend(image, mask, image_pixelized, options);
      }

      glFlush();
      glBindVertexArray(0);

      auto output = result.GetFrame<GpuBuffer>();

      cc->Outputs()
          .Tag(kImageGpuTag)
          .Add(output.release(), cc->InputTimestamp());
      return absl::OkStatus();
    });
  }

  static std::pair<float, float> getScaledDownSize(
      float width, float height,
      const PixelizationCalculatorOptions& options) {
    float x, y;
    if (options.has_max_resolution()) {
      const float max_side = options.max_resolution();
      if (width > height) {
        x = max_side;
        y = x * height / width;
      } else {
        y = max_side;
        x = y * width / height;
      }
    } else {
      // Computes x and y to keep the subdivisions square
      // with x*y = total_pixels
      const float total_pixels = options.total_nb_pixels();
      x = sqrt(total_pixels * (float)width / (float)height);
      y = sqrt(total_pixels * (float)height / (float)width);
    }
    return std::make_pair(x, y);
  }

  absl::Status Close(CalculatorContext* cc) override {
    helper_.RunInGlContext([this] {
      if (copy_program_) {
        glDeleteProgram(copy_program_);
        copy_program_ = 0;
      }
      if (median_program_) {
        glDeleteProgram(median_program_);
        median_program_ = 0;
      }
      if (blend_program_) {
        glDeleteProgram(blend_program_);
        blend_program_ = 0;
      }
      if (vao_) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
      }
    });
    return absl::OkStatus();
  }

 private:
  GlCalculatorHelper helper_;
  GLuint copy_program_ = 0;
  GLuint median_program_ = 0;
  GLuint blend_program_ = 0;

  // vertex attributes
  // NOTE: This assumes newer GL versions
  GLuint vao_;

  absl::Status GlSetup(PixelizationCalculatorOptions options) {
    const GLint attr_location[NUM_ATTRIBUTES] = {
        ATTRIB_VERTEX,
        ATTRIB_TEXTUREPOSITION,
    };
    const GLchar* attr_name[NUM_ATTRIBUTES] = {
        "position",
        "texture_coordinate",
    };

    const std::string copy_shader =
        std::string(mediapipe::kMediaPipeFragmentShaderPreamble) + R"(
    DEFAULT_PRECISION(highp, float)
    varying vec2 sample_coordinate;

    uniform sampler2D image;

    void main() {
      gl_FragColor = texture2D(image, sample_coordinate);
    }
    )";
    mediapipe::GlhCreateProgram(mediapipe::kBasicVertexShader, copy_shader.c_str(),
                              NUM_ATTRIBUTES, attr_name, attr_location,
                              &copy_program_);
    RET_CHECK(copy_program_)
        << "Problem initializing the pixelisation program.";

    glUseProgram(copy_program_);
    glUniform1i(glGetUniformLocation(copy_program_, "image"), 0);

    // Median filter
    if (options.median_filter_enabled()) {
      const std::string median_shader = absl::Substitute(
          R"($0
      DEFAULT_PRECISION(highp, float)
      varying vec2 sample_coordinate;

      uniform sampler2D image;
      uniform vec2 inv_texture_size;

      // ksize should be positive odd number only
      const int ksize = $1;
      const int ksize_square = ksize*ksize;
      // Sorts in place and for each channel, the values of two pixels
      //    (.4, .3, .2, 1.0), (.1, .2, .3, 1.0)
      // -> (.1, .2, .2, 1.0), (.4, .3, .3, 1.0)
      #define sort2(a, b) vec4 t=min(a,b); b=max(a,b); a=t;

      void main() {
        vec4 pixels[ksize_square];

        // Read mask
        for(int dX = -ksize/2; dX <= ksize/2; ++dX) {
          for(int dY = -ksize/2; dY <= ksize/2; ++dY) {
            pixels[(dX+ksize/2)*ksize+(dY+ksize/2)] = texture2D(image, sample_coordinate + vec2(dX,dY) * inv_texture_size);
          }
        }

        // Partial bubble sort
        // We run half the rounds of a per-channel bubble sort, ensurring the
        // second part of the array being at its final position, including the
        // median at position pixels[ksize_square/2].
        for(int i=0; i<ksize_square/2+1; ++i) {
          for(int j=0; j<ksize_square-1-i; ++j) {
            sort2(pixels[j], pixels[j+1])
          }
        }

        // Median
        gl_FragColor = pixels[ksize_square/2];
      }
      )",
          mediapipe::kMediaPipeFragmentShaderPreamble,
          options.median_filter_ksize());
      mediapipe::GlhCreateProgram(mediapipe::kBasicVertexShader,
                                median_shader.c_str(), NUM_ATTRIBUTES,
                                attr_name, attr_location, &median_program_);
      RET_CHECK(median_program_)
          << "Problem initializing the median filter program.";

      glUseProgram(median_program_);
      glUniform1i(glGetUniformLocation(median_program_, "image"), 0);
    }

    // Blend shader
    const std::string blend_shader =
        std::string(mediapipe::kMediaPipeFragmentShaderPreamble) + R"(
    DEFAULT_PRECISION(highp, float)
    varying vec2 sample_coordinate;

    uniform sampler2D image;
    uniform sampler2D mask;
    uniform sampler2D blur;

    void main() {
      float alpha = texture2D(mask, sample_coordinate).r;
      vec4 c = texture2D(image, sample_coordinate);
      vec4 b = texture2D(blur, sample_coordinate);
      gl_FragColor = (1.0 - alpha) * c + alpha * b;
    }
    )";
    mediapipe::GlhCreateProgram(mediapipe::kBasicVertexShader, blend_shader.c_str(),
                              NUM_ATTRIBUTES, attr_name, attr_location,
                              &blend_program_);
    RET_CHECK(blend_program_) << "Problem initializing the blend program.";

    glUseProgram(blend_program_);
    glUniform1i(glGetUniformLocation(blend_program_, "image"), 0);
    glUniform1i(glGetUniformLocation(blend_program_, "mask"), 1);
    glUniform1i(glGetUniformLocation(blend_program_, "blur"), 2);

    glUseProgram(0);

    // Now all shaders have been prepared, we set up our VAO
    // Generate vbos and vao.
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    GLuint vbo[2];
    glGenBuffers(2, vbo);

    // Vertex position coords
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat),
                 mediapipe::kBasicSquareVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, nullptr);

    // Texture coords
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, 4 * 2 * sizeof(GLfloat),
                 mediapipe::kBasicTextureVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(ATTRIB_TEXTUREPOSITION);
    glVertexAttribPointer(ATTRIB_TEXTUREPOSITION, 2, GL_FLOAT, 0, 0, nullptr);

    // Unbind and free vbo handles
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(2, vbo);

    return absl::OkStatus();
  }

  GlTexture RenderPixelisation(const GlTexture& image, int width, int height) {
    GlTexture result = helper_.CreateDestinationTexture(width, height);
    helper_.BindFramebuffer(result);

    glUseProgram(copy_program_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(image.target(), image.name());

    // Prevents texture to wrap around the opposite edge if the center of the
    // pixel is out of the frame
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    // Do the actual rendering.
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    return result;
  }

  GlTexture RenderMedian(const GlTexture& image) {
    GlTexture result =
        helper_.CreateDestinationTexture(image.width(), image.height());
    helper_.BindFramebuffer(result);

    glUseProgram(median_program_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(image.target(), image.name());

    glUniform2f(glGetUniformLocation(median_program_, "inv_texture_size"),
                1.0f / static_cast<float>(image.width()),
                1.0f / static_cast<float>(image.height()));

    // Prevents texture to wrap around the opposite edge if the mask is out of
    // the frame
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    // Do the actual rendering.
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    return result;
  }

  GlTexture RenderBlend(
      const GlTexture& image, const GlTexture& mask, const GlTexture& blur,
      const PixelizationCalculatorOptions& options) {
    GlTexture result =
        helper_.CreateDestinationTexture(image.width(), image.height());
    helper_.BindFramebuffer(result);

    glUseProgram(blend_program_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(image.target(), image.name());

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(mask.target(), mask.name());

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(blur.target(), blur.name());
    switch (options.blend_method()) {
      case PixelizationCalculatorOptions::DEFAULT:
      case PixelizationCalculatorOptions::PIXELIZATION:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        break;
      case PixelizationCalculatorOptions::LINEAR_INTERPOLATION:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        break;
      default:
        LOG(ERROR) << "Blending type not supported";
    }
    // Do the actual rendering.
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    return result;
  }
};

REGISTER_CALCULATOR(PixelizationCalculatorGpu);

}  // namespace magritte
