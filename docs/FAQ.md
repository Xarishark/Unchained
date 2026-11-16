# FAQ

**Is the game included?** No. Unchained contains no game files or code. You need your own copy of the original Xbox
*Whiplash* (USA): its disc image (`.iso`) or extracted files.

**Which release?** Only the one whose `default.xbe` has SHA-1 `3ea30323cec5734ddd4d535b5a269786b6900dc4`. Another one
is refused with a message naming what was found. The xiso-tool patched copy is a different file; use the untouched
image.

**What languages?** The USA release is English only (text and voices).

**How do I set it up or change settings?** Run `Unchained.exe --configure` (a first run without the game's files
opens it by itself): GAME FILES takes your disc image or folder, the other pages hold the settings.

**The first start takes a while.** The game's code is built for your PC once; later starts are quick.

**The picture is blurry / small.** The game renders at the window's size; check `render_percent` is 100 or more in
`settings.toml`. See [SETTINGS.md](SETTINGS.md).

**The picture is stretched / has bars.** `widescreen = "expand"` (the default) shows more at the sides at 16:9 and
wider; `"off"` gives the console's 4:3 with bars.

**The HUD sits at the screen's edges.** That is `hud_edge = 100`. Set `0` for the console's positions.

**Characters look dark in some rooms.** That is the Xbox release's own level lighting (the lobby's ambient light is
green in the game's data); the PS2 version you may have seen lights them differently.

**Can I use a keyboard and mouse?** Yes, rebindable in `settings.toml`'s `[keys]`; the mouse turns the camera.

**Can I keep my game files and saves somewhere else?** Set `UNCHAINED_DATA_ROOT` to a folder: the game files,
saves, settings, logs and caches go there; the program stays where it is.

**How do I take a screenshot?** F12 saves the picture as a PNG in `screenshots\`. F10 shows the performance
overlay.

**Where are my saves?** In `hdd\E\UDATA\45530011` beside the exe (or `save_folder`). Each start backs them up to
`save backups\`.

**It crashed.** Please open a bug report with the newest `logs\unchained_NNN.log` and, if there is one, the newest
file in `CrashDumps\`.
