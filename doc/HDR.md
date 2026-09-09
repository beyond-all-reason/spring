# HDR output

RecoilEngine's experimental HDR path targets Windows 10/11, SDL3, and OpenGL.
It uses linear extended sRGB (scRGB) in a floating-point presentation
framebuffer. SDR remains the default and fallback.

## Configuration

`HDRMode` accepts `off`, `auto`, or `on` and requires an engine restart.

- `off` uses the legacy SDR framebuffer and rendering path.
- `auto` attempts HDR when the operating system reports HDR enabled.
- `on` always attempts an HDR-capable floating-point window.

Both `auto` and `on` fall back to SDR if window or context creation, format
verification, or render-target allocation fails. The reason is logged and
available through `Spring.GetHDRInfo()`.

HDR output is active only when the current SDL window reports HDR, the default
framebuffer is verified as floating point, and the scene-linear presentation
pipeline is available. Moving the window to an SDR display updates runtime
state but does not recreate the window.

## Lua API

`Engine.FeatureSupport.hdrOutputApiVersion` is `1`. Unsynced Lua can call
`Spring.GetHDRInfo()` and compare its `generation` field while polling.
Capability is reported as `supported`, `unsupported`, or `unknown`; absence of
platform capability information is never interpreted as unsupported.

During `DrawScreenEffects`, `$scene_color` names the resolved `RGBA16F`
scene-linear color texture and `$scene_depth` names its depth/stencil texture.
They are engine-owned, valid until the next resize/context recreation, and
must not be attached to a writable Lua FBO.

The engine renders world and scene effects before the final output transform.
HUD, cursor, console, RmlUi, and other screen UI are composed afterward at SDR
reference white. Scene working primaries are linear sRGB/Rec.709. Negative
presentation values are clamped; highlights roll off against reported
headroom. Invalid white-level or headroom properties fall back to `1.0`.

## Screenshots

The ordinary screenshot command and `Spring.TakeScreenshot("sdr", "png", 80)`
capture the final presented SDR-compatible framebuffer.

`/screenshothdr` or `Spring.TakeScreenshot("hdr")` writes
`screenshots/hdr_scene_<timestamp>.hdr`. This is a scene-linear Radiance RGBE
capture of `$scene_color`, before UI composition, in linear sRGB/Rec.709
primaries. Values above `1.0` are preserved, subject to RGBE precision. HDR
capture reports an error when no scene-linear target exists.

Radiance HDR has no standardized container metadata for primaries, reference
white, or exposure, so this document defines the interpretation. Viewers may
apply their own exposure or tone mapping.

The legacy in-engine video recorder remains SDR and records the final
tone-mapped presentation. Use an external recorder such as OBS Studio for HDR
video capture.

## Troubleshooting

Check `infolog.txt` for the requested and actual channel formats, display
Advanced Color state, window HDR state, SDR white level, headroom, and fallback
reason. On Windows, enable HDR for the target display before starting the
engine when using `auto`.
