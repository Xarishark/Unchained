# Settings

`settings.toml` beside `Unchained.exe` holds every setting. The game writes it with the current values; edit it with
the game closed. A missing key takes its default. The Configuration app will set these too.

## Display

| Key | Default | What it does |
|---|---|---|
| `display_mode` | `"borderless"` | `"fullscreen"`, `"borderless"` or `"windowed"` |
| `width`, `height` | `0` | Window or fullscreen size; `0` uses the desktop's |
| `render_percent` | `100` | Render scale against what the window shows; under 100 renders smaller (FSR 1 upscales), 200 and up supersample. Never under the console's own resolution unless asked |
| `widescreen` | `"expand"` | `"expand"`: a wider view at 16:9 and beyond (up to 32:9), HUD kept in shape. `"off"`: 4:3 with bars. `"console"`: what the Xbox did with a widescreen TV (stretched) |
| `hud_edge` | `100` | How far the HUD moves out to the screen's edges: `0` where the console put it, `100` at the edges like a modern HUD |
| `hud_visible` | `true` | `false` hides the HUD pieces at the edges (menus stay) |
| `fov` | `60` | Vertical field of view of the gameplay camera in degrees (the game's is 60); cutscene cameras keep theirs |
| `draw_distance` | `100` | The cameras' far clip in percent of the game's (100 to 1000) |
| `vsync_mode` | `"on"` | `"off"`, `"on"`, `"adaptive"` (a tear instead of a held frame when late), `"fast"` (no wait, no tear) or `"vrr"` (for G-Sync / FreeSync, capped 3 under the refresh) |
| `fps_cap` | `60` | Frame rate cap; `0` none, `1` the monitor's refresh rate |
| `perf_overlay` | `false` | The performance overlay at start (F10 shows or hides it in game) |

## Picture

| Key | Default | What it does |
|---|---|---|
| `graphics_preset` | `"custom"` | `"og"` (OG Graphics in the Configuration app: the console as it was: its own 640x480 at 4:3 shown in your window mode, every setting at its default, clean menu edges off; the fixes stay), `"original"` (the console's picture, sharp), `"low"`, `"high"`, `"max"`; anything else leaves each setting as written |
| `renderer` | `"vulkan"` | Windows: `"vulkan"` or `"d3d12"`. D3D12 takes over when Vulkan cannot start. Linux always draws with Vulkan |
| `upscaler` | `1` | `0` bilinear, `1` AMD FSR 1 when rendering smaller than the window |
| `sharpen` | `0` | FSR 1 RCAS sharpening, 0 to 100 |
| `hdr` | `false` | Windows: HDR10 output when Windows has the display in HDR (Use HDR in display settings); SDR otherwise. Set aside for now: read as `false` whatever the file says (D3D12's HDR10 output stays in the code for later) |
| `hdr_paper_white` | `200` | Nits for the game's white, 80 to 500 |
| `hdr_peak` | `0` | Nits for the brightest highlights, `0` the display's own peak |
| `hdr_highlights` | `50` | Percent the brightest parts are stretched toward the peak, `0` the SDR look |
| `anisotropic_filtering` | `16` | Texture filtering at angles: 0 (as the game asks), 2, 4, 8, 16 |
| `character_shadows` | `true` | `false` turns the characters' shadows off |
| `shadow_resolution` | `2048` | Pixels across the characters' shadow map (the console's is 256); `0` follows the render scale |
| `grade_exposure` | `0` | Hundredths of a stop, -300 to 300 |
| `grade_contrast`, `grade_saturation` | `100` | Percent |
| `grade_temperature`, `grade_tint` | `0` | -100 to 100 |
| `colour_filter` | `0` | Colour vision: `1` protanopia, `2` deuteranopia, `3` tritanopia (colours shifted so they stay apart) |
| `colour_filter_strength` | `100` | Percent |
| `colour_lut` | `""` | A colour table (`.cube`, as editors export them) in `luts\` in the data folder, applied after the grade |
| `custom_textures` | `true` | Texture packs (see [MODS.md](MODS.md)) |
| `ui_upscale` | `true` | Flat 2D art (text, menu frames and boxes, the HUD's terminal icons) drawn 4x with clean edges; glows, gradients, pictures and noise stay as they are |
| `dump_textures`, `texture_dump_format` | `false`, `0` | Write each texture once to `textures\dumps\` (`0` PNG, `1` DDS) |

## Controls

Keyboard and mouse play is off in every version for now: the game is played with a controller, and `[keys]`,
`mouse_camera`, `mouse_sensitivity` and `button_prompts = 4` (key caps) do nothing until it comes back
(`src/common/keyboard_mouse.h`). The F-key shortcuts (F1 menu, F10 overlay, F12 screenshot) still work.

| Key | Default | What it does |
|---|---|---|
| `[keys]` | see below | Each action's keys and mouse buttons |
| `mouse_camera` | `false` | A click in the window captures the mouse to turn the camera (cursor hidden); Alt or switching away gives it back |
| `mouse_sensitivity` | `100` | Percent |
| `camera_speed` | `100` | The right stick's camera speed in percent (up to 300) |
| `invert_camera_x`, `invert_camera_y` | `false` | |
| `vibration` | `100` | Rumble strength in percent; `0` off |
| `button_prompts` | `0` | The game's button prompts: `0` for the device in use (a controller's maker, or the keyboard with the bound keys), `1` Xbox (today's pads: lettered buttons, LB RB LT RT), `2` PlayStation, `3` Nintendo, `4` keyboard, `5` the original Xbox's own glyphs |

Default keys: W A S D move, mouse or I J K L camera, Space jump (A), left mouse or F attack (X), right mouse or Left
Shift B, R Y, Q / E triggers, C / V black / white, Enter Start, Esc Back, arrows the d-pad. Key names: `A`-`Z`,
`0`-`9`, `F1`-`F12`, `Up` `Down` `Left` `Right`, `Space` `Enter` `Escape` `Tab` `Backspace`, `LShift` `RShift`
`LCtrl` `RCtrl` `LAlt` `RAlt`, `Mouse1`-`Mouse5`. For example:

```toml
[keys]
jump = ["Space"]
attack = ["F", "Mouse1"]
```

## Sound and saves

| Key | Default | What it does |
|---|---|---|
| `audio_output` | `0` | `0` automatic, then stereo, surround, mono |
| `save_folder` | `""` | Where the console's hard disk (your saves) lives; empty is `hdd` beside the exe |
| `save_backups` | `5` | Copies of your saves made at each start in `save backups\`, the newest kept; `0` none |
| `save_profile` | `""` | A named set of saves in `profiles\<name>`; empty is the main one |
