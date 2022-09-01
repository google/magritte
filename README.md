# Magritte

Magritte is a [MediaPipe](https://mediapipe.dev)-based library to redact faces
from photos and videos.

It provides *processing graphs* to reliably detect faces, track their movements
in videos, and disguise the person's identity by obfuscating their face.

Processing graphs are built from *feature subgraphs* that solve sub-problems
such as detecting or tracking faces (without recognizing them), determining the
area to be redacted, and applying de-identification effect to this area (e.g.,
blurring, pixelization or sticker redaction).

To get started, refer to our [GitHub page](https://google.github.io/magritte/).
