---
layout: default
title: Calculators
parent: Technical guide
nav_order: 4
---

# Overview of Magritte Calculators
{: .no_toc }

1. TOC
{:toc}
---

This page gives an overview of all Magritte calculators.


### BlendCalculator

A calculator that takes two ImageFrame input streams and blends them
according to a mask.

**Input streams:**

*   `FRAMES_BG`: An ImageFrame stream, containing a background image. The
  background and foreground image streams must be of the same dimension.
*   `FRAMES_FG`: An ImageFrame stream, containing a foreground image. The
  background and foreground image streams must be of the same dimension.
*   `MASK`: An ImageFrame stream, containing a mask in ImageFormat::VEC32F1
  format. This determines how the background and foreground images will be
  blended: 0 means using the background value, 255 means using the forground
  value, and intermediate value will result in the weigted average between
  the two.

**Output streams:**

*   `FRAMES`: An ImageFrame stream containing the result of the blending as
  described above.

**Example config:**

```proto
node {
  calculator: "BlendCalculator"
  input_stream: "FRAMES_BG:frames_bg"
  input_stream: "FRAMES_FG:frames_fg"
  input_stream: "MASK:mask"
  output_stream: "FRAMES:output_video"
}
```
**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/calculators/blend_calculator.h)

### DetectionListToDetectionsCalculator

A calculator that takes DetectionList and converts to std::vector<Detection>.

**Input streams:**

*   `DETECTION_LIST`: A DetectionList containing a list of detections.

**Output streams:**

*   `DETECTIONS`: An std::vector<Detection> containing the same data.

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/calculators/detection_list_to_detections_calculator.cc)

### DetectionTransformationCalculator

A calculator used to perform transformations on Detections, supports only
relative-bounding-box based detections for now.

**Input streams:**

*   `DETECTIONS`: Detections stream, containing detections in an image.
  detection.location_data is assumed as relative_bouding_box as only used
  like this for now.
*   `SIZE`: [Optional] Pair<int,int> containing original image size. Not used yet
  since we don't need it to rotate relative_bounding_box.

**Output streams:**

*   `DETECTIONS`: Detections stream, with bouding box and keypoints rotated.

**Example config:**

```proto
node {
  calculator: "DetectionTransformationCalculator"
  input_stream: "DETECTIONS:detections_rotated"
  input_stream: "SIZE:image_size"
  output_stream: "DETECTIONS:output_detections"
  node_options: {
    [type.googleapis.com/magritte.RotationCalculatorOptions] {
      rotation_mode: 2 #90 degree anti-clockwise rotation
    }
  }
}
```
**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/calculators/detection_transformation_calculator.h)

### NewCanvasCalculator

A calculator that creates a new image with uniform color (set in options)
using the type, dimensions and format of the input image.

**Input streams:**

*   `IMAGE or IMAGE_GPU`: An ImageFrame or GpuBuffer stream, containing the image
  dimensions and format.

**Output streams:**

*   `IMAGE or IMAGE_GPU`: An ImageFrame or GpuBuffer stream, containing the new
  canvas.

**Options (see [proto file](https://github.com/google/magritte/blob/master/magritte/calculators/new_canvas_calculator.proto) for details):**

*   color defining the new canvas color.
*   scaling information (see proto file for details).

**Example config:**

```proto
node {
  calculator: "NewCanvasCalculator"
  input_stream: "IMAGE:input_video"
  output_stream: "IMAGE:output_video"
  node_options: {
    [type.googleapis.com/magritte.NewCanvasCalculatorOptions] {
      color { r: 0 g: 0 b: 0 }
    }
  }
}
```
**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/calculators/new_canvas_calculator.h)

### PixelizationByRoiCalculatorGpuExperimental

A calculator that pixelizes an image.
The targets are given by regions of interest defined as NormalizeRects.
It first pixelizes the image to the number of pixels specified in parameter.
The pixelized image is then blended with the input image on the regions of
interest.

**Input streams:**

*   `IMAGE_GPU`: A GpuBuffer stream, containing the image to be pixelized.
*   `NORM_RECTS`: An std::vector<NormalizedRect> stream, containing the regions
  of interest to be pixelized.

**Output streams:**

*   `IMAGE_GPU`: A GpuBuffer stream, containing the pixelized image.

**Options:**

*   Pixelization options (see proto file for details).
*   Median filter options (see proto file for details).
*   TODO: Whether to pixelize the whole rectangle or only the
inscribed oval.

**Example config:**

```proto
node {
  calculator: "PixelizationByRoiCalculatorGpuExperimental"
  input_stream: "IMAGE_GPU:throttled_input_video"
  input_stream: "NORM_RECTS:rois"
  output_stream: "IMAGE_GPU:output_video"
  node_options: {
    [type.googleapis.com/magritte.PixelizationCalculatorOptions] {
      total_nb_pixels: 576
      # median_filter_enabled: false
      # median_filter_ksize: 5
      blend_method: PIXELIZATION
    }
  }
}
```
**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/calculators/pixelization_by_roi_calculator_experimental_gpu.cc)

