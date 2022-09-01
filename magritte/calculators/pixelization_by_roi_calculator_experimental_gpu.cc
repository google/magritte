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
//   calculator: "PixelizationByRoiCalculatorGpuExperimental"
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
class PixelizationByRoiCalculatorGpuExperimental : public CalculatorBase {
 public:
  PixelizationByRoiCalculatorGpuExperimental() = default;
  ~PixelizationByRoiCalculatorGpuExperimental() override = default;

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

        std::pair<float, float> scaled_down_size =
            getScaledDownSize(roi.width() * image.width(),
                              roi.height() * image.height(), options);

        ReloadVboCrop(image, crop_center_x, crop_center_y, roi.width(),
                      roi.height(), roi.rotation());

        const int pixelized_width = static_cast<int>(scaled_down_size.first);
        const int pixelized_height = static_cast<int>(scaled_down_size.second);
        GlTexture image_pixelized =
            RenderPixelisation(image, pixelized_width, pixelized_height);

        if (options.median_filter_enabled()) {
          ReloadVboMedian();
          image_pixelized = RenderMedian(image_pixelized);
        }

        ReloadVboBlend(image, crop_center_x, crop_center_y, roi.width(),
                       roi.height(), roi.rotation());
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

    void main() {
      if (length(sample_coordinate - vec2(0.5, 0.5)) < 0.5)
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

  void ReloadVboCrop(const GlTexture& bg_texture, float center_x,
                     float center_y, float width, float height,
                     float rotation) {
    // Loading a custom position vertex, to be able to crop the region of
    // interest (a rotated rectangle). For that, we need to compute the
    // coordinates of the rectangles's corner in the target image, in a [0;1]²
    // space.

    // Default vertex position coords, as we are generating an entire new image
    glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mediapipe::kBasicSquareVertices),
                 mediapipe::kBasicSquareVertices, GL_STREAM_DRAW);

    // Texture coords

    float coordinate_system_top_of_screen = 1.f;
#ifdef __APPLE__
    coordinate_system_top_of_screen = -1.f;
#endif
    rotation *= coordinate_system_top_of_screen;

    // Pre-computing common operations
    const float cos_angle = std::cos(rotation);
    const float sin_angle = std::sin(rotation);
    const float cosx = cos_angle * width * bg_texture.width();
    const float cosy = cos_angle * height * bg_texture.height();
    const float sinx = sin_angle * width * bg_texture.width();
    const float siny = sin_angle * height * bg_texture.height();

    const float bg_width = bg_texture.width() * 2.0f;
    const float bg_height = bg_texture.height() * 2.0f;
    center_y *= coordinate_system_top_of_screen;

    const GLfloat textureVertices[] = {
        // clang-format off
        (-cosx + siny)/bg_width + center_x, (-sinx - cosy)/bg_height + center_y,
        ( cosx + siny)/bg_width + center_x, ( sinx - cosy)/bg_height + center_y,
        (-cosx - siny)/bg_width + center_x, (-sinx + cosy)/bg_height + center_y,
        ( cosx - siny)/bg_width + center_x, ( sinx + cosy)/bg_height + center_y,
        // clang-format on
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(textureVertices), textureVertices,
                 GL_STREAM_DRAW);

    // Unbind vbo handles
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  void ReloadVboMedian() {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mediapipe::kBasicSquareVertices),
                 mediapipe::kBasicSquareVertices, GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mediapipe::kBasicTextureVertices),
                 mediapipe::kBasicTextureVertices, GL_STREAM_DRAW);

    // Unbind vbo handles
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  void ReloadVboBlend(const GlTexture& bg_texture, float center_x,
                      float center_y, float width, float height,
                      float rotation) {
    // Loading a custom position vertex, to be able to blend the image directly
    // rotated in the target image.
    // For that, we need to compute the coordinates of the rectangles's corner
    // in the target image, in a [-1;1]² space.

    float coordinate_system_top_of_screen = 1.f;
#ifdef __APPLE__
    coordinate_system_top_of_screen = -1.f;
#endif

    rotation *= coordinate_system_top_of_screen;

    const float bg_width = bg_texture.width();
    const float bg_height = bg_texture.height();

    // Pre-computing common operations
    const float cos_angle = std::cos(rotation);
    const float sin_angle = std::sin(rotation);
    const float cosx = cos_angle * width * bg_width;
    const float cosy = cos_angle * height * bg_height;
    const float sinx = sin_angle * width * bg_width;
    const float siny = sin_angle * height * bg_height;

    // Compensating for the GL viewport coordinates ranging from -1.0 to 1.0.
    center_x = 2.f * (center_x - 0.5f);
    center_y = coordinate_system_top_of_screen * 2.f * (center_y - 0.5f);

    const GLfloat rectangleVertices[] = {
        // clang-format off
        (-cosx + siny)/bg_width + center_x, (-sinx - cosy)/bg_height + center_y,
        ( cosx + siny)/bg_width + center_x, ( sinx - cosy)/bg_height + center_y,
        (-cosx - siny)/bg_width + center_x, (-sinx + cosy)/bg_height + center_y,
        ( cosx - siny)/bg_width + center_x, ( sinx + cosy)/bg_height + center_y,
        // clang-format on
    };

    glBindBuffer(GL_ARRAY_BUFFER, vbo_[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices), rectangleVertices,
                 GL_STREAM_DRAW);

    // Default texture coords, we are "pasting" the whole image
    glBindBuffer(GL_ARRAY_BUFFER, vbo_[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mediapipe::kBasicTextureVertices),
                 mediapipe::kBasicTextureVertices, GL_STREAM_DRAW);

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

REGISTER_CALCULATOR(PixelizationByRoiCalculatorGpuExperimental);

}  // namespace magritte
