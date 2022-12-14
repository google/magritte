---
layout: default
title: Graphs
parent: Technical guide
nav_order: 3
---

# Overview of Magritte Graphs
{: .no_toc }

1. TOC
{:toc}
---

This page gives an overview of all Magritte top-level and feature graphs. See the [concepts page](https://google.github.io/magritte/technical_guide/concepts.html) for an explanation of this terminology.


## Top-level graphs

#### FaceBlurWithTrackingLiveCpu

A graph that detects/tracks faces and blurs them using simple box blur.

Note that simple blurring is not an effective de-identification method!

The face detection supports all orientations and both short and full ranges.

Once detected, moving faces are tracked with mediapipe object tracking.

This graph is specialized for CPU architectures and live environments,
tracking movements at a fixed frame rate of 5 fps.

**Input streams:**

*   `input_video`: An ImageFrame stream containing the image on which detection
  models are run.

**Output streams:**

*   `output_video`: An ImageFrame stream containing the blurred image.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs:face_blur_with_tracking_live_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs:face_blur_with_tracking_live_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs:face_blur_with_tracking_live_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/face_blur_with_tracking_live_cpu.pbtxt)

#### FaceBlurWithTrackingOfflineCpu

A graph that detects/tracks faces and blurs them using simple box blur.

Note that simple blurring is not an effective de-identification method!

The face detection supports all orientations and both short and full ranges.

Once detected, moving faces are tracked with mediapipe object tracking.

This graph is specialized for CPU architectures and offline environments,
tracking movements across all frames.

**Input streams:**

*   `input_video`: An ImageFrame stream containing the image on which detection
  models are run.

**Output streams:**

*   `output_video`: An ImageFrame stream containing the blurred image.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs:face_blur_with_tracking_offline_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs:face_blur_with_tracking_offline_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs:face_blur_with_tracking_offline_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/face_blur_with_tracking_offline_cpu.pbtxt)

#### FaceOverlayLiveGpu

A graph that detects faces and draws debug information.

It draws in red the raw detection output (detection box, keypoints, score),
and in green the redacted area outline.

This graph is specialized for GPU architectures. It also optimized for live
streaming environments by throttling the input stream and by applying only
full-range face detection.

**Input streams:**

*   `input_video`: A GpuBuffer stream containing the image on which detection
  models are run.

**Output streams:**

*   `output_video`: A GpuBuffer stream containing the image annotated with
  detection and redaction areas.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs:face_overlay_live_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs:face_overlay_live_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs:face_overlay_live_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/face_overlay_live_gpu.pbtxt)

#### FaceOverlayOfflineCpu

A graph that detects faces and draws debug information.

It draws in red the raw detection output (detection box, keypoints, score),
and in green the redacted area outline.

This graph is specialized for CPU architectures and offline environments
(no throttling is applied).

**Input streams:**

*   `input_video`: An ImageFrame stream containing the image on which detection
  models are run.

**Output streams:**

*   `output_video`: An ImageFrame stream containing the image annotated with
  detection and redaction areas.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs:face_overlay_offline_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs:face_overlay_offline_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs:face_overlay_offline_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/face_overlay_offline_cpu.pbtxt)

#### FacePixelizationLiveGpu

A graph that detects and redacts faces by pixelizing them.

This graph is specialized for GPU architectures. It also optimized for live
streaming environments by throttling the input stream and by applying only
full-range face detection.

**Input streams:**

*   `input_video`: The GpuBuffer stream containing the image to be redacted.

**Output streams:**

*   `output_video`: A GpuBuffer stream containing the redacted image.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs:face_pixelization_live_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs:face_pixelization_live_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs:face_pixelization_live_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/face_pixelization_live_gpu.pbtxt)

#### FacePixelizationOfflineCpu

A graph that detects and redacts faces by pixelizing them.

This graph is specialized for CPU architectures and offline environments
(no throttling is applied).

**Input streams:**

*   `input_video`: The ImageFrame stream containing the image to be redacted.

**Output streams:**

*   `output_video`: An ImageFrame stream containing the redacted image.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs:face_pixelization_offline_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs:face_pixelization_offline_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs:face_pixelization_offline_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/face_pixelization_offline_cpu.pbtxt)

