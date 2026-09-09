# SDL3 Migration — Manual In-Game Testing Checklist

> Verify all engine features work correctly after the SDL2→SDL3 migration (131 files, +1224/-1148 lines).
> Each section corresponds to a migration phase. Mark ☑ pass / ☒ fail / ☐ skipped. Document any failures.

---
# Found bugs needing fixing:
- [ ] | B3 | Toggle back to windowed from fullscreen | Returns to windowed mode at correct size | NO: switch to windowed from fullscreen needs to be toggled twice from options menu, to two different resolutions in order for it to take effect
- [ ] Need to test RMLUI altogether anyway

---

## A. Window Creation and Display

| # | Test | Expected | Status |
|---|------|----------|--------|
| A1 | Launch engine to main menu | Window opens at configured resolution, no crash | ok |
| A2 | Window appears centered on primary display | Window is centered, not at (0,0) | ok |
| A3 | Window title bar shows correct game title | Title matches expected | ok |
| A4 | Window is visible immediately on launch | No hidden window; `SDL_WINDOW_SHOWN` removed in SDL3 | ok |
| A5 | Resize window manually (drag edges) | GL viewport resizes correctly, no black borders or stretching | ok |
| A6 | Resize triggers `SDL_EVENT_WINDOW_RESIZED` | UI layout updates, no frozen rendering | ok |
| A7 | Resize triggers `SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED` | GL backbuffer matches pixel size (critical on HiDPI) | ☐ |
| A8 | Minimize window | Game pauses or continues running without crash | ok |
| A9 | Restore minimized window | Rendering resumes correctly, no black screen | ok |
| A10 | Maximize window | Window fills screen, viewport updates | ok |
| A11 | Restore from maximized | Returns to previous size/position | ok |
| A12 | Close window (X button) | Game exits cleanly, no crash | ok |
| A13 | `SDL_EVENT_WINDOW_CLOSE_REQUESTED` fires | Graceful shutdown | ☐ |

## B. Fullscreen Modes

| # | Test | Expected | Status |
|---|------|----------|--------|
| B1 | Toggle exclusive fullscreen | Resolution changes, fullscreen activates | ok |
| B2 | Toggle borderless (desktop) fullscreen | Window fills screen, taskbar hidden | ok |
| B3 | Toggle back to windowed from fullscreen | Returns to windowed mode at correct size | NO: switch to windowed needs to be toggled twice |
| B4 | Fullscreen toggle via keyboard hotkey | Same as B1-B3 | n/a |
| B5 | Fullscreen toggle via Lua (if available) | No crash, mode changes | ok |
| B6 | `SDL_SetWindowFullscreen(bool)` async behavior | Mode change applies; no race with render loop | ☐ |
| B7 | Window border toggle (`SDL_SetWindowBordered`) | Borderless windowed mode works | n/a, I dont think we have borderless windowed  |
| B8 | Fullscreen after window resize | Correct resolution applied | ok |

## C. Multi-Monitor

| # | Test | Expected | Status |
|---|------|----------|--------|
| C1 | Move window from one display to another | Window renders correctly on second display | ok |
| C2 | `SDL_EVENT_WINDOW_DISPLAY_CHANGED` fires | Engine logs display change (check console/log) | n/a |
| C3 | Start fullscreen on secondary display | Fullscreen activates on correct monitor | ok |
| C4 | Displays with different DPI | Each display renders at correct pixel density | ok |
| C5 | `SDL_GetDisplays()` enumeration | All monitors detected (check log output) | ok |
| C6 | `SDL_GetDisplayBounds()` per display | Correct bounds logged for each monitor | ok |
| C7 | Window centered on secondary display | `SDL_WINDOWPOS_CENTERED_DISPLAY()` uses `SDL_DisplayID` | n/a |

## D. High-DPI / Retina

| # | Test | Expected | Status |
|---|------|----------|--------|
| D1 | Launch on HiDPI display (e.g., 200% scaling) | Window renders at full pixel density, not half-size | ☐ |
| D2 | `SDL_GetWindowSizeInPixels()` vs `SDL_GetWindowSize()` | Pixel size ≠ window size on HiDPI; GL viewport uses pixel size | ☐ |
| D3 | UI elements are readable | No blurry or tiny UI | ☐ |
| D4 | `SDL_WINDOW_HIGH_PIXEL_DENSITY` flag set | Pixel-accurate sizing active | ☐ |
| D5 | Screenshot at native resolution | Matches expected pixel dimensions | ☐ |

