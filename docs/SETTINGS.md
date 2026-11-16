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

## Picture

| Key | Default | What it does |
|---|---|---|
| `graphics_preset` | `"custom"` | `"original"` (the console's picture, sharp), `"low"`, `"high"`, `"max"`; anything else leaves each setting as written |
| `upscaler` | `1` | `0` bilinear, `1` AMD FSR 1 when rendering smaller than the window |
| `sharpen` | `0` | FSR 1 RCAS sharpening, 0 to 100 |
| `anisotropic_filtering` | `16` | Texture filtering at angles: 0 (as the game asks), 2, 4, 8, 16 |
| `character_shadows` | `true` | `false` turns the characters' shadows off |
| `shadow_resolution` | `2048` | Pixels across the characters' shadow map (the console's is 256); `0` follows the render scale |
| `grade_exposure` | `0` | Hundredths of a stop, -300 to 300 |
| `grade_contrast`, `grade_saturation` | `100` | Percent |
| `grade_temperature`, `grade_tint` | `0` | -100 to 100 |
| `colour_filter` | `0` | Colour vision: `1` protanopia, `2` deuteranopia, `3` tritanopia (colours shifted so they stay apart) |
| `colour_filter_strength` | `100` | Percent |
| `custom_textures` | `true` | Texture packs (see [MODS.md](MODS.md)) |
| `dump_textures`, `texture_dump_format` | `false`, `0` | Write each texture once to `textures\dumps\` (`0` PNG, `1` DDS) |

## Controls

| Key | Default | What it does |
|---|---|---|
| `[keys]` | see below | Each action's keys and mouse buttons |
| `mouse_camera` | `true` | The mouse turns the camera while the window has focus (cursor hidden) |
| `mouse_sensitivity` | `100` | Percent |
| `camera_speed` | `100` | The right stick's camera speed in percent (up to 300) |
| `invert_camera_x`, `invert_camera_y` | `false` | |
| `vibration` | `100` | Rumble strength in percent; `0` off |

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
