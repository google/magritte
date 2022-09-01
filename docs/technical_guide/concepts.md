---
layout: default
title: Magritte concepts
parent: Technical guide
nav_order: 1
---

# Magritte concepts
{: .no_toc }

1. TOC
{:toc}
---

This document gives an overview of the architecture of the Magritte library and
its concepts.

NOTE: You may not need to understand all the concepts explained here for your
application of Magritte. We recommend reading the
[codelab](https://google.github.io/magritte/getting_started/codelab.html) first to get
started, and only come back to this page if you need to.

## MediaPipe Basics

The Magritte library is built on top of
[MediaPipe](https://mediapipe.dev), so
you should have some understanding of how MediaPipe works before working with
the Magritte library. Here we summarize some of the most important concepts, but
this is just for your convenience and is thus kept at a very high and informal
level. For more details please refer to the official documentation, in
particular the [framework concepts](https://google.github.io/mediapipe/framework_concepts/framework_concepts.html)
section.

MediaPipe is a framework to process media streams that consist of *packets*. A
packet has a timestamp (which is a positive integer) and a payload, which can be
any kind of data (more precisely, any C++ type).

MediaPipe works with *graphs* that define how media should be processed. Those
graphs consist of nodes, where data is processed, and of edges ("pipes") that
connect them. Graphs can have any number of input streams (for consuming packets
to process) and output streams (for outputting results). Packets are usually
assumed to arrive with strictly monotonically increasing timestamps.

![An example MediaPipe graph, consisting of nodes labelled "calculator" and
"subgraph", input and output streams, and edges between the
nodes](images/example_mediapipe_graph.png "An example MediaPipe graph")

Imagine packets ("media") flowing into the graph via its input streams and
through the various nodes, getting processed there, and eventually coming out of
the output streams. The MediaPipe framework takes care of synchronization and
parallelization.

There are two types of nodes in a MediaPipe graph:

*   *Calculators* are the smallest unit in any MediaPipe graph. They are
    basically C++ programs that define how the data should be processed in that
    node.
*   Nodes can also reference other MediaPipe graphs, which get embedded as
    subgraphs into the larger graph.

## Magritte Architecture

The Magritte library consists of MediaPipe graphs and calculators. It is
designed in a modular way that makes it easy to plug graphs together in many
different ways to build solutions adapted to the specific situation at hand.
Here we describe the architecture that makes this possible.

Note: For standard applications, you can often use one of the available Magritte
graphs to solve a problem (e.g. to pixelize faces). Such ready-to-use graphs are
called top-level graphs, see below. If that sufficient for you and you just want
to use it, you can skip the technical details and follow the
[codelab](https://google.github.io/magritte/getting_started/codelab.html); see also the
[Developer's Guide](https://google.github.io/magritte/technical_guide/dev_guide.html).

### Categories of Graphs

Here are some examples of problems that the Magritte library can solve:

*   detect faces in videos,
*   from a face detection, calculate an oval area on the frame that nicely
    covers the face,
*   blur a certain area in a frame.

Those examples, and many more, are examples of more general problem types:

*   detecting sensitive content in a video,
*   for previously detected sensitive content, decide which area of a frame
    needs to be redacted to hide it,
*   redact a certain area of a frame.

With that preparation, let's define some terminology.

Magritte Feature
:   A general problem type as described above that the Magritte library can
    solve. See the [Magritte Features](#magritte-features) section below for a
    complete list of all available Magritte Features.

Feature Subgraph
:   A MediaPipe graph that solves a specific instance of one of those problem
    types.

The Feature Subgraphs that belong to the same feature will often have similar
signatures (by which we mean the set of input and output streams and their data
types, in analogy to the signature of a function).

NOTE: The property of a graph being a Feature Subgraph is not defined by a
technical condition, like "all graphs having a certain signature". Instead,
there is a manually created and maintained list of Feature Subgraphs, and what
they have in common is that they serve a similar purpose.

Not every graph in the Magritte library is a Feature Subgraph. There are two
more categories.

Top-level Graph
:   A "main" graph that operates on videos (meaning that it has one stream of
    images as input and output, respectively). Such graphs are usually composed
    of Feature Subgraphs, but they may contain additional nodes to
    [optimize for certain situations](https://google.github.io/magritte/technical_guide/dev_guide.html#live-vs-offline-treatment).
    An example of a top-level graph would be a graph that blurs all faces that
    can be found in a video.

Technical Subgraphs
:   A subgraph of a Feature Subgraph that doesn't solve a general type of
    problem, but just exist for technical reasons as a part of a bigger graph.

For many applications, using one of the pre-defined Top-level Graphs should be a
good solution. If this is not sufficient, users of the library can compose their
own Top-level Graphs by plugging together Feature Subgraphs.

Feature Subgraphs can be plugged together in any way provided the inputs and
outputs match up. There are no restrictions on how many Feature Subgraphs of the
same type can be used within any binary, or even as subgraphs within the same
top-level graph.

There should almost never be a need to use Technical Subgraphs directly, which
is why they are not mentioned any further in this documentation.

Let's conclude this section with a simple example that shows how feature
subgraphs can be combined into a top-level graph that solves a certain problem.

![An example Top-level graph, consisting of two nodes labelled "FaceDetection"
and "FacePixelization" and with one input and output stream, respectively,
labelled input_video and
output_video](images/example_toplevel_graph.png "An example Top-level graph")

Here you can see a top-level graph for face pixelization. Both nodes are feature
subgraphs. The upper node labelled `FaceDetection` consumes video frames and
produces a stream of detections, and the lower node labelled `FacePixelization`
consumes the detections produced by the upper node and in addition the original
video frames, and outputs a stream of frames where the faces are pixelized.

### Important Data Concepts

There are certain types of data that play an important role in the Magritte
library, and they are the ones that Feature Subgraphs mostly operate on. They
are not data types in the strict sense, rather in a conceptual way, which is why
we call them Data Concepts to avoid confusion. See the
[Developer's Guide](https://google.github.io/magritte/technical_guide/dev_guide.html) for
more details, including a description of which actual data types correspond to
the concepts, along with code pointers.

Frames
:   A still image, which typically represents a frame from a video.

Detections
:   Detections capture information about the location and nature of detected
    sensitive content in a video frame. They may contain information about
    keypoints (e.g. location of the eyes for face detections).

Masks
:   Masks are images representing areas within a frame with an intensity, which
    is assigned to each pixel and lies between `0` and a maximal value
    (depending on the format). A value of `0` (black) means that this pixel is
    not part of the region, a maximal pixel value (e.g. `255` (white)) means
    that the pixel is part of the region, and values in between will be treated
    depending on the situation.

### Magritte Features

Here we list all Magritte features, together with input and output streams that
corresponding feature subgraphs will typically have.

WARNING: Feature subgraphs belonging to the same feature do not need to have a
common signature. Instead, their crucial property is the fact that they serve a
similar purpose. Therefore, the given inputs and outputs here are just a rule of
thumb, concrete feature subgraphs may have different (mostly additional) input
and output streams. For example, most detection feature subgraphs only consume
frames, but some variants may consume additional parameters to influence or
optimize detections.

There's an
[overview of all existing top-level and feature graphs](https://google.github.io/magritte/technical_guide/graphs.html).

Feature             | Description                                                                                                                 | Graph input                                  | Graph output     | Examples
:------------------ | :-------------------------------------------------------------------------------------------------------------------------- | :------------------------------------------- | :--------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------
Detection           | Detect sensitive content in video frames                                                                                    | Frame                                        | Detections       | Face detection in different variants; detection of other content (hands, persons, etc.)
Tracking            | Motion tracking                                                                                                             | Frame, detections                            | Detections       | Box tracking, object tracking
Segmentation        | Detect sensitive regions in frames                                                                                          | Frame                                        | Mask             | Hair segmentation
Detection to Mask   | Convert detected sensitive content into regions in frames by adding to input regions                                        | Detections (possibly multiple streams), mask | Mask             | A graph that simply draws the location of a detection into a mask; A graph that consumes face detections and makes use of eye positions to create rotated ovals
Mask Redaction      | Redact sensitive regions in frames                                                                                          | Mask                                         | Frame            | Pixelization, blurring
Detection Redaction | Redact sensitive detected content in frames (mostly in a way where the type of the content matters and not only the region) | (Input) frame, detections                    | (Redacted) frame | Stickers on faces

Summary: The Magritte library consists of MediaPipe graphs and calculators that
can be grouped into *Top-level Graphs*, *Feature Subgraphs* that solve
particular problems, *Technical Subgraphs*, and *calculators* that may appear
anywhere in those graphs. \
The graphs and calculators operate mostly on the data concepts *frames*,
*detections* and *masks*.