## E. VSync and Swap Interval

| # | Test | Expected | Status |
|---|------|----------|--------|
| E1 | VSync ON | Frame rate capped to monitor refresh rate | ok |
| E2 | VSync OFF | Frame rate uncapped, runs at max | ok |
| E3 | Adaptive VSync (if available) | No tearing, graceful handling of missed frames | cant test on win10, frames were already missing anyway (see jitter timer widget running cube) |
| E4 | `SDL_GL_SetSwapInterval` returns bool | No crash on swap interval change | ok |
| E5 | `SDL_GL_GetSwapInterval(int*)` out-param | Correct value reported in console/log | ok |
| E6 | Toggle vsync in-game during gameplay | Smooth transition, no flicker or crash | ok |

## F. Keyboard Input

| # | Test | Expected | Status |
|---|------|----------|--------|
| F1 | Basic WASD/movement keys | Unit movement responds correctly | ok |
| F2 | Modifier keys (Shift, Ctrl, Alt) | Modifiers work in hotkey combinations | ok |
| F3 | Num Lock, Caps Lock, Scroll Lock | State detected correctly (`SDL_KMOD_NUM`, etc.) | ☐ |
| F4 | All on-screen hotkeys | Every UI hotkey resolves to the same action as pre-SDL3 | ok |
| F5 | In-game console commands | Typing in console works, commands execute | ok |
| F6 | Chat input | Text entry in chat works | ok |
| F7 | Special characters and symbols | All keyboard layouts produce correct characters | ok |
| F8 | Non-US keyboard layout (if available) | Keys map correctly for non-US layouts | ☐ |
| F9 | `SDL_GetKeyboardState()` returns `const bool*` | Key state queries work (e.g., edge scroll while holding key) | ok |
| F10 | Key repeat (held keys) | Held keys repeat at expected rate | ok |
| F11 | `SDL_SetKeyRepeat` works | Key repeat can be toggled on/off | ☐ |
| F12 | Key up/down events | Both press and release detected | ok |
| F13 | `SDL_EVENT_KEY_DOWN` / `SDL_EVENT_KEY_UP` | Events fire in event loop | ☐ |
| F14 | `SDL_EVENT_KEYMAP_CHANGED` | Keyboard layout change detected (if applicable) | ☐ |
| F15 | Lua keycode compatibility | Widgets/gadgets receive same keycode values as SDL2 | ok |
| F16 | `event.key.key` and `event.key.scancode` fields | Direct field access works (no `event.key.keysym`) | ☐ |
| F17 | `event.key.down` (bool) | Button state is bool, not `SDL_PRESSED`/`SDL_RELEASED` | ☐ |
| F18 | `SDL_GetModState()` / `SDL_SetModState()` | Modifier state preserved across focus loss | ok |

## G. Text Input and IME

| # | Test | Expected | Status |
|---|------|----------|--------|
| G1 | `SDL_StartTextInput(window)` | Text input activates with window handle | ☐ |
| G2 | `SDL_StopTextInput()` | Text input deactivates | ☐ |
| G3 | `SDL_SetTextInputArea(window, rect, cursor)` | IME composition area positioned correctly | ☐ |
| G4 | `SDL_TextInputActive(window)` | Returns correct active state | ☐ |
| G5 | `SDL_EVENT_TEXT_INPUT` event | Characters received in event loop | ☐ |
| G6 | `SDL_EVENT_TEXT_EDITING` event | Composition updates received | ☐ |
| G7 | Text event `const char*` lifetime | Text copied before next pump (no use-after-free) | ☐ |
| G8 | IME input (CJK, if available) | Composition window appears, characters commit correctly | ☐ |
| G9 | Text input in chat box | Characters appear as typed | ☐ |
| G10 | Text input in console | Characters appear, commands work | ☐ |
| G11 | Text input in UI edit fields (aGui) | LineEdit widgets accept input | ok |
| G12 | Text input in RmlUi elements | UI text fields accept input | ☐ |

## H. Mouse Input