### PixelizationByRoiCalculatorGpu

A calculator that pixelizes an image.
The targets are given by regions of interest defined as NormalizeRects.
It first pixelizes the image to the number of pixels specified in parameter.
The pixelized image is then blended with the input image on the regions of
interest.

**Input streams:**

*   `IMAGE_GPU`: A GpuBuffer stream, containing the image to be pixelized.
*   `NORM_RECTS`: An std::vector<NormalizedRect> stream, containing the regions
  of interest to be pixelized.

**Output streams:**

*   `IMAGE_GPU`: A GpuBuffer stream, containing the pixelized image.

**Options:**

*   Pixelization options (see proto file for details).
*   Median filter options (see proto file for details).
*   TODO: Whether to pixelize the whole rectangle or only the
inscribed oval.

**Example config:**

```proto
node {
  calculator: "PixelizationByRoiCalculatorGpu"
  input_stream: "IMAGE_GPU:throttled_input_video"
  input_stream: "NORM_RECTS:rois"
  output_stream: "IMAGE_GPU:output_video"
  node_options: {
    [type.googleapis.com/magritte.PixelizationCalculatorOptions] {
      total_nb_pixels: 576
      # median_filter_enabled: false
      # median_filter_ksize: 5
      blend_method: PIXELIZATION
    }
  }
}
```
**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/calculators/pixelization_by_roi_calculator_gpu.cc)

### PixelizationCalculatorCpu

A calculator that applies pixelization to the whole input image. The total
number of pixels that should have the same color after pixelization is given
as a parameter. The ignore_mask parameter is ignored.

**Input streams:**

*   `FRAMES`: An ImageFrame stream, containing the input images.

**Output streams:**

*   `FRAMES`: An ImageFrame stream, containing the pixelized images.

**Example config:**

```proto
node {
  calculator: "PixelizationCalculatorCpu"
  input_stream: "FRAMES:input_video"
  output_stream: "FRAMES:output_video"
  node_options: {
    [type.googleapis.com/magritte.PixelizationCalculatorOptions] {
      total_nb_pixels: 576
      blend_method: PIXELIZATION
    }
  }
}
```
**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/calculators/pixelization_calculator_cpu.cc)

### PixelizationCalculatorGpu

Apply pixelization to an input image. The target region is given by the mask.
It first pixelizes the image to the number of pixels specified in parameter.
The pixelized image is then blended with the input image; if no mask is
found, the calculator defaults to applying it on the whole image.

**Input streams:**

*   `MASK`: Target region to pixelize (GpuBuffer).
*   `IMAGE_GPU`: Image to pixelize (GpuBuffer).

**Output streams:**

*   `IMAGE_GPU`: Resulting pixelized image (GpuBuffer).

**Example config:**

```proto
node {
  calculator: "PixelizationCalculatorGpu"
  input_stream: "IMAGE_GPU:input_video"
  input_stream: "MASK:blur_mask_gpu"
  output_stream: "IMAGE_GPU:output_video"
  node_options: {
    [type.googleapis.com/magritte.PixelizationCalculatorOptions] {
      total_nb_pixels: 576
      ignore_mask: false # Debug option to apply to whole picture
      blend_method: PIXELIZATION
    }
  }
}
```
**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/calculators/pixelization_calculator_gpu.cc)

### RoisToSpriteListCalculator

A calculator that, given a list of regions of interest (ROIs) and a sticker
image, generates a SpriteList, to be used in SpriteCalculator{Cpu|Gpu}.

A SpritePose is generated for each ROI, with the corresponding center and
rotation. The sticker is then zoomed so to cover the entire ROI while
preserving aspect ratio. An extra default zoom given by the STICKER_ZOOM may
be applied to ensure that, e.g. stickers with transaparency indeed redact
the ROI.

**Input streams:**

