# Missing ride-world art

The procedural ride world ([RideWorld.md](RideWorld.md)) needs 34 art roles, listed in
`assets/World/manifest.json`. 27 of them have no real file yet, so the game draws a placeholder in code for each
(`RideWorld::GeneratePlaceholder`, named `placeholder_<role>` in logs). The game runs fine without them.

**To replace a placeholder:** save a PNG at the expected path below, or edit the role's `file` in the manifest to
point at an existing asset. Nothing else changes; sizes, frame layout and anchors are read from the manifest.

- **Size:** sizes are per frame. Frames (variants or crank positions) run left to right and rows top to bottom.
- **Alpha:** straight (not premultiplied).
- **Sampling:** nearest-neighbour, so pixel art stays crisp.
- **Props:** drawn as camera-facing billboards with the base at the bottom centre, except guardrails, buildings and
  the banner, which face along / across the road.
- **Placement:** world size comes from the biome's `height` range, and the width follows the frame's aspect ratio.
- **Rider frames per revolution:** 8 is the minimum (45 degree steps), 12 is recommended (and what the placeholder
  uses), and more is smoother. The legs can't strobe backwards at any real cadence (that needs more than half a
  revolution per displayed frame, over 1800 rpm at 60 fps), but a spoked side-on wheel can. See RideWorld.md.
- **Already mapped to existing assets** (not listed below):
  - `rider_shadow` -> `Actors/Character/Shadow.png`.
  - `sky_clouds` -> `FX/Particle/Clouds.png`.
  - `particle_rain` -> `FX/Particle/Rain.png`.
  - `particle_snow` -> `FX/Particle/Snow.png`.
  - `tree_broadleaf` and `tree_pine` -> the free-standing trees on the bottom row of
    `Backgrounds/Tilesets/TilesetNature.png`, cut out with manifest `regions`.
  - `rider_explorer` -> the Explorer's `upRunAnimation` and `upIdleAnimation` frames in `Actors/explorer.png`.
- **Not needed by the shipped rider:** `rider.json` draws the Explorer over the bike, so `rider_legs`, `rider_torso`
  and `rider_sheet` are only used if you switch back to the drawn rider rig or to a flat sheet. The bike itself
  (`rider_frame`, `rider_wheel`) is still a placeholder.

This list is also logged once at startup, and `Struktur.exe --list-missing-assets` prints it (this exact table) and
quits.