#### FaceStickerRedactionLiveGpu

A graph that detects and redacts faces with an opaque "sticker" image.

The sticker image file is specified manually in this graph. We use one of the
files in the test_data directory.

This graph is specialized for GPU architectures. It also optimized for live
streaming environments by throttling the input stream and by applying only
full-range face detection.

**Input streams:**

*   `input_video`: The GpuBuffer stream containing the image to be redacted.

**Output streams:**

*   `output_video`: A GpuBuffer stream containing the redacted image.

**Input side packets:**

*   `sticker_image`: The file path for the sticker image in the PNG format. If not
  provided, defaults to
  [emoji.png](https://github.com/google/magritte/blob/master/magritte/test_data/emoji.png).
*   `sticker_zoom`: The default sticker zoom as a float. This is a default extra
  zoom that is applied to the sticker once it covers the face detection
  bounding box. If not provided, the default value is 1.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs:face_sticker_redaction_live_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs:face_sticker_redaction_live_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs:face_sticker_redaction_live_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/face_sticker_redaction_live_gpu.pbtxt)

#### FaceStickerRedactionOfflineCpu

A graph that detects and redacts faces with an opaque "sticker" image.

The sticker image file is specified manually in this graph. We use one of the
files in the test_data directory.

**Input streams:**

*   `input_video`: The ImageFrame stream containing the image to be redacted.

**Output streams:**

*   `output_video`: An ImageFrame stream containing the redacted image.

**Input side packets:**

*   `sticker_image`: The file path for the sticker image in the PNG format. If not
  provided, defaults to
  [emoji.png](https://github.com/google/magritte/blob/master/magritte/test_data/emoji.png).
*   `sticker_zoom`: The default sticker zoom as a float. This is a default extra
  zoom that is applied to the sticker once it covers the face detection
  bounding box. If not provided, the default value is 1.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs:face_sticker_redaction_offline_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs:face_sticker_redaction_offline_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs:face_sticker_redaction_offline_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/face_sticker_redaction_offline_cpu.pbtxt)

#### FaceTrackingOverlayLiveGpu

A graph that detects and tracks faces to draw debug information.

It draws in red the raw detection output (detection box, keypoints, score),
and in blue the tracked detection output (detection box, keypoints, score,
when present).

This graph is specialized for GPU architectures. It also optimized for live
streaming environments by throttling the input stream and by applying only
full-range face detection.

**Input streams:**

*   `input_video`: A GpuBuffer stream containing the image on which detection
  models are run.

**Output streams:**

*   `output_video`: A GpuBuffer stream containing the image annotated with
  detections data.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs:face_tracking_overlay_live_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs:face_tracking_overlay_live_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs:face_tracking_overlay_live_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/face_tracking_overlay_live_gpu.pbtxt)

#### FaceTrackingOverlayOfflineCpu

A graph that detects and tracks faces to draw debug information.

It draws in red the raw detection output (detection box, keypoints, score),
and in blue the tracked detection output (detection box, keypoints, score,
when present).

This graph is specialized for CPU architectures and offline environments
(no throttling is applied).

**Input streams:**

*   `input_video`: An ImageFrame stream containing the image on which detection
  models are run.

**Output streams:**

*   `output_video`: An ImageFrame stream containing the image annotated with
  detections data.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs:face_tracking_overlay_offline_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs:face_tracking_overlay_offline_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs:face_tracking_overlay_offline_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/face_tracking_overlay_offline_cpu.pbtxt)

## Subgraphs

### Detection

#### FaceDetection360ShortAndFullRangeSubgraphCpu

A face detection subgraph that supports all orientations, and both short and
full ranges.

This subgraph utilizes a face detection method that only supports orientations
of up to +/- 45°. Thus the image is rotated 0°, 90°, 180° and 270°, the
detection method is applied in each rotated image, and the corresponding
detections are rotated back. Finally, a non-max suppression is applied to
remove duplicate detections.

**Input streams:**

*   `IMAGE`: The ImageFrame stream containing the image on which faces will be
  detected.

**Output streams:**

*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/detection:face_detection_360_short_and_full_range_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/detection:face_detection_360_short_and_full_range_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/detection:face_detection_360_short_and_full_range_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/detection/face_detection_360_short_and_full_range_cpu.pbtxt)

