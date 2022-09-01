//
// Copyright 2020-2022 Google LLC
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
#include <memory>
#include <utility>
#include <vector>

#include "mediapipe/framework/calculator_framework.h"
#include "magritte/calculators/pixelization_calculator.pb.h"
#include "mediapipe/framework/formats/rect.pb.h"
#include "mediapipe/gpu/gl_calculator_helper.h"
#include "mediapipe/gpu/gl_simple_shaders.h"
#include "mediapipe/gpu/shader_util.h"

namespace magritte {

namespace {
constexpr char kNormalizedRectsTag[] = "NORM_RECTS";
constexpr char kImageGpuTag[] = "IMAGE_GPU";

using ::mediapipe::CalculatorBase;
using ::mediapipe::CalculatorContext;
using ::mediapipe::CalculatorContract;
using ::mediapipe::GlCalculatorHelper;
using ::mediapipe::GlTexture;
using ::mediapipe::GpuBuffer;
using ::mediapipe::NormalizedRect;
using NormalizedRects = std::vector<NormalizedRect>;
}  // namespace

enum { ATTRIB_VERTEX, ATTRIB_TEXTUREPOSITION, NUM_ATTRIBUTES };

// A calculator that pixelizes an image.
// The targets are given by regions of interest defined as NormalizeRects.
// It first pixelizes the image to the number of pixels specified in parameter.
// The pixelized image is then blended with the input image on the regions of
// interest.
//
// Inputs:
// - IMAGE_GPU: A GpuBuffer stream, containing the image to be pixelized.
// - NORM_RECTS: An std::vector<NormalizedRect> stream, containing the regions
//   of interest to be pixelized.
//
// Outputs:
// - IMAGE_GPU: A GpuBuffer stream, containing the pixelized image.
//
// Options:
// - Pixelization options (see proto file for details).
// - Median filter options (see proto file for details).
// - TODO: Whether to pixelize the whole rectangle or only the
// inscribed oval.
//
// Example config:
// node {
//   calculator: "PixelizationByRoiCalculatorGpu"
//   input_stream: "IMAGE_GPU:throttled_input_video"
//   input_stream: "NORM_RECTS:rois"
//   output_stream: "IMAGE_GPU:output_video"
//   options: {
//     [magritte.PixelizationCalculatorOptions.ext] {
//       total_nb_pixels: 576
//       # median_filter_enabled: false
//       # median_filter_ksize: 5
//       blend_method: PIXELIZATION
//     }
//   }
// }
class PixelizationByRoiCalculatorGpu : public CalculatorBase {
 public:
  PixelizationByRoiCalculatorGpu() = default;
  ~PixelizationByRoiCalculatorGpu() override = default;

  static absl::Status GetContract(CalculatorContract* cc) {
    const auto& options = cc->Options<PixelizationCalculatorOptions>();
    cc->Inputs().Tag(kImageGpuTag).Set<GpuBuffer>();
    cc->Inputs().Tag(kNormalizedRectsTag).Set<NormalizedRects>();
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

      if (cc->Inputs().Tag(kNormalizedRectsTag).IsEmpty()) {
        cc->Outputs()
            .Tag(kImageGpuTag)
            .AddPacket(cc->Inputs().Tag(kImageGpuTag).Value());
        return absl::OkStatus();
      }

      GpuBuffer image_buffer = cc->Inputs().Tag(kImageGpuTag).Get<GpuBuffer>();
      GlTexture image = helper_.CreateSourceTexture(image_buffer);

      // All the render passes are going to draw to the whole buffer, so we can
      // just use the same VAO throughout.
      glBindVertexArray(vao_);

      NormalizedRects norm_rects =
          cc->Inputs().Tag(kNormalizedRectsTag).Get<NormalizedRects>();
      for (const NormalizedRect& roi : norm_rects) {
        // Formulas to get the bounding box of the rotated oval
        const float crop_center_x = roi.x_center();
        const float crop_center_y = roi.y_center();

        const float ux = roi.width() * image.width() * cos(roi.rotation());
        const float uy = roi.width() * image.width() * sin(roi.rotation());
        const float vx = roi.height() * image.height() * -sin(roi.rotation());
        const float vy = roi.height() * image.height() * cos(roi.rotation());

        std::pair<float, float> scaled_down_size =
            getScaledDownSize(image.width(), image.height(), options);

        // Add extra margin to avoid edge artifacts.
        const int median_ksize =
            options.median_filter_enabled() ? options.median_filter_ksize() : 1;
        const float extra_pixels = 2 * (0.5f + median_ksize / 2);
        const float pixel_width =
            std::ceil(image.width() / scaled_down_size.first);
        const float pixel_height =
            std::ceil(image.height() / scaled_down_size.second);

        const float crop_width = sqrt(ux * ux + vx * vx) / image.width() +
                                 extra_pixels * pixel_width / image.width();
        const float crop_height = sqrt(uy * uy + vy * vy) / image.height() +
                                  extra_pixels * pixel_height / image.height();

        ReloadVbo(crop_center_x, crop_center_y, crop_width, crop_height);

        const int pixelized_width = static_cast<int>(scaled_down_size.first);
        const int pixelized_height = static_cast<int>(scaled_down_size.second);
        GlTexture image_pixelized =
            RenderPixelisation(image, pixelized_width, pixelized_height);

        if (options.median_filter_enabled()) {
          image_pixelized = RenderMedian(image_pixelized);
        }
        image =
            RenderBlend(image, image_pixelized, roi.x_center(), roi.y_center(),
                        roi.width(), roi.height(), roi.rotation(), options);
      }

      glFlush();
      glBindVertexArray(0);

      auto output = image.GetFrame<GpuBuffer>();

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
      if (oval_blend_program_) {
        glDeleteProgram(oval_blend_program_);
        oval_blend_program_ = 0;
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

 private:
  GlCalculatorHelper helper_;
  GLuint copy_program_ = 0;
  GLuint median_program_ = 0;
  GLuint oval_blend_program_ = 0;

  // vertex attributes
  // NOTE: This assumes newer GL versions
  GLuint vao_;
  GLuint vbo_[2];

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
    const std::string oval_blend_shader =
        std::string(mediapipe::kMediaPipeFragmentShaderPreamble) + R"(
    DEFAULT_PRECISION(highp, float)
    varying vec2 sample_coordinate;

    uniform sampler2D blur;

    // center point of the ellipse
    uniform vec2 roi_center;

    // vec2(width, height) axis lengths of the ellipse
    uniform vec2 roi_radius;

    // vec2(width, height) resolution of the target image
    uniform vec2 frame_resolution;

    // matrix rotation used to rotate points from image to oval space
    // mat2(cos(θ), -sin(θ),
    //      sin(θ),  cos(θ))
    uniform mat2 rotation_matrix;

    void main() {
      vec2 diff = (sample_coordinate-roi_center);
      diff = rotation_matrix * (frame_resolution * diff) / frame_resolution;
      if (length(diff/roi_radius) < 1.0)
      {
        gl_FragColor = vec4(texture2D(blur, sample_coordinate).rgb, 1.0);
      }
      // gl_FragColor default is vec4(0.0), with is transparent if blend is enabled
    }
    )";
    mediapipe::GlhCreateProgram(mediapipe::kBasicVertexShader,
                              oval_blend_shader.c_str(), NUM_ATTRIBUTES,
                              attr_name, attr_location, &oval_blend_program_);
    RET_CHECK(oval_blend_program_) << "Problem initializing the blend program.";

    glUseProgram(oval_blend_program_);
    glUniform1i(glGetUniformLocation(oval_blend_program_, "blur"), 0);

    glUseProgram(0);

    // Now all shaders have been prepared, we set up our VAO and bind empty VBOs
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(2, vbo_);

    // Linking vertex position coords
    glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, nullptr);
    glEnableVertexAttribArray(ATTRIB_VERTEX);