| # | Test | Expected | Status |
|---|------|----------|--------|
| H1 | Left mouse button | Unit selection, command issuance | ok |
| H2 | Right mouse button | Movement commands, context menus | ok |
| H3 | Middle mouse button (scroll) | Expected behavior (camera pan, zoom) | ok |
| H4 | Mouse button 4/5 (side buttons) | Bound actions execute | ☐ |
| H5 | `SDL_EVENT_MOUSE_BUTTON_DOWN` / `UP` | Both events fire correctly | ☐ |
| H6 | `event.button.down` (bool) | State is bool, not `SDL_PRESSED`/`SDL_RELEASED` | ☐ |
| H7 | `SDL_EVENT_MOUSE_MOTION` | Cursor movement tracked | ☐ |
| H8 | `event.motion.x/y` (float) | Sub-pixel coordinates work | ☐ |
| H9 | `event.motion.buttons` | Active buttons reported correctly | ☐ |
| H10 | `SDL_EVENT_MOUSE_WHEEL` | Scroll wheel triggers zoom/pan | ☐ |
| H11 | `event.wheel.x/y` (float) | Precise scroll amounts received | ☐ |
| H12 | `SDL_GetMouseState()` returns `float*` | Mouse coordinates as float | ☐ |
| H13 | `SDL_GetGlobalMouseState()` returns `float*` | Global coordinates as float | ☐ |
| H14 | Mouse wheel zoom | Zoom in/out works smoothly | ok |
| H15 | Edge scroll (hold key + move to edge) | Camera pans at screen edge | ok |
| H16 | `SDL_PeepEvents` with `SDL_EVENT_MOUSE_MOTION` | Event flushing works | ☐ |
| H17 | `SDL_FlushEvent(SDL_EVENT_MOUSE_MOTION)` | Pending motion events cleared | ☐ |

## I. Cursor

| # | Test | Expected | Status |
|---|------|----------|--------|
| I1 | Hardware cursor visible in menu | Custom cursor renders | NOPE not in menu |
| I2 | `SDL_ShowCursor()` / `SDL_HideCursor()` | Cursor can be shown/hidden | Does not hide on cinematic mode, but also does not show cursor on screenshot |
| I3 | `SDL_CursorVisible()` | Returns correct visibility state | ☐ |
| I4 | Cursor hidden during gameplay | No OS cursor visible | ok |
| I5 | Custom cursor image loads | Cursor bitmap renders correctly | ok |
| I6 | `SDL_CreateSurface` for cursor | Surface creation with `SDL_PIXELFORMAT_ABGR8888` works | i guess yeah |
| I7 | `SDL_CreateColorCursor` | Color cursor created from surface | n/a |
| I8 | `SDL_DestroyCursor` | Cursor freed without crash | probably ok because resizing cursor works |
| I9 | Mouse warp (`SDL_WarpMouseInWindow`) | Cursor repositioning works | very interesting, needs confirmation/lua hooks |
| I10 | Relative mouse mode (`SDL_SetWindowRelativeMouseMode`) | Cursor clamped to window, relative motion works | ok |
| I11 | Exit relative mouse mode | Cursor free again | ok |
| I12 | Cursor show/hide during video playback | `AviVideoCapturing` cursor toggle works | ☐ |

## J. Clipboard

| # | Test | Expected | Status |
|---|------|----------|--------|
| J1 | Copy text in-game console | Text copied to system clipboard | ok |
| J2 | Paste text into console | `SDL_GetClipboardText()` returns pasted text | ok |
| J3 | `SDL_SetClipboardText` | Returns `bool`, text set on clipboard | ☐ |
| J4 | `SDL_HasClipboardText` | Returns correct state | ☐ |
| J5 | `SDL_free()` on clipboard text | No memory leak, no crash | ☐ |
| J6 | Clipboard via Lua (`LuaUnsyncedCtrl`) | Lua copy/paste functions work | probably yeah because chat widget works with copy/paste |
| J7 | Copy/paste coordinates in chat | Full round-trip works | ☐ |

## K. Focus Handling