#### FaceDetection360ShortAndFullRangeSubgraphGpu

A face detection subraph that supports all orientations, and both short and
full ranges.

This subgraph utilizes a face detection method that only supports orientations
of up to +/- 45°. Thus the image is rotated 0°, 90°, 180° and 270°, the
detection method is applied in each rotated image, and the corresponding
detections are rotated back. Finally, a non-max suppression is applied to
remove duplicate detections.

**Input streams:**

*   `IMAGE`: The GpuBuffer stream containing the image on which faces will be
  detected.

**Output streams:**

*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/detection:face_detection_360_short_and_full_range_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/detection:face_detection_360_short_and_full_range_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/detection:face_detection_360_short_and_full_range_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/detection/face_detection_360_short_and_full_range_gpu.pbtxt)

#### FaceDetection360ShortRangeByRoiSubgraphCpu

A face detection subgraph that supports all orientations.

This subgraph only supports short-range detection, e.g. from a phone's front
camera.

This subgraph utilizes a face detection method that only supports orientations
of up to +/- 45°, but can take a Region of Interest (ROI) as a NormalizedRect,
on which the faces will be detected. Thus, a ROI including the whole image
is created for rotations of 0°, 90°, 180° and 270°, and the detection method
is applied in those ROIs. Finally, a non-max suppression is applied to remove
duplicate detections.

This subgraph is up to 5% more efficient than
FaceDetection360ShortAndFullRangeSubgraphCpu, though it does not support
full-range detection.

**Input streams:**

*   `IMAGE`: The ImageFrame stream containing the image on which faces will be
  detected.

**Output streams:**

*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/detection:face_detection_360_short_range_by_roi_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/detection:face_detection_360_short_range_by_roi_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/detection:face_detection_360_short_range_by_roi_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/detection/face_detection_360_short_range_by_roi_cpu.pbtxt)

#### FaceDetection360ShortRangeByRoiSubgraphGpu

A face detection subgraph that supports all orientations.

This subgraph only supports short-range detection, e.g. from a phone's front
camera.

This subgraph utilizes a face detection method that only supports orientations
of up to +/- 45°, but can take a Region of Interest (ROI) as a NormalizedRect,
on which the faces will be detected. Thus, a ROI including the whole image
is created for rotations of 0°, 90°, 180° and 270°, and the detection method
is applied in those ROIs. Finally, a non-max suppression is applied to remove
duplicate detections.

This subgraph is up to 5% more efficient than
FaceDetection360ShortAndFullRangeSubgraphGpu, though it does not support
full-range detection.

**Input streams:**

*   `IMAGE`: The GpuBuffer stream containing the image on which faces will be
  detected.

**Output streams:**

*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/detection:face_detection_360_short_range_by_roi_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/detection:face_detection_360_short_range_by_roi_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/detection:face_detection_360_short_range_by_roi_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/detection/face_detection_360_short_range_by_roi_gpu.pbtxt)

#### FaceDetectionFullRangeSubgraphCpu

A full-range face detection subgraph.

This subgraph only supports full-range detection, e.g. from a phone's back
camera.

This subgraph only supports orientations of up to +/- 45°.

**Input streams:**

*   `IMAGE`: The ImageFrame stream containing the image on which faces will be
  detected.

**Output streams:**

*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/detection:face_detection_full_range_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/detection:face_detection_full_range_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/detection:face_detection_full_range_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/detection/face_detection_full_range_cpu.pbtxt)

#### FaceDetectionFullRangeSubgraphGpu

A full-range face detection subgraph.

This subgraph only supports full-range detection, e.g. from a phone's back
camera.

This subgraph only supports orientations of up to +/- 45°.

**Input streams:**

*   `IMAGE`: The GpuBuffer stream containing the image on which faces will be
  detected.

**Output streams:**

*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/detection:face_detection_full_range_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/detection:face_detection_full_range_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/detection:face_detection_full_range_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/detection/face_detection_full_range_gpu.pbtxt)

#### FaceDetectionRotatedFullRangeSubgraphGpu

A full-range face detection subgraph, with an extra input stream for the
camera's orientation.

