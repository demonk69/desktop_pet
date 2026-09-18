# Assets

V0.2 includes small ASCII P3 PPM pixel-art frames under `pet/`. `pet/manifest.txt`
maps logical asset IDs to files; the PC file provider lazily converts them to RGB565.

These files validate the state-to-animation-to-renderer pipeline, not final art. Future
providers may resolve the same IDs from compiled assets, Flash, PSRAM, SD, or another
filesystem format without changing Pet Core or the animation player.
