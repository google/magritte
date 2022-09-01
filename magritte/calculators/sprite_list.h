//
// Copyright 2016-2022 Google LLC
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
#ifndef MAGRITTE_CALCULATORS_SPRITE_LIST_H
#define MAGRITTE_CALCULATORS_SPRITE_LIST_H

#include <vector>

#include "magritte/calculators/sprite_pose.pb.h"
#include "mediapipe/framework/packet.h"

namespace magritte {

// A simple storage representing a sprite image and its pose. This should
// encapsulate all the information necessary to render the sprite over a given
// background.
struct SpriteListElement {
  SpriteListElement() {}
  SpriteListElement(const mediapipe::Packet& image_packet, const SpritePose& pose)
      : image_packet(image_packet), pose(pose) {}

  // This packet contains the image used to render the sprite. In CPU pipelines,
  // this must be an SRGB or SRGBA ImageFrame. In GPU pipelines, it must be a
  // GpuBuffer. The image is assumed to have a premultiplied alpha channel.
  mediapipe::Packet image_packet;
  SpritePose pose;
};

typedef std::vector<SpriteListElement> SpriteList;

}  // namespace magritte

#endif  // MAGRITTE_CALCULATORS_SPRITE_LIST_H