| Role | Category | Expected file | Size and format | Frames | Anchor | Placeholder used | Notes |
|---|---|---|---|---|---|---|---|
| `rider_frame` | rider | `assets/World/Rider/rider_frame.png` | 64 x 96; PNG, RGBA (straight alpha) | 1 | 32, 94 | `placeholder_rider_frame` (rider_frame) | Bike seen from behind (seat post, saddle, handlebar, derailleur) without wheels or rider. Layer 2 of 4, drawn over the wheel. |
| `rider_wheel` | rider | `assets/World/Rider/rider_wheel.png` | 6 x 32, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_rider_wheel` (rider_wheel) | Rear tyre tread strip, tileable vertically - scrolled with the wheel angle (rear view). For a side-on wheel instead, set motion 'rotate' and 'spokes' in rider.json and draw the whole wheel. |
| `rider_legs` | rider | `assets/World/Rider/rider_legs.png` | 64 x 96 per frame (sheet 768 x 288); PNG, RGBA (straight alpha) | 12 x 3 rows (12 per crank revolution) | 32, 94 | `placeholder_rider_legs` (rider_legs) | Legs from behind. Row 0: seated pedalling, frame 0 = right crank at 12 o'clock, then clockwise (seen from the right) in equal steps. Row 1: out of the saddle, same phases. Row 2: column 0 coasting (cranks level), column 1 idle (one foot down). Minimum 8 frames per revolution, 12 recommended. |
| `rider_torso` | rider | `assets/World/Rider/rider_torso.png` | 64 x 96; PNG, RGBA (straight alpha) | 1 | 32, 94 | `placeholder_rider_torso` (rider_torso) | Back, arms to the bars, helmet. Drawn last; shifted up 7 px out of the saddle (rider.json outOfSaddleOffsetY). |
| `rider_sheet` | rider | `assets/World/Rider/rider_sheet.png` | 64 x 96 per frame (sheet 768 x 288); PNG, RGBA (straight alpha) | 12 x 3 rows (12 per crank revolution) | 32, 94 | `placeholder_rider_sheet` (rider_legs) | OPTIONAL alternative to the four layers: one flat sheet (whole bike + rider per frame), same rows as rider_legs. Only used when assets/World/Rider/rider.json has kind 'sheet'. |
| `tree_palm` | props | `assets/World/Props/tree_palm.png` | 96 x 128 per frame (sheet 288 x 128); PNG, RGBA (straight alpha) | 3 | bottom centre | `placeholder_tree_palm` (tree_palm) | Palm, 3 variants (coastal). |
| `cactus` | props | `assets/World/Props/cactus.png` | 48 x 96 per frame (sheet 96 x 96); PNG, RGBA (straight alpha) | 2 | bottom centre | `placeholder_cactus` (cactus) | Desert cactus, 2 variants. |
| `bush` | props | `assets/World/Props/bush.png` | 64 x 40 per frame (sheet 192 x 40); PNG, RGBA (straight alpha) | 3 | bottom centre | `placeholder_bush` (bush) | Low shrub, 3 variants. |
| `rock` | props | `assets/World/Props/rock.png` | 64 x 40 per frame (sheet 192 x 40); PNG, RGBA (straight alpha) | 3 | bottom centre | `placeholder_rock` (rock) | Boulder, 3 variants. |
| `grass` | props | `assets/World/Props/grass.png` | 32 x 24 per frame (sheet 96 x 24); PNG, RGBA (straight alpha) | 3 | bottom centre | `placeholder_grass` (grass) | Grass tuft, 3 variants; drawn only near the road. |
| `sign_post` | props | `assets/World/Props/sign_post.png` | 48 x 64 per frame (sheet 96 x 64); PNG, RGBA (straight alpha) | 2 | bottom centre | `placeholder_sign_post` (sign) | Roadside sign on a post, 2 variants (no real place names or brands). |
| `guardrail` | props | `assets/World/Props/guardrail.png` | 128 x 24; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_guardrail` (guardrail) | One 4 m guardrail section with posts at both ends. Drawn as a quad along the road on the outside of bends. |
| `lamp_post` | props | `assets/World/Props/lamp_post.png` | 32 x 128; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_lamp_post` (lamp) | Street lamp; the lamp head overhangs to the left (towards the road on the right-hand side). |
| `building` | props | `assets/World/Props/building.png` | 128 x 128 per frame (sheet 384 x 128); PNG, RGBA (straight alpha) | 3 | bottom centre | `placeholder_building` (building) | House / small building front, 3 variants; faces the road. |
| `finish_banner` | props | `assets/World/Props/finish_banner.png` | 256 x 96; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_finish_banner` (banner) | Banner spanning the road on two poles (workout complete). Poles at the far left and right edges. |
| `sky_sun` | sky | `assets/World/Sky/sun.png` | 64 x 64; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_sky_sun` (sun) | Sun disc with glow, additive. |
| `sky_moon` | sky | `assets/World/Sky/moon.png` | 64 x 64; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_sky_moon` (moon) | Moon disc, alpha blended. |
| `sky_stars` | sky | `assets/World/Sky/stars.png` | 256 x 128, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_sky_stars` (stars) | Star field, tileable both ways; mapped around the sky by heading and elevation. Alpha = brightness. |
| `mountains_far` | sky | `assets/World/Sky/mountains_far.png` | 512 x 128, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_mountains_far` (mountains) | Far mountain silhouette, white on transparent (tinted per biome), tileable horizontally. Parallax layer 1. |
| `mountains_near` | sky | `assets/World/Sky/mountains_near.png` | 512 x 128, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_mountains_near` (mountains) | Nearer hills silhouette, white on transparent, tileable horizontally. Parallax layer 2. |
| `terrain_grass` | terrain | `assets/World/Terrain/grass.png` | 64 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_terrain_grass` (noise) | Detail texture multiplied over the vertex colour; keep it near-grey/neutral. |
| `terrain_rock` | terrain | `assets/World/Terrain/rock.png` | 64 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_terrain_rock` (noise) | Detail texture for steep slopes. |
| `terrain_snow` | terrain | `assets/World/Terrain/snow.png` | 64 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_terrain_snow` (noise) | Detail texture for snow. |
| `terrain_sand` | terrain | `assets/World/Terrain/sand.png` | 64 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_terrain_sand` (noise) | Detail texture for sand / beaches. |
| `water` | terrain | `assets/World/Terrain/water.png` | 64 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_water` (water) | Water surface detail, scrolled slowly; multiplied with the biome's water colour. |
| `road_asphalt` | road | `assets/World/Road/asphalt.png` | 64 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_road_asphalt` (noise) | Asphalt detail, multiplied with the biome's road colour; tiled every 4 m. |
| `road_markings` | road | `assets/World/Road/markings.png` | 16 x 64, tileable; PNG, RGBA (straight alpha) | 1 | bottom centre | `placeholder_road_markings` (markings) | Paint texture inside the lines (the line pattern itself is computed in the shader). |
