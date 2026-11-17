# Light beam flicker (open)

Found while fixing the widescreen pop-in (TODO 18, 2026-11-16). Everything else that popped is fixed; this one is
not, and is written down so anyone can pick it up.

## What the player sees

In the training lab (the atrium by the cafeteria and the escalator) the light beams that wall lamps throw straight up
vanish and come back. It depends on the player's distance from the lamp, not on where the lamp is on screen (middle or
edge). Seen at 2560x1440, `widescreen = "expand"`, `fov = 80`, `draw_distance = 291`.

It is **not**:

- the renderer's per-object cull: with the sphere test passing everything (`UNCHAINED_DBG_NOCULL`) it still happens,
  and the beams' drawables never reach that test anyway (see below);
- portal culling: with every portal test reporting "all visible" (`UNCHAINED_DBG_PORTALS`) it still happens;
- the frame rate: the same at a 60 fps cap as at the monitor's rate;
- the beam's own ray test or fade (both logged, see below).

Not yet checked: whether it happens at `fov = 60` and 4:3, and whether the console does it (the reference video).

## How the game draws a beam

- `LightBeamScript` (vtable 0x3150FC) has a drawable, `DrawableAttr<LightBeamScript>` (vtable 0x3150EC). Its draw
  (vtable +4, 0xE0190) stores the pass id at drawable+0x18 and calls the beam's draw 0xDD060 on drawable+0x30.
- 0xDD060 reads a record at beam+0x0C through offsets in globals: enabled byte at `[0x434AA0]`, a count at
  `[0x434AB0]`, a length at `[0x434AAC]` (400 in the lab). Then:
  - 0xDCA10, the ray test: casts with 0x142A00 (cdecl: from, to, ?, hit info, mask, 2) from beam+0xC0 toward
    beam+0xA0 with mask 0xFFFFFFFF; if the object hit was not drawn last frame (its +0x14C below the counter at
    0x36320C minus 1) it casts again with mask 0x8000. The end point moves to the hit.
  - 0xDCD30, the fade: the dot of the camera's forward (0x17F450) and the beam's direction (beam+0xB4).
  - by type at `[0x434AB4]`: 0xDE060, 0xDEBF0 or 0xDF8F0, which build the cone (0xDD9C0: 240 vertices, quads,
    `vs FFFEFFED`, additive blend 1/302/1, no depth write, no culling, a 16x16 texture) and a sprite at the start
    (0xDD290).
- Room drawables are drawn by 0x194200: it skips a drawable whose flags at +0x14 have bit 0 or 1 set, or whose +0x18
  already holds this pass id; with flag bit 2 set it skips the sphere test (0x16D680) and draws. The beams are never
  sphere tested (the probe matched drawables by their vtable and never saw one there), so most likely they have bit 2
  set; the flags were not read directly.

## What the probe showed

`tools/probes/cull_probe.cpp` (copy into `src/game/` to build it in; switches in its header). Logs from the user's runs:

- **The beam's draw is not called at all while it is gone.** `beamdraw` gaps of 380 ms, 1.5 s, 4 s, up to 13 s
  between calls of 0xDD060 for one beam, against about 5 ms (16.7 ms at 60 fps) while it shows. So the beam is
  dropped before 0xDD060, upstream of everything above.
- F3 captures (`C:\Unchained-private\turns\beam-f3`, 20:56:59 to 20:57:17): two cone draws in most frames, one in
  205703, 205705, 205708 and 205714.
- The ray test runs every few frames and its length changes as the beam's hit moves between objects (270 to 570); it
  never needed the re-cast (`recast 0` throughout). The fade changes smoothly. Neither depends on distance.
- The lamps' lens flares (`GlowLight`, vtable 0x307D20, update 0x241B70, handed to the renderer at vtable +0x58,
  0x16D860, drawn around 0x1D5E20) draw 1 to 10 sprite quads (textures: a soft dot, a star) and vary between frames
  from the same spot. That may be meant (a flare dims as it is occluded) and is a separate question.

## Where to look next

The beam is skipped either because its drawable is not in a room list that gets drawn that pass, or because its
flags at +0x14 have bit 0 or 1 set. In order:

1. Log the beam drawable's +0x14 flags each frame (from 0xE0190's object, or by walking the room lists in 0x194200).
   If bit 0 or 1 comes and goes with distance, find who sets them: the sim LOD (`SimLODAlgorithm`, vtable 0x308528;
   `ModelSizeLOD` 0x308538 and `ModelSizeAndVelLOD` 0x308548, scoring with 0x251A30 / 0x251B70, which use the
   camera distance and a scale from the camera's near and fov cached once at 0x4340A0 by 0x2517C0) is the first
   suspect, since it is distance based.
2. Otherwise check which room the drawable is listed in. A 400-long beam pointing up may be listed in a cell the
   camera's cell walk (0x169600 / 0x17C8E0) does not reach from some positions, or be moved between cells.
3. Compare with `fov = 60` and with the console (reference video) before changing the game: it may be the original's
   behaviour.

Hooks used by the probe, for anyone writing their own: 0x16D680 (sphere test, `al` non-zero = culled), 0x1BC980
(portal test, result byte 1 = all visible), 0xDCA10, 0xDCD30, 0xDD060, 0x142A00 (only from 0xDCABE / 0xDCB57; other
callers pass a null hit info).
