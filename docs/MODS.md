# Mods: texture packs

Unchained can draw your own images in place of the game's textures, at any resolution.

1. Set `dump_textures = true` in `settings.toml` and play: every texture the game uses is written once to
   `textures\dumps\` beside `Unchained.exe`, named like `256x256_0123456789abcdef_DXT1.png` (size, a hash of the
   texture's contents, its console format). `texture_dump_format = 1` writes DDS instead of PNG.
2. Edit or upscale the ones you want. Keep the 16-digit hash in the file name; the size can change (a 256x256 texture
   may come back as 1024x1024).
3. Put the results in `textures\replacement\` (any folder depth). PNG and DDS both work.
4. With `custom_textures = true` (the default) the game draws them instead. Changes are picked up while the game runs.

Turn `dump_textures` off again once you have what you need: dumping costs time on each new texture.

Nothing from the game itself may be shared in this repository or its releases. Share packs only as your own work.