This subgraph utilizes a face detection method that only supports orientations
of up to +/- 45°. However, it uses the ROTATION_DEGREES input stream to
identify the camera's orientation. Thus the input image is rotated by that
amount, the detection method is applied in the rotated image, and the
resulting detections are rotated back.

This subgraph only supports full-range detection, e.g. from a phone's back
camera.

**Input streams:**

*   `IMAGE`: The GpuBuffer stream containing the image on which faces will be
  detected.
*   `ROTATION_DEGREES`: An int stream representing the camera's orientation in
  degrees, counter-clockwise. Only 0, 90, 180, 270 are considered valid
  inputs.

**Output streams:**

*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/detection:face_detection_rotated_full_range_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/detection:face_detection_rotated_full_range_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/detection:face_detection_rotated_full_range_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/detection/face_detection_rotated_full_range_gpu.pbtxt)

#### FaceDetectionShortAndFullRangeSubgraphCpu

A face detection subraph that supports both short and full ranges.

This subgraph only supports orientations of up to +/- 45°.

This subgraph applies separate short and full-range detections, and then
applies a non-max suppression to remove duplicate detections.

**Input streams:**

*   `IMAGE`: The ImageFrame stream containing the image on which faces will be
  detected.

**Output streams:**

*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/detection:face_detection_short_and_full_range_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/detection:face_detection_short_and_full_range_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/detection:face_detection_short_and_full_range_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/detection/face_detection_short_and_full_range_cpu.pbtxt)

#### FaceDetectionShortAndFullRangeSubgraphGpu

A face detection subraph that supports both short and full ranges.

This subgraph only supports orientations of up to +/- 45°.

This subgraph applies separate short and full-range detections, and then
applies a non-max suppression to remove duplicate detections.

**Input streams:**

*   `IMAGE`: The GpuBuffer stream containing the image on which faces will be
  detected.

**Output streams:**

*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/detection:face_detection_short_and_full_range_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/detection:face_detection_short_and_full_range_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/detection:face_detection_short_and_full_range_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/detection/face_detection_short_and_full_range_gpu.pbtxt)

#### FaceDetectionShortRangeSubgraphCpu

A short-range face detection subgraph.

This subgraph only supports short-range detection, e.g. from a phone's front
camera.

This subgraph only supports orientations of up to +/- 45°.

**Input streams:**

*   `IMAGE`: The ImageFrame stream containing the image on which faces will be
  detected.

**Output streams:**

*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/detection:face_detection_short_range_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/detection:face_detection_short_range_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/detection:face_detection_short_range_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/detection/face_detection_short_range_cpu.pbtxt)

#### FaceDetectionShortRangeSubgraphGpu

A short-range face detection subgraph.

This subgraph only supports short-range detection, e.g. from a phone's front
camera.

This subgraph only supports orientations of up to +/- 45°.

**Input streams:**

*   `IMAGE`: The GpuBuffer stream containing the image on which faces will be
  detected.

**Output streams:**

*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/detection:face_detection_short_range_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/detection:face_detection_short_range_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/detection:face_detection_short_range_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/detection/face_detection_short_range_gpu.pbtxt)

### Redaction

#### FaceDetectionToMaskSubgraphCpu

Converts face detections into a mask. The mask will have oval shapes at the
location of each face, which are rotated by the angle given by the position of
the eyes for each face detection. More precisely, this will take the bounding
box for each face detection, slightly enlarge it, rotate it so that its edges
are parallel and perpendicular to the line connecting the eyes, and inscribe
an oval into the resulting rectangle. The mask background will be black and
the ovals will be white.

**Input streams:**

*   `IMAGE`: An ImageFrame used to determine the size and format of the mask. The
  mask will have the same resolution as this, and use the same image format.
*   `DETECTIONS`: Face detections.

**Output streams:**

*   `MASK`: ImageFrame containing the created mask.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/redaction/detection_to_mask:face_detection_to_mask_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/redaction/detection_to_mask:face_detection_to_mask_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/redaction/detection_to_mask:face_detection_to_mask_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/redaction/detection_to_mask/face_detection_to_mask_cpu.pbtxt)

#### FaceDetectionToMaskSubgraphGpu

