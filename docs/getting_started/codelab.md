---
layout: default
title: Codelab
parent: Getting started
nav_order: 2
---

# Magritte Codelab
{: .no_toc }

1. TOC
{:toc}
---

Welcome to the Magritte codelab! In this codelab you'll learn how to use
Magritte to write a simple command line tool to redact faces from an image file.
Later you'll also learn how to apply this to videos and how to customize the
redaction. For more advanced topics, the text will point you to other parts of
the documentation.

This codelab is written in C++. We will add codelabs for other languages later.

TODO: Add codelabs for other languages.

## Prerequisites

Please have a look at the
[installation instructions](https://google.github.io/magritte/getting_started/demos.html#installation)
to install necessary prerequisites.

### Bazel

This codelab assumes that you are familiar with [Bazel](https://bazel.build).
See the [Bazel C++ tutorial](https://bazel.build/tutorials/cpp) for an
introduction to using Bazel to build C++ projects.

We assume that you have a Bazel workspace set up (the tutorial linked above
explains how to do that). Your WORKSPACE needs to reference Magritte as an
external dependency, and since [Bazel doesn't support transitive external
dependencies](https://docs.bazel.build/versions/main/external.html#transitive-dependencies),
it also needs to list all of Magritte's dependencies. We recommend that you just
copy [Magritte's WORKSPACE file](https://github.com/google/magritte/blob/main/WORKSPACE.bazel),
change the workspace name, replace all occurrences of `@//` with `@magritte//`,
and add Magritte as a dependency by appending the following to the file:

```python
local_repository(
    name = "magritte",
    path = "/full/path/to/where/you/installed/magritte",
)
```

### Abseil

Magritte makes use of the [Abseil C++ library collection](https://abseil.io).
Most importantly it uses
[Abseil Status](https://abseil.io/docs/cpp/guides/status) for error handling. If
you're not familiar with Abseil Status, we recommend having a look.

We also use
[macros defined by MediaPipe](https://github.com/google/mediapipe/blob/master/mediapipe/framework/deps/status_macros.h)
for convenient handling of Abseil statuses. There are two of those,
`MP_RETURN_IF_ERROR` and `ASSIGN_OR_RETURN`, and we recommend having a look at
their documentation too.

## A command line tool to deidentify image files

Here you will learn step by step how to write a simple command line tool to
treat images. If you don't want to read the text, you can just have a look at
the
[final code](https://github.com/google/magritte/blob/master/magritte/examples/codelab/magritte_deidentify_image.cc).

### Introduction

Magritte is based on [MediaPipe](https://mediapipe.dev/), but
for now you only need the following two notions about MediaPipe:

Graph
:   An entity that defines how images or other types of content are treated. For
    example, a graph could define that images are rotated by 180 degrees, or
    some filter effect is applied -- or that faces are redacted in a certain
    way. For now all you need to know about graphs is that they exist. We will
    go into more detail later.

ImageFrame
:   A class defined by MediaPipe to deal with images. For practical purposes,
    you can think of it as a wrapper around
    [OpenCV matrices](https://docs.opencv.org/4.6.0/d3/d63/classcv_1_1Mat.html),
    a common image container class, although this is not the only way to use it.
    Its
    [source code](https://github.com/google/mediapipe/blob/master/mediapipe/framework/formats/image_frame.h)
    contains more information, and you can use the function in
    [image_frame_opencv.h](https://github.com/google/mediapipe/blob/master/mediapipe/framework/formats/image_frame_opencv.h)
    to convert between `ImageFrame`s and OpenCV matrices.

### First steps

Let's start by creating a simple C++ binary. In this example we will call many
functions that return `absl::Status` or similar, and to make it easier to deal
with those, we create a `Run` function returning `absl::Status` itself that we
call from the `main` function.

Create two new files in a Bazel workspace:

*   File `magritte_deidentify_image.cc`:

    ```c++
    #include "mediapipe/framework/port/logging.h"
    #include "absl/status/status.h"

    absl::Status Run() {

      return absl::OkStatus();
    }

    int main(int argc, char** argv) {
      google::InitGoogleLogging(argv[0]);
      absl::Status status = Run();
      return status.raw_code();
    }
    ```

*   File `BUILD`:

    ```python
    cc_binary(
        name = "magritte_deidentify_image",
        srcs = ["magritte_deidentify_image.cc"],
        deps = [
            "@mediapipe//mediapipe/framework/port:logging",
            "@com_google_absl//absl/status",
        ],
    )
    ```

We will now learn how to choose a graph, load it, and apply it to an
`ImageFrame`.

### Loading a graph

As explained before, images will be processed by a MediaPipe graph. Magritte
comes with ready-made graphs for you, and we will just use one of them. Later
you will learn how to make your own graphs.

Let's begin by choosing a graph. There are two main types of graphs in Magritte:
Top-level graphs and subgraphs. For treating an image, we need a top-level
graph. Subgraphs are meant for more specialized use cases and we will ignore
them for now (see the
[concepts page](https://google.github.io/magritte/technical_guide/concepts.html) for more
information). On the
[graphs page](https://google.github.io/magritte/technical_guide/graphs.html) you can see
an overview of all the graphs that Magritte defines. Head to that page and have
a look at the top-level graphs described there, and choose one that you want to
try out in this codelab. **Please choose a graph ending in `Cpu`, we will cover
the `Gpu` ones later.**

Let's assume you've chosen
[`FacePixelizationOfflineCpu`](https://google.github.io/magritte/technical_guide/graphs.html#facepixelizationofflinecpu).
Now let's load this graph in our binary.

At the end of the description of your graph on the
[graphs page](https://google.github.io/magritte/technical_guide/graphs.html) you will find
a list of build targets, one of which is called "Graph `cc_library`". Add this
target to the `deps` of your `cc_binary` in your `BUILD` file.

We can then call the `MagritteGraphByName` function from
[`magritte_api_factory.h`](https://github.com/google/magritte/blob/master/magritte/api/magritte_api_factory.h)
to obtain the graph with this name:

```c++
// Inside the Run() function:
ASSIGN_OR_RETURN(mediapipe::CalculatorGraphConfig graph_config,
                 magritte::MagritteGraphByName("FacePixelizationOfflineCpu"));
```

You also need to add these includes

```c++
#include "magritte/api/magritte_api_factory.h"
#include "mediapipe/framework/calculator.pb.h"
```

and add dependencies in your BUILD file (only showing new entries):

```python
cc_binary(
    name = "magritte_deidentify_image",
    # [...]
    deps = [
        # [...]
        "@magritte//magritte/api:magritte_api_factory",
        "@magritte//magritte/graphs:face_pixelization_offline_cpu",
        "@mediapipe//mediapipe/framework:calculator_cc_proto",
    ],
)
```

### Using the graph to process an image

For treating images, Magritte defines a simple interface:
[`DeidentifierSync`](https://github.com/google/magritte/blob/master/magritte/api/magritte_api.h). It
contains a method `Deidentify` which (essentially) takes an image and returns
the resulting treated image.

To obtain such a `DeidentifierSync`, use the factory methods defined in
[`magritte_api_factory.h`](https://github.com/google/magritte/blob/master/magritte/api/magritte_api_factory.h).
They take a graph as input, which we already know how to load.

Let's add this to our `magritte_deidentify_image.cc` file.

```c++
// Inside the Run() function (new code only):
// Create DeidentifierSync
ASSIGN_OR_RETURN(
      std::unique_ptr<magritte::DeidentifierSync<mediapipe::ImageFrame>>
          deidentifier,
      magritte::CreateCpuDeidentifierSync(graph_config));
```

We now have a `DeidentifierSync` that operates on `ImageFrame`s. We just need to
obtain an `ImageFrame`. As said above, you can use the function in
[image_frame_opencvx.h](https://github.com/google/mediapipe/blob/master/mediapipe/framework/formats/image_frame_opencvx.h)
to convert between `ImageFrame`s and OpenCV matrices, so if you're working with
OpenCV in your project that's the approach you should take. For convenience,
we've created a small
[`image_io_util` library](https://github.com/google/magritte/blob/master/magritte/examples/codelab/image_io_util.h)
that allows you to read and write image files into/from `ImageFrame`s (note that
it only works for specific image formats, see the comments in the file). Let's
use that to read a file, pass the resulting `ImageFrame` into the `Deidentify`
function of the `DeidentifierSync` we created, and save the resulting
`ImageFrame` in an image file.

We thus complete our `Run` method to the following:

```c++
absl::Status Run(const std::string& input_file,
                 const std::string& output_file) {
  // [...]
  // Read image file into an ImageFrame.
  ASSIGN_OR_RETURN(std::unique_ptr<mediapipe::ImageFrame> image,
                   magritte::LoadFromFile(input_file));
  // Deidentify the ImageFrame.
  ASSIGN_OR_RETURN(std::unique_ptr<mediapipe::ImageFrame> result,
                   deidentifier->Deidentify(std::move(image)));
  // Close the DeidentifierSync.
  MP_RETURN_IF_ERROR(deidentifier->Close());
  // Save the result ImageFrame into a file.
  return magritte::SaveToFile(output_file, *result);
}
```

We can add command line flags for the input and output file names, and read them
in the `main` function. Doing so, we arrive at the following complete code:

```c++
#include <memory>
#include <string>

#include "mediapipe/framework/calculator.pb.h"
#include "mediapipe/framework/port/logging.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/status/status.h"
#include "magritte/api/magritte_api_factory.h"
#include "magritte/examples/codelab/image_io_util.h"

ABSL_FLAG(std::string, input_file, "", "input file path");
ABSL_FLAG(std::string, output_file, "", "output file path");

absl::Status Run(const std::string& input_file,
                 const std::string& output_file) {
  ASSIGN_OR_RETURN(mediapipe::CalculatorGraphConfig graph_config,
                   magritte::MagritteGraphByName("FacePixelizationOfflineCpu"));
  ASSIGN_OR_RETURN(std::unique_ptr<mediapipe::ImageFrame> image,
                   magritte::LoadFromFile(input_file));
  ASSIGN_OR_RETURN(
      std::unique_ptr<magritte::DeidentifierSync<mediapipe::ImageFrame>>
          deidentifier,
      magritte::CreateCpuDeidentifierSync(graph_config));
  ASSIGN_OR_RETURN(std::unique_ptr<mediapipe::ImageFrame> result,
                   deidentifier->Deidentify(std::move(image)));
  MP_RETURN_IF_ERROR(deidentifier->Close());
  return magritte::SaveToFile(output_file, *result);
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  absl::ParseCommandLine(argc, argv);
  std::string input_file = absl::GetFlag(FLAGS_input_file);
  std::string output_file = absl::GetFlag(FLAGS_output_file);
  absl::Status status = Run(input_file, output_file);
  LOG(INFO) << status;
  return status.raw_code();
}
```

### Running the binary

The binary will need to load certain files at runtime, e.g. a file containing a
face detection ML model. The exact list of files depends on the graph you chose,
but there's an easy way to collect the required files and tell the binary how to
find them.

To do so, add the following to your `BUILD` file:

```python
load(
    "@magritte//magritte:magritte_graph.bzl",
    "magritte_resources_folder",
    "magritte_runtime_data",
)

magritte_runtime_data(
    name = "runtime_data",
    # In the following line, change the target by the one for your chosen graph.
    deps = ["@magritte//magritte/graphs:face_pixelization_offline_cpu"],
)

magritte_resources_folder(
    name = "resources_folder",
    runtime_data = ":runtime_data",
)
```

See the [developer's guide](https://google.github.io/magritte/technical_guide/dev_guide.html#resource-folders)
for more information on how these build rules work.

Then build the `resources_folder` target with Bazel. The output should look
something like this:

```shell
$ bazel build :resources_folder --experimental_repo_remote_exec
[...]
Target :resources_folder up-to-date:
  bazel-bin/resources_folder/mediapipe/modules/face_detection/face_detection_full_range.tflite
  bazel-bin/resources_folder/mediapipe/modules/face_detection/face_detection_full_range_sparse.tflite
  bazel-bin/resources_folder/mediapipe/modules/face_detection/face_detection_short_range.tflite
[...]
```

It should contain a list of files in a folder `bazel-bin/resources_folder` or
similar. The list of files and name of the folder may vary in your case.
We need to provide the absolute path to this folder as a value to the
`resource_root_dir` command line flag when running the binary. So run the binary
with the following command:

```shell
bazel run :magritte_deidentify_image \
  --cxxopt='-std=c++17' --experimental_repo_remote_exec \
  -- \
  --resource_root_dir=/absolute/path/to/workspace/bazel-bin/resources_folder \
  --input_file=/absolute/path/to/your/input/image.jpg \
  --output_file=/absolute/path/to/your/output/image.jpg
```

Remember that you need to add the flag `--define MEDIAPIPE_DISABLE_GPU=1` before
the `--` if you do not have a GPU (see the
[desktop example instructions](https://google.github.io/magritte/getting_started/demos.html#build-desktop)).

You can also build the binary with `bazel build` and then run it directly
instead of calling `bazel run` (see the
[Bazel user guide](https://bazel.build/docs/user-manual)).

Now try to run this binary on an image file that contains faces! You should see
that the faces are pixelized in the output file.

TODO: Add example images.

### Variations

#### Asynchronous Processing

In the above example we used the `DeidentifierSync` class. Its `Deidentify`
method returns (essentially) the processed image and blocks until the processing
is complete. Sometimes this is not what you need, so there is also a
`DeidentifierAsync` class. Its `Deidentify` method returns immediately, just
giving you an `absl::Status`. It will call a callback on the resulting image
once processing is completed. You provide the callback in the factory function.

Have a look at the documentation in
[`magritte_api.h`](https://github.com/google/magritte/blob/master/magritte/api/magritte_api.h) and
[`magritte_api_factory.h`](https://github.com/google/magritte/blob/master/magritte/api/magritte_api_factory.h)
for more information. Try to use that and adapt the code from the previous
example to use asynchronous processing!

In our repository you will find
[a complete example containing both synchronous and asynchronous processing](https://github.com/google/magritte/blob/master/magritte/examples/codelab/magritte_deidentify_image.cc).

#### Using a GPU

If your machine has a GPU, we strongly recommend using it whenever possible,
since processing will be faster.

To use a GPU with Magritte, you need to change the following two things from
what we did before:

1.  Use a graph intended for GPU processing. All such graphs have a name ending
    in `Gpu`. The ones with their name ending in `Cpu` will do processing on the
    CPU even if you have a GPU.
1.  Use `GpuBuffer` instead of `ImageFrame`.

The first item is easy: Just load a different graph.

For the second one, you need to use MediaPipe's
[`GpuBuffer` class](https://github.com/google/mediapipe/blob/master/mediapipe/gpu/gpu_buffer.h). It
is a platform-specific implementation of an image in GPU memory. You can obtain
a `GpuBuffer` from an `ImageFrame` using the functions in MediaPipe's
[`GlCalculatorHelper` class](https://github.com/google/mediapipe/blob/master/mediapipe/gpu/gl_calculator_helper.h).
Please see the comments in the linked files for more information.

Once you have loaded a graph and obtained a `GpuBuffer`, the only other thing
you need to adapt in the above example is the call to the factory function:
above we called `magritte::CreateCpuDeidentifierSync(graph_config)` which gives
us a `DeidentifierSync` implementation that operates on `ImageFrame`s. If you
replace `Cpu` by `Gpu` in the function name, you will receive a
`DeidentifierSync` operating on `GpuBuffer`s instead.

## A command line tool to deidentify videos

Now let's create a version of our deidentification tool that can work with
videos. We will process videos frame by frame, so there's not much difference to
the still images case. However, when dealing with videos, we recommend paying
attention to the timestamps, as we will explain now.

Have a look at the
[`DeidentifierSync` class definition](https://github.com/google/magritte/blob/master/magritte/api/magritte_api.h)
(everything we say applies equally to the `DeidentifierAsync`). As you can see,
there are two versions of the `Deidentify` method. Both of them take an image,
but one version takes an extra timestamp parameter. We will use the timestamped
version here. See [below](#timestamped-vs-non-timestamped-processing-methods)
for an explanation on why we always recommend using the timestamped method for
video processing.

### Reading and writing a video

For reading a video file, we can use OpenCV's
[`VideoCapture`](https://docs.opencv.org/4.6.0/d8/dfe/classcv_1_1VideoCapture.html)
class, and for writing to one, we can use
[`VideoWriter`](https://docs.opencv.org/4.6.0/dd/d9e/classcv_1_1VideoWriter.html).
Those classes work with OpenCV matrices, which we can convert to and from
`ImageFrame`s as described before.

Concretely, we can read and write video frames one by one as follows:

```c++
cv::VideoCapture capture(input_file);
cv::VideoWriter writer;
cv::Mat frame_raw;
for (capture >> frame_raw; !frame_raw.empty(); capture >> frame_raw) {
  cv::Mat deidentified_mat;
  // Process frame_raw, save result in deidentified_mat.
  // [...]
  // Write the output frame.
  if (!writer.isOpened()) {
    writer.open(output_file, /* some other parameters [...] */);
  }
  writer.write(deidentified_mat);
}
capture.release();
writer.release();
```

Before this loop, let's create a `DeidentifierSync` as before (assuming you have
loaded a graph exactly as in the previous example):

```c++
ASSIGN_OR_RETURN(
  std::unique_ptr<magritte::DeidentifierSync<mediapipe::ImageFrame>>
      deidentifier,
  magritte::CreateCpuDeidentifierSync(graph_config));
```

For the timestamps, let's also calculate the duration of a single frame
(timestamps are always in microseconds) and count the frame numbers:

```c++
double fps = capture.get(cv::CAP_PROP_FPS);
int64_t frame_duration_us = 1e6 / fps;
int frame_number = 0; // increment this by one in every loop iteration
```

Inside the loop, we can essentially do the same as we did before for still
images. We convert `frame_raw` into an `ImageFrame`, send it to the
`DeidentifierSync` along with the timestamp for the current frame, and convert
the output back to an OpenCV matrix. So in the loop we call

```c++
ASSIGN_OR_RETURN(
    std::unique_ptr<mediapipe::ImageFrame> deidentified_frame,
    deidentifier->Deidentify(std::move(input_frame),
                             frame_number * frame_duration_us));
```

These code snippets should give you an idea of how video processing with
Magritte works. We left out a few technical details. In our repository you can
find the
[full code of the video example](https://github.com/google/magritte/blob/master/magritte/examples/codelab/magritte_deidentify_video.cc).

### Timestamped vs. non-timestamped processing methods

When processing any data, the underlying technology used in Magritte, MediaPipe,
always works with timestamps. This is because, for some forms of processing,
time-wise relations between data matters. Imagine for example a case involving
motion tracking. Here the order of frames matters greatly, because motion
tracking computes differences between consecutive frames.

The methods in the Magritte API that do not take timestamps as parameters
implicitly choose a reasonable value when calling MediaPipe. This allows you to
process still images in an easy way without having to make up timestamps. Beware
that this may lead to unexpected results in case you choose a graph for still
image processing that contains motion tracking or other methods that are meant
for video processing.

In a video, we have natural timestamps available. So while using the methods
without timestamp parameters and letting them implicitly choose timestamps would
work, we recommend using the methods that do take a timestamp whenever you
process a video.

We recommend not to mix usage of timestamped and non-timestamped processing
methods.

See the comments in
[`magritte_api.h`](https://github.com/google/magritte/blob/master/magritte/api/magritte_api.h) and the
[graphs page](https://google.github.io/magritte/technical_guide/graphs.html) for more
information.

## Further reading

You now know how to use ready-made Magritte graphs to process images and videos.
Here we list some resources you can consult for more advanced topics.

### Creating your own graphs

All the examples in this codelab used a ready-made top-level graph. If the
top-level graphs described on the
[graphs page](https://google.github.io/magritte/technical_guide/graphs.html) don't suit
your needs, you can create your own graphs. For this you should learn about
subgraphs, calculators and other concepts; start on the
[concepts page](https://google.github.io/magritte/technical_guide/concepts.html) and then
continue with the
[developer's guide](https://google.github.io/magritte/technical_guide/dev_guide.html),
which has a section about building your own graphs.

### Partial processing (detection or redaction only)

Your use case may not need end-to-end image deidentification, but only a partial
treatment, for example only detect faces, or only redact faces that have been
previously detected.

In this case, you need to use a subgraph for this purpose instead of a top-level
graph. At this point, Magritte does not offer a simple API for this scenario as
it does for the Deidentification use case, so you will need to work with
MediaPipe directly. For this we refer to the
[concepts page](https://google.github.io/magritte/technical_guide/concepts.html) and the
[MediaPipe documentation](https://google.github.io/mediapipe/).