    // Linking texture coords
    glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);
    glVertexAttribPointer(ATTRIB_TEXTUREPOSITION, 2, GL_FLOAT, 0, 0, nullptr);
    glEnableVertexAttribArray(ATTRIB_TEXTUREPOSITION);

    // Unbind vbo handles
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return absl::OkStatus();
  }

  void ReloadVbo(float center_x, float center_y, float width, float height) {
    // Vertex position coords
    // Converting [0;1]² space coordinates to [-1;1]²
    // Equivalent to (kBasicTextureVertices * 2.0f -1)
    const GLfloat kBasicSquareVertices[] = {
        // clang-format off
        center_x*2.0f - width - 1, center_y*2.0f - height - 1,  // bottom left
        center_x*2.0f + width - 1, center_y*2.0f - height - 1,  // bottom right
        center_x*2.0f - width - 1, center_y*2.0f + height - 1,  // top left
        center_x*2.0f + width - 1, center_y*2.0f + height - 1,  // top right
        // clang-format on
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kBasicSquareVertices),
                 kBasicSquareVertices, GL_STREAM_DRAW);

    // Texture coords
    const GLfloat kBasicTextureVertices[]{
        center_x - width / 2.0f, center_y - height / 2.0f,  // bottom left
        center_x + width / 2.0f, center_y - height / 2.0f,  // bottom right
        center_x - width / 2.0f, center_y + height / 2.0f,  // top left
        center_x + width / 2.0f, center_y + height / 2.0f,  // top right
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kBasicTextureVertices),
                 kBasicTextureVertices, GL_STREAM_DRAW);

    // Unbind vbo handles
    glBindBuffer(GL_ARRAY_BUFFER, 0);
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

  // Blends the blur texture onto the input image for the region of interrest.
  GlTexture RenderBlend(
      const GlTexture& image, const GlTexture& blur, float roi_center_x,
      float roi_center_y, float roi_width, float roi_height, float roi_rotation,
      const PixelizationCalculatorOptions& options) {
    helper_.BindFramebuffer(image);

    glUseProgram(oval_blend_program_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(blur.target(), blur.name());

    glUniform2f(glGetUniformLocation(oval_blend_program_, "roi_center"),
                roi_center_x, roi_center_y);
    glUniform2f(glGetUniformLocation(oval_blend_program_, "roi_radius"),
                roi_width / 2.0f, roi_height / 2.0f);
    glUniform2f(glGetUniformLocation(oval_blend_program_, "frame_resolution"),
                static_cast<float>(image.width()),
                static_cast<float>(image.height()));
    const GLfloat rotation_matrix[4] = {
        // clang-format off
        std::cos(roi_rotation), -std::sin(roi_rotation),
        std::sin(roi_rotation), std::cos(roi_rotation)};
    // clang-format on
    glUniformMatrix2fv(
        glGetUniformLocation(oval_blend_program_, "rotation_matrix"), 1,
        GL_FALSE, rotation_matrix);

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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Do the actual rendering.
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisable(GL_BLEND);
    return image;
  }
};

REGISTER_CALCULATOR(PixelizationByRoiCalculatorGpu);

}  // namespace magritte
