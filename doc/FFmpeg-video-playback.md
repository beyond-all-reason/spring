# FFmpeg video playback dependency

RecoilEngine video playback is built from upstream FFmpeg revision
`68af2cc3feb8c78aec2722c728fd87f03515fa7c` (the 7.1.1 release preparation
revision). The Docker dependency images verify the checked-out revision before
building it.

The build is static and deliberately does not enable FFmpeg's GPL or nonfree
configuration modes. It disables programs, documentation, networking,
`libavdevice`, `libavfilter`, post-processing, and all components before enabling
only these runtime pieces:

- `libavformat`, with the MOV/MP4 demuxer
- `libavcodec`, with native H.264 and AAC decoders and parsers
- `libavutil`, `libswscale`, and `libswresample`

No external x264 library is linked. x264 appears only in the optional authoring
example in the video-texture documentation.

The dependency source is available from <https://git.ffmpeg.org/ffmpeg.git>.
Portable packages install `FFMPEG-COPYING.LGPLv2.1` and
`FFMPEG-MANIFEST.txt`; the latter records the exact revision and configure
arguments produced by the dependency build.

Static LGPL compliance and H.264 patent obligations must be reviewed by the
release owner for every distribution jurisdiction. This note records the build
inputs but is not a substitute for that legal review or any required relinking
materials/source offer.
