"""Builds the opening animation sheet for a double door from its single closed frame.

    python tools/door_open_sheet.py [source.png] [output.png] [--frames N] [--scale S]

Defaults: assets/Tiles/Wall Decor/Doors.png -> assets/Tiles/Wall Decor/DoorsOpening.png, 8 frames.

The source is one frame of a double door seen face on: two leaves, hinged at the frame's outer edges, meeting in the
middle. Each frame swings both leaves a little further in (away from the viewer). A leaf turned by angle a:
  - narrows towards its hinge to cos(a) of its width,
  - skews: its free edge recedes, so it is drawn shorter and shifted up (further away in the game's top-down view),
  - darkens a little as it turns away from the light, snapped back to the door's own palette so it stays pixel art.
The bottom row (the threshold) stays put, and the doorway the leaves uncover is transparent, so the floor behind
shows through. Frame 0 is the closed door unchanged; the last frame is fully open, leaves edge-on at the hinges.

The sheet is one row of frames - GameObjects/Door.wren plays it (columns = frame count). --scale writes an enlarged
preview copy (frames spaced apart on grey) next to the output for checking by eye.
"""
import argparse
import math
import os

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
DOORS_DIR = os.path.join(HERE, "..", "assets", "Tiles", "Wall Decor")

OPEN_ANGLE = 80.0      # degrees a leaf has turned in the last frame
RECEDE_TOP = 1.0       # px the free edge's top drops at 90 degrees
RECEDE_BOTTOM = 3.0    # px the free edge's bottom rises at 90 degrees (more than the top - a skew upwards)
DARKEN = 0.12          # brightness lost at 90 degrees (kept low - snapping to the palette exaggerates it)
THRESHOLD_ROWS = 1     # rows at the bottom left in place


def palette_of(image):
    px = image.load()
    return sorted({px[x, y] for x in range(image.width) for y in range(image.height) if px[x, y][3] > 0})


def nearest(palette, color):
    return min(palette, key=lambda p: sum((a - b) ** 2 for a, b in zip(p[:3], color[:3])))


def leaf_frame(source, out, palette, x0, x1, hinge_left, angle, cache):
    """Draws the leaf occupying source columns [x0, x1) turned by angle (radians) onto out."""
    src = source.load()
    dst = out.load()
    width = x1 - x0
    height = source.height - THRESHOLD_ROWS
    drawn = max(1, round(width * math.cos(angle)))
    s = math.sin(angle)
    shade = 1.0 - DARKEN * s
    for dx in range(drawn):
        # t: 0 at the hinge, 1 at the free edge - in the drawn (narrowed) leaf.
        t = (dx + 0.5) / drawn
        top = t * RECEDE_TOP * s
        bottom = height - t * RECEDE_BOTTOM * s
        u = int(t * width)                       # source column within the leaf, measured from its hinge
        sx = x0 + u if hinge_left else x1 - 1 - u
        ox = x0 + dx if hinge_left else x1 - 1 - dx
        for oy in range(height):
            if oy + 0.5 < top or oy + 0.5 > bottom:
                continue
            v = (oy + 0.5 - top) / (bottom - top)
            sy = min(height - 1, int(v * height))
            c = src[sx, sy]
            if c[3] == 0:
                continue
            if shade < 1.0:
                key = (c, round(shade, 3))
                if key not in cache:
                    cache[key] = nearest(palette, tuple(int(ch * shade) for ch in c[:3]) + (255,))
                c = cache[key]
            dst[ox, oy] = c


def build(source, frames):
    w, h = source.size
    palette = palette_of(source)
    half = w // 2
    sheet = Image.new("RGBA", (w * frames, h), (0, 0, 0, 0))
    cache = {}
    for i in range(frames):
        angle = math.radians(OPEN_ANGLE * i / (frames - 1))
        frame = Image.new("RGBA", (w, h), (0, 0, 0, 0))
        if i == 0:
            frame.paste(source, (0, 0))
        else:
            leaf_frame(source, frame, palette, 0, half, True, angle, cache)
            leaf_frame(source, frame, palette, half, w, False, angle, cache)
            # The threshold doesn't move.
            frame.paste(source.crop((0, h - THRESHOLD_ROWS, w, h)), (0, h - THRESHOLD_ROWS))
        sheet.paste(frame, (i * w, 0))
    return sheet


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", nargs="?", default=os.path.join(DOORS_DIR, "Doors.png"))
    parser.add_argument("output", nargs="?", default=os.path.join(DOORS_DIR, "DoorsOpening.png"))
    parser.add_argument("--frames", type=int, default=8)
    parser.add_argument("--scale", type=int, default=0, help="also write an enlarged preview (e.g. 8)")
    args = parser.parse_args()

    source = Image.open(args.source).convert("RGBA")
    sheet = build(source, args.frames)
    sheet.save(args.output)
    print(f"Wrote {args.output} ({args.frames} frames of {source.width}x{source.height})")
    if args.scale > 0:
        preview = os.path.splitext(args.output)[0] + "_preview.png"
        w, h = source.size
        gap = 4
        board = Image.new("RGBA", (args.frames * (w + gap) + gap, h + 2 * gap), (90, 90, 90, 255))
        for i in range(args.frames):
            frame = sheet.crop((i * w, 0, (i + 1) * w, h))
            board.alpha_composite(frame, (gap + i * (w + gap), gap))
        board.resize((board.width * args.scale, board.height * args.scale), Image.NEAREST).save(preview)
        print(f"Wrote {preview}")


if __name__ == "__main__":
    main()
