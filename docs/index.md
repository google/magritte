---
layout: default
title: Home
nav_order: 1
---

# Magritte

Magritte is an image redaction library to disguise the identity of persons in
photos and video streams.

## Why should you use Magritte?

### Technology

Magritte offers Visual Deidentification technology that is

*   **robust**: Magritte allows to use redaction technology that provides a
    higher level of obfuscation than simple blurring.
*   **reliable**: We allow to combine and tweak detection strategies to detect
    sensitive content more reliably.
*   **fast**: We offer solutions that are optimized towards a real-time
    situation, while others are optimized towards offline treatment.
*   **modular**: You can combine various detection and redaction pieces in many
    different ways to find a solution that fits your situation, and you can also
    use them on their own.

WARNING: Magritte is currently in early development. We are still evaluating the
precision of the detection and redaction methods we provide. For the face
detection models, please refer to
[MediaPipe's documentation](https://google.github.io/mediapipe/solutions/models.html#face-detection)
to understand their precision.

### Getting started

If you want to try out some demos, see
[Trying out Magritte](https://google.github.io/magritte/getting_started/demos.html)

If you're an engineer and want to learn how to use Magritte in your project, we
recommend starting with the
[codelab](https://google.github.io/magritte/getting_started/codelabs.html). It gives
simple examples that help you understand the basic concepts and links to the
more advanced material that you can read in case you need it.

Magritte comes as a library that is built on top of
[MediaPipe](https://mediapipe.dev/). As
such, it can be used in any situation where you can use MediaPipe. That means it
has interfaces to process videos from various languages (C++, Java, Objective-C,
Python, JavaScript) and on various platforms (Desktop, Web, Android, iOS). It
supports both CPU and GPU treatment.