| # | Test | Expected | Status |
|---|------|----------|--------|
| K1 | Click outside game window | `SDL_EVENT_WINDOW_FOCUS_LOST` fires | probably needs a lua hook |
| K2 | Click back into game window | `SDL_EVENT_WINDOW_FOCUS_GAINED` fires | probably needs a lua hook |
| K3 | Input stops when window unfocused | No phantom key presses | ok |
| K4 | Input resumes when window focused | Input works immediately | ok |
| K5 | `SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS` | Window behavior on focus loss is correct | ☐ |
| K6 | Alt-Tab away and back | Game restores correctly | ok |
| K7 | Window grab (`SDL_SetWindowMouseGrab`) | Mouse confined to window when grabbed | ok |
| K8 | `SDL_GetWindowMouseGrab` | Returns correct grab state | ☐ |
| K9 | Keyboard grab (`SDL_SetWindowKeyboardGrab`) | Keyboard confined when grabbed | ☐ |

## L. Camera Controls

| # | Test | Expected | Status |
|---|------|----------|--------|
| L1 | Dolly camera | Zoom and pan work | ok |
| L2 | Free camera | Free movement works | ok |
| L3 | Overhead camera | Standard RTS camera controls | ok |
| L4 | Spring camera | Spring-style camera behavior | ok |
| L5 | FPS unit camera | First-person view works | ok |
| L6 | Camera key bindings | All camera hotkeys resolve correctly | ok |
| L7 | Mouse-based camera rotation | Smooth rotation, no jumps | ok |

## M. Audio

| # | Test | Expected | Status |
|---|------|----------|--------|
| M1 | Audio device detected at startup | Sound system initializes, no errors in log | ok |
| M2 | Game sounds play | Explosion, unit, and UI sounds audible | NOPE |
| M3 | `SDL_OpenAudioDevice` with SDL3 signature | Device opens with `SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK` | ☐ |
| M4 | `SDL_GetAudioDeviceFormat` | Format query succeeds | ☐ |
| M5 | `SDL_PauseAudioDevice(deviceID)` single arg | Audio pauses without crash | ☐ |
| M6 | `SDL_ResumeAudioDevice(deviceID)` | Audio resumes | ☐ |
| M7 | `SDL_CloseAudioDevice` | Device closes cleanly | ☐ |
| M8 | `SDL_EVENT_AUDIO_DEVICE_ADDED` | Hot-plug add detected (plug in headset) | ☐ |
| M9 | `SDL_EVENT_AUDIO_DEVICE_REMOVED` | Hot-plug remove detected, no crash | ☐ |
| M10 | `event.adevice.recording` (bool) | Capture device flag is bool | ☐ |
| M11 | `event.adevice.which` is `SDL_AudioDeviceID` | Device ID type correct | ☐ |
| M12 | Mute/unmute in-game | Audio toggles correctly | ☐ |
| M13 | Volume control | Volume changes take effect | ☐ |

## N. Screen Saver

| # | Test | Expected | Status |
|---|------|----------|--------|
| N1 | `SDL_DisableScreenSaver()` | Screen saver disabled during gameplay | n/a |
| N2 | `SDL_EnableScreenSaver()` | Screen saver re-enabled on exit | n/a |
| N3 | `SDL_ScreenSaverEnabled()` | Returns correct state | n/a |

## O. RmlUi / GUI Rendering

| # | Test | Expected | Status |
|---|------|----------|--------|
| O1 | Main menu renders | All UI elements visible, no missing textures | ok |
| O2 | Menu navigation with keyboard | Arrow keys, Enter, Escape work | dont know, enter only allows me to continue to main game loading |
| O3 | Menu navigation with mouse | Click, hover work | no cursor, no click |
| O4 | In-game HUD renders | Health bars, minimap, command panel visible | ok |
| O5 | Chat box | Opens, accepts text, displays messages | ok |
| O6 | In-game console | Opens, accepts commands, displays output | ok |
| O7 | Unit selection UI | Selection box, unit info panel | ok |
| O8 | Minimap | Renders, clickable for camera movement | ok |
| O9 | Groups panel | Group assign/reveal works | ok |
| O10 | Share box | Screenshot share dialog works | ok |
| O11 | Quit box | Confirm quit dialog works | ok |
| O12 | Start position selector | Map selection UI works | ☐ |
| O13 | RmlUi text input | Text fields in RmlUi accept keyboard input | ☐ |
| O14 | RmlUi clipboard | Copy/paste in RmlUi elements | ☐ |
| O15 | RmlUi event handling | `SDL_EVENT_KEY_DOWN` etc. in RmlUi backend | ☐ |