Converts face detections into a mask. The mask will have oval shapes at the
location of each face, which are rotated by the angle given by the position of
the eyes for each face detection. More precisely, this will take the bounding
box for each face detection, slightly enlarge it, rotate it so that its edges
are parallel and perpendicular to the line connecting the eyes, and inscribe
an oval into the resulting rectangle. The mask background will be black and
the ovals will be white.

**Input streams:**

*   `IMAGE`: A GpuBuffer used to determine the size and format of the mask. The
  mask will have the same resolution as this, and use the same image format.
*   `DETECTIONS`: Face detections.

**Output streams:**

*   `MASK`: GpuBuffer containing the created mask.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/redaction/detection_to_mask:face_detection_to_mask_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/redaction/detection_to_mask:face_detection_to_mask_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/redaction/detection_to_mask:face_detection_to_mask_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/redaction/detection_to_mask/face_detection_to_mask_gpu.pbtxt)

#### DetectionTrackingOverlaySubgraphCpu

Subgraph to draw debug information at the locations specified by two incoming
detection streams. For each detection stream, it will draw all data available
from detections, including the bounding box, keypoints and score.

**Input streams:**

*   `IMAGE`: An ImageFrame containing the image to draw the overlays on.
*   `[0]`: A std::vector<mediapipe::Detection>, will be rendered in red.
*   `[1]`: A std::vector<mediapipe::Detection>, will be rendered in blue.

**Output streams:**

*   `IMAGE`: The resulting image.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/redaction:detection_tracking_overlay_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/redaction:detection_tracking_overlay_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/redaction:detection_tracking_overlay_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/redaction/detection_tracking_overlay_cpu.pbtxt)

#### DetectionTrackingOverlaySubgraphGpu

Subgraph to draw debug information at the locations specified by two incoming
detection streams. For each detection stream, it will draw all data available
from detections, including the bounding box, keypoints and score.

**Input streams:**

*   `IMAGE_GPU`: A GpuBuffer containing the image to draw the overlays on.
*   `[0]`: A std::vector<mediapipe::Detection>, will be rendered in red.
*   `[1]`: A std::vector<mediapipe::Detection>, will be rendered in blue.

**Output streams:**

*   `IMAGE_GPU`: The resulting image.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/redaction:detection_tracking_overlay_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/redaction:detection_tracking_overlay_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/redaction:detection_tracking_overlay_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/redaction/detection_tracking_overlay_gpu.pbtxt)

#### FaceDetectionOverlaySubgraphCpu

Subgraph to draw debug information at the locations specified by incoming
detections. It will draw the detection bounding boxes as red un-filled
rectangles with keypoints and detection score. In addition, it will draw green
un-filled ovals around the area to be redacted.

**Input streams:**

*   `IMAGE`: An ImageFrame containing the image to draw the overlays on.
*   `DETECTIONS`: Face detections.

**Output streams:**

*   `IMAGE`: The resulting image.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/redaction:face_detection_overlay_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/redaction:face_detection_overlay_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/redaction:face_detection_overlay_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/redaction/face_detection_overlay_cpu.pbtxt)

#### FaceDetectionOverlaySubgraphGpu

Subgraph to draw debug information at the locations specified by incoming
detections. It will draw the detection bounding boxes as red un-filled
rectangles with keypoints and detection score. In addition, it will draw green
un-filled ovals around the area to be redacted.

**Input streams:**

*   `IMAGE`: A GpuBuffer containing the image to draw the overlays on.
*   `DETECTIONS`: Face detections.

**Output streams:**

*   `IMAGE`: The resulting image.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/redaction:face_detection_overlay_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/redaction:face_detection_overlay_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/redaction:face_detection_overlay_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/redaction/face_detection_overlay_gpu.pbtxt)

#### FacePixelizationSubgraphCpu

A subgraph that pixelizes faces.

This subgraph utilizes the mask pixelization: a mask is created based on the
face detections, which is then used to blend the input with a pixelized
version of the whole image.

**Input streams:**

*   `IMAGE`: An ImageFrame stream containing the image to be pixelized.
*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Output streams:**

