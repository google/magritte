---
layout: default
title: Changelog
nav_order: 4
---

# Changelog

This document contains a summary of all changes to the Magritte open-source
library.

## 1.0.1

*   Add overlay graphs to display both base detection and tracked detections.
*   Replace absl::make_unique with std::make_unique (introduced in C++14).

## 1.0.0

*   First release to GitHub.
*   New graphs: face blurring, face overlay, face pixelization, face sticker
    redaction.
*   Magritte library API that simplifies common tasks by providing abstractions
    around the use of MediaPipe in many places.
*   Added Android and desktop example apps.
*   Added documentation and C++ codelab.