## P. aGui Elements

| # | Test | Expected | Status |
|---|------|----------|--------|
| P1 | aGui buttons | Clickable, hover state works | ok |
| P2 | aGui windows | Draggable, resizable, closable | ok  |
| P3 | aGui line edits | Text input works | ok |
| P4 | aGui list boxes | Scrollable, selectable | ok |
| P5 | `SDL_GetMouseState` with `float*` | List mouse picking uses float coords | ok |
| P6 | `ev.motion.buttons` | Button state in motion events | ok |
| P7 | `SDL_BUTTON_MASK(x)` macro | Button masks work correctly | ☐ |

## Q. GL Context

| # | Test | Expected | Status |
|---|------|----------|--------|
| Q1 | GL context creation | Context created, no errors | ok |
| Q2 | `SDL_GL_CreateContext` returns `SDL_GLContext` | Opaque pointer, non-NULL | ☐ |
| Q3 | `SDL_GL_MakeCurrent` returns bool | Context made current successfully | ☐ |
| Q4 | `SDL_GL_DestroyContext` | Context destroyed without crash | ☐ |
| Q5 | `SDL_GL_SetAttribute` returns bool | Attributes set successfully | ☐ |
| Q6 | `SDL_GL_GetAttribute` returns bool | Attributes queried successfully | ☐ |
| Q7 | `SDL_GL_SwapWindow` returns bool | Buffer swap succeeds | ☐ |
| Q8 | `SDL_GL_GetProcAddress` returns `SDL_FunctionPointer` | GL functions loaded | ☐ |
| Q9 | GL rendering during gameplay | No artifacting, no crashes | ☐ |
| Q10 | GL rendering after resize | No corrupted viewport | ☐ |
| Q11 | GL rendering after fullscreen toggle | Context survives mode change | ☐ |
| Q12 | Chobby -> Game -> Chobby -> Game | Context survives mode change | ☐ |


## R. Pixel Format and Surface

| # | Test | Expected | Status |
|---|------|----------|--------|
| R1 | `SDL_GetWindowPixelFormat` | Returns valid pixel format | ☐ |
| R2 | `SDL_GetPixelFormatName` | Returns readable format string | ☐ |
| R3 | `SDL_BITSPERPIXEL` macro | Returns correct bits-per-pixel | ☐ |
| R4 | `SDL_CreateSurface(w, h, format)` | Surface creation works | ☐ |
| R5 | `SDL_CreateSurfaceFrom` | Surface from existing pixels works | ☐ |
| R6 | `SDL_DestroySurface` | Surface freed without crash | ☐ |
| R7 | `surface->w`, `surface->h`, `surface->pixels` | Direct field access works | ☐ |
| R8 | `SDL_BlitSurface` | Surface blitting works | ☐ |
| R9 | `SDL_HasRectIntersection` returns bool | Intersection check works | ☐ |
| R10 | `SDL_SetSurfaceAlphaMod` returns bool | Alpha modulation works | ☐ |
| R11 | `SDL_SetSurfaceBlendMode` returns bool | Blend mode set works | ☐ |

## S. SDL Version Reporting

| # | Test | Expected | Status |
|---|------|----------|--------|
| S1 | SDL version logged at startup | Shows SDL3 version (e.g., 3.2.10) | ☐ |
| S2 | `SDL_GetVersion()` returns int | Version decoded via `SDL_VERSIONNUM_MAJOR/MINOR/MICRO` | ☐ |
| S3 | `SDL_GetRevision()` | Returns revision string | ☐ |
| S4 | `SDL_GetCurrentVideoDriver` | Returns driver name (e.g., "x11", "opengl") | ☐ |

## T. Event Loop