*   `IMAGE`: An ImageFrame stream containing the pixelized image.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/redaction:face_pixelization_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/redaction:face_pixelization_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/redaction:face_pixelization_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/redaction/face_pixelization_cpu.pbtxt)

#### FacePixelizationSubgraphGpu

A subgraph that pixelizes faces.

This subgraph utilizes a calculator that blends the pixelized image only in
the redacted areas. This is about 3.7 times faster than using the mask
pixelization, but less general-purpose.

**Input streams:**

*   `IMAGE`: A GpuBuffer stream containing the image to be pixelized.
*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Output streams:**

*   `IMAGE`: A GpuBuffer stream containing the pixelized image.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/redaction:face_pixelization_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/redaction:face_pixelization_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/redaction:face_pixelization_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/redaction/face_pixelization_gpu.pbtxt)

#### FaceStickerRedactionSubgraphCpu

A subgraph that redacts faces with a custom "sticker" image.

The sticker is positioned and zoomed so that it covers the bounding box of
each detected face. A default extra zoom can be applied via an input side
packet, e.g. if the sticker has a transparent border.

**Input streams:**

*   `IMAGE`: The ImageFrame stream containing the image to be redacted.
*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Output streams:**

*   `IMAGE`: An ImageFrame stream containing the redacted image.

**Input side packets:**

*   `STICKER_PATH`: The path for the sticker image file. If not provided, defaults
  to [emoji.png](https://github.com/google/magritte/blob/master/magritte/test_data/emoji.png).
*   `STICKER_ZOOM`: The default extra sticker zoom as a float. If not provided,
  the default value is 1.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/redaction:face_sticker_redaction_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/redaction:face_sticker_redaction_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/redaction:face_sticker_redaction_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/redaction/face_sticker_redaction_cpu.pbtxt)

#### FaceStickerRedactionSubgraphGpu

A subgraph that redacts faces with a custom "sticker" image.

The sticker is positioned and zoomed so that it covers the bounding box of
each detected face. A default extra zoom can be applied via an input side
packet, e.g. if the sticker has a transparent border.

**Input streams:**

*   `IMAGE`: The GpuBuffer stream containing the image to be redacted.
*   `DETECTIONS`: A list of face detections as std::vector<mediapipe::Detection>.

**Output streams:**

*   `IMAGE`: A GpuBuffer stream containing the redacted image.

**Input side packets:**

*   `STICKER_PATH`: The path for the sticker image file. If not provided, defaults
  to [emoji.png](https://github.com/google/magritte/blob/master/magritte/test_data/emoji.png).
*   `STICKER_ZOOM`: The default extra sticker zoom as a float. If not provided,
  the default value is 1.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/redaction:face_sticker_redaction_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/redaction:face_sticker_redaction_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/redaction:face_sticker_redaction_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/redaction/face_sticker_redaction_gpu.pbtxt)

#### MaskPixelizationSubgraphCpu

Subgraph to pixelize a certain area of an image. The mask specifies which area
is to be pixelized: the parts where the mask is black will contain the
original image, the parts where the mask is white will contain the pixelized
image, and for values in-between blending will be used.

**Input streams:**

*   `IMAGE`: An ImageFrame containing the image to be pixelized.
*   `MASK`: An ImageFrame, containing a mask in ImageFormat::VEC32F1 format. It
  doesn't need to have the same resolution as the input image, if it has a
  different resolution it will be scaled using cv::INTER_NEAREST
  interpolation.

**Output streams:**

*   `IMAGE`: An ImageFrame containing the resulting image.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/redaction/mask_redaction:mask_pixelization_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/redaction/mask_redaction:mask_pixelization_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/redaction/mask_redaction:mask_pixelization_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/redaction/mask_redaction/mask_pixelization_cpu.pbtxt)

#### MaskPixelizationSubgraphGpu

Subgraph to pixelize a certain area of an image. The mask specifies which area
is to be pixelized: the parts where the mask is black will contain the
original image, the parts where the mask is white will contain the pixelized
image, and for values in-between blending will be used.

**Input streams:**

*   `IMAGE`: A GpuBuffer containing the image to be pixelized.
*   `MASK`: A GpuBuffer containing a mask. The image format of the mask doesn't
  matter, we always use the first channel as the alpha value for blending. It
  doesn't need to have the same resolution as the input image, if it has a
  different resolution it will be scaled using linear interpolation (in most
  cases, see SetStandardTextureParams in MediaPipe's GlContext class for
  details).