*   `SIZE`: The backgroud image size as a std::pair<int, int>.
*   `NORM_RECTS`: The ROIs as a std::vector<NormalizedReect>.

**Output streams:**

*   `SPRITES`: the corresponding SpriteList.

**Input side packets:**

*   `STICKER_IMAGE_CPU or STICKER_IMAGE_GPU`: The sticker image as an ImageFrame
  or as a GpuBuffer.
*   `STICKER_ZOOM`: The sticker default zoom as a float.

**Options (see [proto file](https://github.com/google/magritte/blob/master/magritte/calculators/rois_to_sprite_list_calculator.proto) for details):**

*   sticker_is_premultiplied: If the sticker has transparency, whether it is
  premultiplied or straight. Default is false.

**Example config:**

```proto
node {
  calculator: "RoisToSpriteListCalculator"
  input_stream: "SIZE:image_size"
  input_stream: "NORM_RECTS:rois"
  input_side_packet: "STICKER_IMAGE_CPU:sticker_image"
  input_side_packet: "STICKER_ZOOM:sticker_zoom"
  output_stream: "SPRITES:sprites"
  node_options: {
    [type.googleapis.com/magritte.RoisToSpriteListCalculatorOptions] {
      sticker_is_premultiplied: false
    }
  }
}
```
**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/calculators/rois_to_sprite_list_calculator.h)

### RotationRoiCalculator

A calculator that, given an input image, creates a Region Of Interest (ROI)
consisting of a rotation of the whole image.

**Input streams:**

*   `(No tag required)`: A packet of any type, containing the timestamp.

**Output streams:**

*   `ROI`: a NormalizedRect stream, containing the rotated region of interest.

**Options:**

*   RotationMode to define the rotation angle.
*   (optional) clockwise/counter-clockwise rotation
  (default: counter-clockwise, equivalent to rotating the image clockwise).

**Example config:**

```proto
node {
  calculator: "RotationRoiCalculator"
  input_stream: "input_video"
  output_stream: "ROI:output_roi"
  node_options: {
    [type.googleapis.com/magritte.RotationCalculatorOptions] {
      rotation_mode: ROTATION_90
    }
  }
}
```
**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/calculators/rotation_roi_calculator.h)

### SimpleBlurCalculatorCpu

A calculator that applies box blurring or Gaussian blurring on an image. The
type of blurring is configured via the calculator options.

**Input streams:**

*   `FRAMES`: An ImageFrame stream, containing an input image.
*   `DETECTIONS`: A vector of detections, containing the detections to be blurred
  onto the image from the first stream.  The type is vector<Detection>.

**Output streams:**

*   `FRAMES`: An ImageFrame stream, containing the blurred images.

**Example config:**

```proto
node {
  calculator: "SimpleBlurCalculatorCpu"
  input_stream: "FRAMES:input_video"
  input_stream: "DETECTIONS:tracked_detections"
  output_stream: "FRAMES:output_video"
  node_options: {
    [type.googleapis.com/magritte.SimpleBlurCalculatorOptions] {
      blur_type: GAUSSIAN_BLUR
    }
  }
}
```
**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/calculators/simple_blur_calculator_cpu.cc)

### SpriteCalculatorCpu

Stamps the given textures onto the background image after transforming by the
given vertex position matrices.

**Input streams:**

*   `IMAGE`: The input ImageFrame video frame to be overlaid with the sprites.
  If it has transparency, it is assumed to be premultiplied.
*   `SPRITES`: A vector of pairs of sprite images as ImageFrames and vertex
  transformations as SpritePoses to be stamped onto the input video
  (see sprite_list.h). The ImageFrame must have a premultiplied alpha
  channel.

**Output streams:**

*   `IMAGE`: The output image with the sprites addded. If the input background
  image has transparency, then the output will be premultiplied.

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/calculators/sprite_calculator_cpu.cc)

### SpriteCalculatorGpu

Stamps the given textures onto the background image after transforming by the
given vertex position matrices.

**Input streams:**

*   `IMAGE`: The input GpuBuffer video frame to be overlaid with the sprites.
  If it has transparency, it is assumed to be premultiplied.
*   `SPRITES`: A vector of pairs of sprite images as GpuBuffers and vertex
  transformations as SpritePoses to be stamped onto the input video
  (see sprite_list.h). The GpuBuffer must have a premultiplied alpha
  channel.

**Output streams:**

*   `IMAGE`: The output image with the sprites addded. If the input background
  image has transparency, then the output will be premultiplied.

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/calculators/sprite_calculator_gpu.cc)