| # | Test | Expected | Status |
|---|------|----------|--------|
| T1 | `SDL_PollEvent` returns bool | Event polling works | ☐ |
| T2 | `SDL_EVENT_QUIT` | Quit event handled | ☐ |
| T3 | `SDL_EVENT_TEXT_INPUT` | Text events received | ☐ |
| T4 | `SDL_EVENT_TEXT_EDITING` | Editing events received | ☐ |
| T5 | `SDL_EVENT_MOUSE_WHEEL` | Wheel events received | ☐ |
| T6 | `SDL_EVENT_WINDOW_RESIZED` | Resize events received | ☐ |
| T7 | `SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED` | Pixel resize events received | ☐ |
| T8 | `SDL_EVENT_WINDOW_MAXIMIZED` | Maximize events received | ☐ |
| T9 | `SDL_EVENT_WINDOW_RESTORED` | Restore events received | ☐ |
| T10 | `SDL_EVENT_WINDOW_MINIMIZED` | Minimize events received | ☐ |
| T11 | `SDL_EVENT_WINDOW_SHOWN` / `HIDDEN` | Show/hide events received | ☐ |
| T12 | `SDL_EVENT_WINDOW_FOCUS_GAINED` / `LOST` | Focus events received | ☐ |
| T13 | `SDL_EVENT_WINDOW_CLOSE_REQUESTED` | Close event received | ☐ |
| T14 | `SDL_EVENT_WINDOW_DISPLAY_CHANGED` | Display change event received | ☐ |
| T15 | `SDL_EVENT_WINDOW_MOVED` | Move event received | ☐ |
| T16 | `SDL_EVENT_WINDOW_MOUSE_ENTER` / `LEAVE` | Mouse enter/leave received | ☐ |
| T17 | Event timestamp in nanoseconds | `event.*.timestamp` is ns, not ms | ☐ |
| T18 | `SDL_SetEventEnabled` / `SDL_EventEnabled` | Event enable/disable works | ☐ |
| T19 | No `SDL_WINDOWEVENT` or `SDL_DISPLAYEVENT` | All old event types removed | ☐ |
| T20 | `SDL_PeepEvents` with renamed events | Motion event peeping works | ☐ |

## U. Lua Bindings

| # | Test | Expected | Status |
|---|------|----------|--------|
| U1 | Lua keycodes match SDL2 values | Backward-compatible keycode mapping | ☐ |
| U2 | `LuaHandle` key events | Key events pass through to Lua | ☐ |
| U3 | `LuaUnsyncedRead` SDL queries | SDL info accessible from Lua | ☐ |
| U4 | `LuaUnsyncedCtrl` clipboard | Lua clipboard control works | ☐ |
| U5 | Widget keybindings | Existing widget keybindings still work | ☐ |
| U6 | Gadget keybindings | Existing gadget keybindings still work | ☐ |
| U7 | `SDL1_keysym` translation | SDL1↔SDL3 keycode bimap correct | ☐ |
| U8 | `SDL_SCANCODE_*` constants | Scancodes unchanged | ☐ |
| U9 | `SDL_SCANCODE_COUNT` (was `SDL_NUM_SCANCODES`) | Scancode count correct | ☐ |

## V. Platform-Specific

| # | Test | Expected | Status |
|---|------|----------|--------|
| V1 | `SDL_PROP_WINDOW_WIN32_HWND_POINTER` (Windows) | HWND retrieval works | ☐ |
| V2 | `SDL_PROP_WINDOW_X11_DISPLAY_POINTER` (Linux/X11) | Display* retrieval works | ☐ |
| V3 | `SDL_PROP_WINDOW_X11_WINDOW_NUMBER` (Linux/X11) | Window ID retrieval works | ☐ |
| V4 | `SDL_PROP_WINDOW_WAYLAND_*` (Linux/Wayland) | Wayland handle retrieval works | ☐ |
| V5 | `SDL_PROP_WINDOW_COCOA_WINDOW_POINTER` (macOS) | Cocoa window retrieval works | ☐ |
| V6 | No `SDL_syswm.h` includes | All native handles via properties API | ☐ |
| V7 | `SDL_PLATFORM_*` macros | Platform detection macros correct | ☐ |
| V8 | Window manager helper (platform-specific) | WM operations (always-on-top, etc.) work | ☐ |

## W. Game Flow

