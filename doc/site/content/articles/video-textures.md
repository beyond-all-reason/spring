---
title: Video textures
---

# Video textures

Video textures let unsynced Lua draw the frames of a pre-rendered video anywhere
an ordinary texture can be used. The same API can cover the screen for a
cutscene or draw a small looping UI portrait. Playback uses real time and does
not change simulation speed or automatically pause the game.

Video playback is available in client builds configured with
`-DENABLE_VIDEO=ON`. Headless, dedicated, unitsync, and feature-disabled builds
do not link FFmpeg. `gl.CreateVideoTexture` returns `nil` and an explanatory
message when playback is unavailable.

## Authoring contract

The initial format is deliberately narrow:

- MP4 or MOV container
- H.264/AVC, progressive 8-bit `yuv420p`, constant dimensions
- no larger than 4096x2160 with the default safety limits
- AAC-LC stereo audio, or no audio
- no alpha channel, subtitles, network streams, or hardware decoding

Archived VFS members are currently buffered completely by `CFileHandler`.
Account for the compressed asset plus decoder queues when budgeting memory;
keep cutscenes reasonably sized and split very long media when appropriate.
Deployments can tighten `VideoMaxActiveDecoders`, `VideoMaxFileSizeMB`,
`VideoMaxWidth`, `VideoMaxHeight`, and `VideoQueuedFrames`.
Probe/decode and audio queue limits are exposed as `VideoProbeSizeMB`,
`VideoAnalyzeDurationMS`, `VideoDecoderThreads`, and `VideoPCMQueueMB`.

This authoring command produces a compatible 1080p30 file. `libx264` is an
authoring-time encoder and is not linked into the engine:

```sh
ffmpeg -i input.mov -vf "scale=1920:1080:force_original_aspect_ratio=decrease,pad=1920:1080:(ow-iw)/2:(oh-ih)/2,format=yuv420p" \
  -r 30 -c:v libx264 -profile:v high -level 4.1 -g 60 -keyint_min 60 -sc_threshold 0 \
  -c:a aac -profile:a aac_low -ac 2 -ar 48000 -movflags +faststart output.mp4
```

A two-second keyframe interval (`-g 60` at 30 FPS) is a useful starting point
for responsive seeking and looping.

## Lua API

```lua
local video, err = gl.CreateVideoTexture("videos/intro.mp4", {
  autoplay = true,
  loop = false,
  audio = true,
  volume = 1.0,
})

if not video then
  Spring.Log("cutscene", LOG.ERROR, err)
  return
end

gl.PlayVideoTexture(video)
gl.PauseVideoTexture(video)
gl.SeekVideoTexture(video, 12.5)
gl.SetVideoTextureVolume(video, 0.75)
gl.StopVideoTexture(video) -- rewinds to zero

local info = gl.GetVideoTextureInfo(video)
-- state: opening, ready, playing, paused, seeking, finished, stopped, error
-- width, height, duration, position, hasAudio, error, droppedFrames

gl.DeleteVideoTexture(video) -- also accepted by gl.DeleteTexture
```

Handles start with `@` and belong to the Lua context that created them. They are
deleted automatically when that context is destroyed or reloaded. Explicit
deletion is safe and idempotent. A stale handle cannot access another context's
video.

### Full-screen cutscene

```lua
function widget:DrawScreen()
  if not video then return end
  local vsx, vsy = Spring.GetViewGeometry()
  gl.Texture(video)
  gl.TexRect(0, 0, vsx, vsy)
  gl.Texture(false)
end
```

Lua remains responsible for aspect fit or crop, letterboxing, input capture,
skip behavior, and any game-pause policy. Poll `GetVideoTextureInfo`; close the
screen on `finished`, and show a static fallback on `error`.

### Muted looping portrait

```lua
portrait = gl.CreateVideoTexture("videos/portraits/armcom.mp4", {
  autoplay = true,
  loop = true,
  audio = false,
})

function widget:DrawScreen()
  if not portrait then return end
  gl.Texture(portrait)
  gl.TexRect(24, 24, 280, 280)
  gl.Texture(false)
end
```

Do not pre-open a decoder for every unit type. Open the selected portrait,
delete it when selection changes, and retain a static image while it is opening
or if it fails.