**Output streams:**

*   `IMAGE`: A GpuBuffer containing the resulting image.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/redaction/mask_redaction:mask_pixelization_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/redaction/mask_redaction:mask_pixelization_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/redaction/mask_redaction:mask_pixelization_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/redaction/mask_redaction/mask_pixelization_gpu.pbtxt)

### Tracking

#### SampledTrackingSubgraphCpu

A graph that performs motion tracking on detections, and in addition performs
sampling on the input video stream. It is similar to the tracking graphs
without sampling in this directory, and in addition it outputs a downsampled
version of the input video stream with 5 fps. This downsampled video stream
can be used for performing detections that are fed back into this graph.

**Input streams:**

*   `input_video`: The ImageFrame stream in which objects should be tracked. Its
  motion will be analyzed for the tracking.
*   `sampled_detections`: The detections to be tracked. They are expected to be
  calculated from the sampled_input_video output stream of this graph.

**Output streams:**

*   `tracked_detections`: Resulting tracked detections.
*   `sampled_input_video`: The input video stream downsampled to 5 fps.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/tracking:sampled_tracking_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/tracking:sampled_tracking_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/tracking:sampled_tracking_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/tracking/sampled_tracking_cpu.pbtxt)

#### SampledTrackingSubgraphGpu

A graph that performs motion tracking on detections, and in addition performs
sampling on the input video stream. It is similar to the tracking graphs
without sampling in this directory, and in addition it outputs a downsampled
version of the input video stream with 5 fps. This downsampled video stream
can be used for performing detections that are fed back into this graph.

**Input streams:**

*   `input_video`: The GpuBuffer stream in which objects should be tracked. Its
  motion will be analyzed for the tracking.
*   `sampled_detections`: The detections to be tracked. They are expected to be
  calculated from the sampled_input_video output stream of this graph.

**Output streams:**

*   `tracked_detections`: Resulting tracked detections.
*   `sampled_input_video`: The input video stream downsampled to 5 fps.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/tracking:sampled_tracking_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/tracking:sampled_tracking_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/tracking:sampled_tracking_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/tracking/sampled_tracking_gpu.pbtxt)

#### TrackingSubgraphCpu

A graph that performs motion tracking on detections. It's a wrapper around the
MediaPipe object tracking subgraph.

The input streams don't need to have the same timestamps. The detections
stream can be at a lower rate (e.g., obtained from a video that was
downsampled to a lower framerate). The output detection stream will be
generated for the same timestamps as the input video packets.

**Input streams:**

*   `input_video`: The ImageFrame stream in which objects should be tracked. Its
  motion will be analyzed for the tracking.
*   `detections`: The detections to be tracked.

**Output streams:**

*   `tracked_detections`: Resulting tracked detections.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/tracking:tracking_cpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/tracking:tracking_cpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/tracking:tracking_cpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/tracking/tracking_cpu.pbtxt)

#### TrackingSubgraphGpu

A graph that performs motion tracking on detections. It's a wrapper around the
MediaPipe object tracking subgraph.

The input streams don't need to have the same timestamps. The detections
stream can be at a lower rate (e.g., obtained from a video that was
downsampled to a lower framerate). The output detection stream will be
generated for the same timestamps as the input video packets.

**Input streams:**

*   `input_video`: The GpuBuffer stream in which objects should be tracked. Its
  motion will be analyzed for the tracking.
*   `detections`: The detections to be tracked.

**Output streams:**

*   `tracked_detections`: Resulting tracked detections.

**Build targets:**

*   Graph `cc_library`:

    ```
    @magritte//magritte/graphs/tracking:tracking_gpu
    ```
*   Text proto file:

    ```
    @magritte//magritte/graphs/tracking:tracking_gpu.pbtxt
    ```
*   Binary graph:

    ```
    @magritte//magritte/graphs/tracking:tracking_gpu_graph
    ```

**Code:** [source code](https://github.com/google/magritte/blob/master/magritte/graphs/tracking/tracking_gpu.pbtxt)