| # | Test | Expected | Status |
|---|------|----------|--------|
| W1 | Launch to main menu | Menu loads, no crashes | ☐ |
| W2 | Start single-player game | Game loads, simulation runs | ☐ |
| W3 | Load saved game | Save file loads correctly | ☐ |
| W4 | Pause/unpause | Game pauses and resumes | ☐ |
| W5 | Quit from main menu | Clean exit | ☐ |
| W6 | Quit from in-game (`QuitBox`) | Confirm dialog, clean exit | ☐ |
| W7 | Pre-game lobby screen | Pre-game UI renders and functions | NO MOUSE |
| W8 | Load screen | Loading screen renders, progress shown | ok |
| W9 | Splash screen | Splash renders on startup | ☐ |
| W10 | In-game screenshot | Screenshot saved correctly | ok |
| W11 | AVI video capture | Video capture works (if enabled) | ☐ |
| W12 | Game info display | FPS, unit count, etc. shown correctly | ok |
| W13 | Unit selection and command | Select units, issue commands | ok |
| W14 | Group assignment | Assign/reveal groups | ok |

## X. Edge Cases and Error Handling

| # | Test | Expected | Status |
|---|------|----------|--------|
| X1 | Rapid fullscreen toggle | No crash, no rendering artifacts | ok |
| X2 | Rapid window resize | No crash, viewport catches up | ☐ |
| X3 | Disconnect display while fullscreen | Graceful handling, no crash | ☐ |
| X4 | Audio device hot-plug during gameplay | No crash, audio recovers | ☐ |
| X5 | Keyboard layout change during gameplay | Input continues working | ok |
| X6 | Mouse disconnect/reconnect | No crash | ☐ |
| X7 | Game runs with no audio device | Graceful degradation | ☐ |
| X8 | `SDL_GetError()` after failed operation | Meaningful error message | ☐ |
| X9 | Engine runs with `SDL_WINDOW_MOUSE_GRABBED` | Mouse confined, escape releases | ☐ |
| X10 | Async window operations settle | `SDL_SyncWindow` not needed for normal flow | ☐ |

## Y. Headless Build (if testing server)

| # | Test | Expected | Status |
|---|------|----------|--------|
| Y1 | `spring-headless` launches | No SDL display dependency | ☐ |
| Y2 | `spring-dedicated` launches | Dedicated server starts | ☐ |
| Y3 | Headless SDL stub symbols resolved | No undefined symbol errors | ☐ |
| Y4 | `SDL_SetKeyRepeat` stub | Key repeat stub works | ☐ |
| Y5 | Audio device stubs | `SDL_PauseAudioDevice`, `SDL_ResumeAudioDevice` stubs | ☐ |
| Y6 | `SDL_GetPixelFormatName` stub | Pixel format name stub | ☐ |
| Y7 | Surface stubs | `SDL_BlitSurface`, `SDL_DestroySurface` stubs | ☐ |
| Y8 | `SDL_ShowCursor`/`HideCursor`/`CursorVisible` stubs | Cursor stubs present | ☐ |

---

## Summary

| Section | Tests | Pass | Fail | Skipped |
|---------|-------|------|------|---------|
| A. Window Creation | 13 | | | |
| B. Fullscreen | 8 | | | |
| C. Multi-Monitor | 7 | | | |
| D. High-DPI | 5 | | | |
| E. VSync | 6 | | | |
| F. Keyboard | 18 | | | |
| G. Text Input / IME | 12 | | | |
| H. Mouse | 17 | | | |
| I. Cursor | 12 | | | |
| J. Clipboard | 7 | | | |
| K. Focus | 9 | | | |
| L. Camera | 7 | | | |
| M. Audio | 13 | | | |
| N. Screen Saver | 3 | | | |
| O. RmlUi / GUI | 15 | | | |
| P. aGui | 7 | | | |
| Q. GL Context | 11 | | | |
| R. Pixel Format | 11 | | | |
| S. Version Reporting | 4 | | | |
| T. Event Loop | 20 | | | |
| U. Lua Bindings | 9 | | | |
| V. Platform-Specific | 8 | | | |
| W. Game Flow | 14 | | | |
| X. Edge Cases | 10 | | | |
| Y. Headless | 8 | | | |
| **Total** | **254** | | | |

---

## Failure Report Template

For each failed test, record:

```
Test ID: [e.g., F8]
Description: [brief description]
Expected: [what should happen]
Actual: [what happened]
Log output: [relevant console/log lines]
Reproduction steps: [how to reproduce]
Severity: [blocker / major / minor / cosmetic]
```
