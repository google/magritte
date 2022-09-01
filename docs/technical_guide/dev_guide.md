---
layout: default
title: Developer's guide
parent: Technical guide
nav_order: 2
---

# Developer's Guide
{: .no_toc }

1. TOC
{:toc}
---

Here we describe technical aspects of the Magritte library directed to
developers who want to use it in their project in ways beyond the examples
covered by the [codelab](https://google.github.io/magritte/getting_started/codelab.html).

NOTE: We recommend reading the
[codelab](https://google.github.io/magritte/getting_started/codelab.html) first before
having a look at this page.

## Introduction

Generally, you use the Magritte library just as you would use MediaPipe. So it's
a good idea to have a look at the [MediaPipe documentation](https://google.github.io/mediapipe/)
first.

## Data Types and Formats

See the
[Magritte Concepts](https://google.github.io/magritte/technical_guide/concepts.html#important-data-concepts)
page for a description of the Data Concepts used in Magritte. Here we explain
which actual data types we use to handle those concepts.

### Frames

Frames represent still images, mostly frames from a video. There are two
fundamental data types for them:
[`ImageFrame`](https://github.com/google/mediapipe/blob/master/mediapipe/framework/formats/image_frame.h)
and [`GpuBuffer`](https://github.com/google/mediapipe/blob/master/mediapipe/gpu/gpu_buffer.h).

**`ImageFrame`** is an image data type which is conceptually similar to
[OpenCV](https://opencv.org)'s matrix type, into which it can be converted using
the functions from
[`image_frame_opencv.h`](https://github.com/google/mediapipe/blob/master/mediapipe/framework/formats/image_frame_opencv.h).
It is mostly used for images that will be treated by the CPU. Similar to OpenCV
matrices, it can hold images in a variety of formats that are defined in
[`image_formats.proto`](https://github.com/google/mediapipe/blob/master/mediapipe/framework/formats/image_format.proto).

**`GpuBuffer`** on the other hand is used for images that will be treated on the
GPU. They reference FrameBuffer objects from OpenGL 2.0 ES and are manipulated
using a
[GlCalculatorHelper](https://github.com/google/mediapipe/blob/master/mediapipe/gpu/gl_calculator_helper.h).

`ImageFrame`s and `GpuBuffer`s can be converted into each other, see
[below](#cpu-vs-gpu-treatment).

### Detections

For detections, we use the
[MediaPipe `Detection` format](https://github.com/google/mediapipe/blob/master/mediapipe/framework/formats/detection.proto),
which essentially consists of a location (specified mostly via bounding boxes)
within a frame and potentially some keypoints.

### Masks

To capture masks we just use `ImageFrame`s or `GpuBuffer`s. For `ImageFrame` we
typically use a grayscale image format such as `VEC32F1` (see
[`image_format.proto`](https://github.com/google/mediapipe/blob/master/mediapipe/framework/formats/image_format.proto)),
while for `GpuBuffer` we always read the first channel of the image (in case it
has more than one).

Strictly speaking, a mask doesn’t simply encode location information, but more,
since each pixel can take more than binary values (typically between `0` and
`255`, depending on the image format). A graph or calculator can decide how to
deal with that: If strict locations are required, then all pixels with a value
`> 0` can be treated as part of a region, and others not; while in other
situations we can treat the value as an input for e.g. "how much" of some
redaction is required. See the documentation of individual graphs or calculators
for information how they treat masks.

## Magritte Graphs

See the
[Magritte Concepts](https://google.github.io/magritte/technical_guide/concepts.html) page
for an explanation of graph categories and features. Here we explain how to find
and use such graphs.

### Overview of existing Graphs

All Magritte graphs live in the
[`graphs` folder](https://github.com/google/magritte/blob/master/magritte/graphs) and its
subdirectories. The directory structure looks as follows:

```
graphs
├── detection
└── redaction
    ├── detection_to_mask
    └── mask_redaction
```

All top-level graphs live directly in the `graphs` folder. The subdirectories
contain feature subgraphs and correspond to the features described at
[Magritte Concepts](https://google.github.io/magritte/technical_guide/concepts.html#magritte-features).
The technical subgraphs mentioned there live next to the feature subgraphs that
use them, but they are not visible from outside packages.

The calculators that are part of the Magritte library live in the
[`calculators` folder](https://github.com/google/magritte/blob/master/magritte/calculators). Most
users shouldn't need to use them directly. They are part of many of the feature
subgraphs (together with calculators from the MediaPipe library).

If one of the existing top-level graphs fits your needs, you can just use it as
you would use any MediaPipe graph. Otherwise see the next section.

### Building your own Magritte Graphs

You can put together your own Magritte graphs from the existing feature
subgraphs. Have a look at the
[graph documentation page](https://google.github.io/magritte/technical_guide/graphs.html)
for an overview. As long as their inputs and outputs match up, you can put them
together in any way you want.

#### Defining a graph

Technically, a graph is just a fixed
[`CalculatorGraphConfig`](https://github.com/google/mediapipe/blob/master/mediapipe/framework/calculator.proto)
proto message. To build MediaPipe graphs, the most common way is to describe
such a message in a text proto file as explained in the
[MediaPipe documentation](https://google.github.io/mediapipe/framework_concepts/graphs.html),
and create a BUILD target for your graph. Since Magritte graphs are just
MediaPipe graphs, you can define them in just the same way. However, in the
Magritte library we use a custom BUILD macro that bundles many related targets
for convenience.

The `magritte_graph` BUILD macro is defined in
[`magritte_graph.bzl`](https://github.com/google/magritte/blob/master/magritte/magritte_graph.bzl).
You don't have to use it, but you probably should since it provides a number of
targets at once, such as a `cc_library` for the graph, an exported graph text
proto file and a binary proto. Please have a look at the documentation in the
source file for a complete list of targets as well as the parameters. To see
examples of its usage, you can look at any of the BUILD files in the
[`graphs` folder](https://github.com/google/magritte/blob/master/magritte/graphs) and its
subdirectories.

#### Loading a graph

Once you have defined your graph and its build target, you can use the
`MagritteGraphByName` method from
[`magritte_api_factory.h`](https://github.com/google/magritte/blob/master/magritte/api/magritte_api_factory.h)
or the `CreateByName` method from MediaPipe's
[`GraphRegistry` class](https://github.com/google/mediapipe/blob/master/mediapipe/framework/subgraph.h)
(the former works only if you set the `package` field in your graph proto to
"magritte"). In order for these methods
to find your graph, the code calling it must have a build dependency to your
graph's target.

Alternatively, you can use the binary proto
graph file that is created by the `magritte_graph` build macro and read it just
as any a serialized protocol buffer. See
[the protocol buffers tutorial](https://developers.google.com/protocol-buffers/docs/cpptutorial#reading-a-message)
for more information.
In any case, your code still must have a build dependency to your graph's
target.

## Optimizations

### CPU vs. GPU processing

Many of the graphs in the Magritte library exist in variants for CPU and GPU,
designated by a `gpu` or `cpu` in their name. If your platform doesn't have a
GPU, you cannot use the corresponding graphs on it. If you do have access to a
GPU, you can mix both GPU and CPU graphs, even though you should use the GPU
variants in most cases since they are usually faster. If you do want to mix
both, you can plug matching input and output streams together. The only occasion
where this is not possible directly is for streams that use `ImageFrame` or
`GpuBuffer`. To convert between these, you can use
[`ImageFrameToGpuBufferCalculator`](https://github.com/google/mediapipe/blob/master/mediapipe/gpu/image_frame_to_gpu_buffer_calculator.cc)
and
[`GpuBufferToImageFrameCalculator`](https://github.com/google/mediapipe/blob/master/mediapipe/gpu/gpu_buffer_to_image_frame_calculator.cc)
from MediaPipe.

Note: Those streams are usually tagged `IMAGE:` and `IMAGE_GPU:`, respectively.
If you have both, it's probably a sign that you are improperly mixing
ImageFrames and GpuBuffers.

### Live vs. offline treatment

For some graphs, there are variants optimized for live (real-time) or offline
treatment, designated by a `live` or `offline` in their name.

Our `live` graphs contain a
[`FlowLimiterCalculator`](https://github.com/google/mediapipe/blob/master/mediapipe/calculators/core/flow_limiter_calculator.cc)
that throttles the images flowing downstream for flow control. It passes through
the very first incoming image unaltered, and waits for the downstream
calculators in the graph to finish processing before it passes through another
image. All images that come in while waiting are dropped, limiting the number of
in-flight images between the `FlowLimiterCalculator` and later ones to 1. This
prevents the nodes in between from queuing up incoming images and data
excessively, which leads to increased latency and memory usage, unwanted in
real-time mobile applications. It also eliminates unnecessary computation.

## Resource folders

Magritte often needs certain files available at runtime (e.g., face
detection models). When you develop for Android or iOS, you don't need to worry
about this. However, on desktop environments you need to take some steps to
ensure that required files can be found.

### Files in scope

This concerns all files referenced in MediaPipe or Magritte graphs, for example
TensorFlow Lite ML models used in a
[face detection graph](https://github.com/google/mediapipe/blob/master/mediapipe/modules/face_detection/face_detection_full_range.pbtxt#L37)
or the sticker image file in the
[sticker redaction graph](https://github.com/google/magritte/blob/main/graphs/redaction/face_sticker_redaction_cpu.pbtxt#L49).

### MediaPipe's way of finding resource files

The way Magritte finds its runtime files is through MediaPipe's
[`resource_util` library](https://github.com/google/mediapipe/blob/master/mediapipe/util/resource_util.h).
Its implementation is platform-dependent. On desktop environments,
[by default](https://github.com/google/mediapipe/blob/master/mediapipe/util/resource_util_default.cc)
it interprets resource file paths relative to a resource root folder, which you
can provide with the `--resource_root_dir` command line flag. The default value
for this flag is the empty string, so that MediaPipe will look for resource
files in your working directory. If you want to change that, you have two
options:

1.  *Provide the `--resource_root_dir` flag:* You can create a resource folder,
    copy the necessary files to it with the correct relative paths, and provide
    its (absolute) path via the `--resource_root_dir` flag when running your
    binary. To spare you the work of creating such folders manually, there is a
    special build rule you can use for this. It is described
    [in the next subsection](#build-rule-for-resource-folders).
1.  *Change how MediaPipe looks for files:* You can include
    [`resource_util_custom.h`](https://github.com/google/mediapipe/blob/master/mediapipe/util/resource_util_custom.h),
    which provides a function `SetCustomGlobalResourceProvider`. This function
    allows you to override the way MediaPipe searches for resource files by your
    own implementation of a resource provider function.

### Build rule for resource folders

The file [`magritte_graph.bzl`](https://github.com/google/magritte/blob/main/magritte_graph.bzl)
defines two build rules: `magritte_runtime_data` and `magritte_resource_folder`.
The first one takes as its `deps` attribute a list of Magritte graphs. The
second one takes as its `runtime_data` attribute a target defined using the
first one.

The Magritte graphs you use in your project thus need to be referenced in two
places: in the `magritte_runtime_data` rule and as dependencies of your C++
target. It may be useful to introduce a variable in your BUILD file listing the
graphs you use. (The [Bazel style guide](https://bazel.build/rules/bzl-style)
discourages the use of variables, but we consider the risk of breaking automated
tooling around BUILD files to be lower than the risk of introducing
hard-to-debug errors by listing the graphs incorrectly.)

So let's assume your project uses two graphs,
`@magritte//magritte/graphs:face_pixelization_offline_cpu` and
`@magritte//magritte/graphs:face_sticker_redaction_offline_cpu`. Then you can
write your BUILD file as follows:

```python
_my_graphs = [
    "@magritte//magritte/graphs:face_pixelization_offline_cpu",
    "@magritte//magritte/graphs:face_sticker_redaction_offline_cpu",
]

cc_binary(  # Could also be a cc_library, depending on what you're doing.
    name = "my_project_main",
    deps = _my_graphs + [
        # List of other dependencies of your code that are not
        # Magritte graphs.
    ],
)

magritte_runtime_data(
    name = "runtime_data",
    deps = _my_graphs,
)

magritte_resources_folder(
    name = "my_resources_folder",
    runtime_data = ":runtime_data",
)
```

When you then build the `my_resources_folder` target, a folder will be created
that contains all files needed by the graphs in the right subfolders. The output
should look something like this:

```
$ bazel build //path/to/buildfile:my_resources_folder --experimental_repo_remote_exec
INFO: Analyzed target //path/to/buildfile:my_resources_folder (...).
INFO: Found 1 target...
Target //path/to/buildfile:my_resources_folder up-to-date:
  bazel-bin/path/to/buildfile/my_resources_folder/mediapipe/modules/face_detection/face_detection_full_range.tflite
  bazel-bin/path/to/buildfile/my_resources_folder/third_party/mediapipe/modules/face_detection/face_detection_full_range_sparse.tflite
[...]
INFO: Build completed successfully, ... total actions

```

From this you can see that the resource folder was created at
`bazel-bin/path/to/buildfile/my_resources_folder` (it will have the same name as
your target) and that it contains a few files in subfolders. When you run the
binary compiled from the `my_project_main` target, provide
`--resource_root_dir=/full/path/to/workspace/bazel-bin/path/to/buildfile/my_resources_folder`
and MediaPipe will find the files. You may also copy the folder to a more
reasonable location.
