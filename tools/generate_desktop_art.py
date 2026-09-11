#!/usr/bin/env python3
"""
Draws the whole visual kit for MCF's desktop shell and writes it out as PNG
plus a hand-written .edds.meta, which is the same pipeline the phone art uses:
drop the pair in the addon, focus the Workbench, and the watcher builds the
.edds against our own GUID. No import dialog, no GUID we did not choose.

Everything is drawn at 4x and downsampled, because Enfusion will not antialias
an icon for us and a 64px shape drawn directly has stair-stepped edges that are
very visible on a dark panel.
"""
import os
import random
from PIL import Image, ImageDraw

OUT = os.environ.get("MCF_ART_OUT", "UI/images/MCF_Desktop")
SS = 4  # supersample

# ------------------------------------------------------------------ palette
# The same values the phone uses, so the two devices read as one framework.
ACCENT     = (194,  99,  20, 255)
ACCENT_HI  = (230, 138,  46, 255)
STEEL      = (141, 154, 165, 255)
STEEL_DK   = ( 75,  85,  96, 255)
STEEL_DKR  = ( 47,  55,  64, 255)
PAPER      = (236, 239, 242, 255)
PAPER_DIM  = (223, 228, 232, 255)
GREEN      = ( 79, 168, 106, 255)
GREEN_LT   = (111, 191, 122, 255)
GREEN_PALE = (217, 240, 224, 255)
BLUE       = ( 92, 143, 196, 255)
BLUE_LT    = (168, 200, 232, 255)
BLUE_MD    = (127, 173, 216, 255)
GOLD       = (217, 154,  62, 255)
GOLD_LT    = (240, 180,  85, 255)
YELLOW     = (240, 196,  76, 255)
SKIN       = (232, 201, 139, 255)
TERM_BG    = ( 27,  31,  35, 255)
TERM_BAR   = ( 58,  65,  72, 255)
INK        = (207, 214, 219, 255)
SHELL_DK   = ( 32,  36,  40, 255)
SHELL_HI   = ( 42,  47,  52, 255)


# ------------------------------------------------------------------ helpers
def canvas(size):
    im = Image.new("RGBA", (size * SS, size * SS), (0, 0, 0, 0))
    return im, ImageDraw.Draw(im)


def done(im, name, size):
    im = im.resize((size, size), Image.LANCZOS)
    im.save(os.path.join(OUT, name + ".png"))
    return name


def rr(d, box, r, fill):
    x0, y0, x1, y1 = [v * SS for v in box]
    d.rounded_rectangle([x0, y0, x1, y1], radius=r * SS, fill=fill)


def rect(d, box, fill):
    x0, y0, x1, y1 = [v * SS for v in box]
    d.rectangle([x0, y0, x1, y1], fill=fill)


def ell(d, box, fill):
    x0, y0, x1, y1 = [v * SS for v in box]
    d.ellipse([x0, y0, x1, y1], fill=fill)


def poly(d, pts, fill):
    d.polygon([(x * SS, y * SS) for x, y in pts], fill=fill)


def line(d, pts, fill, w, joint="curve"):
    d.line([(x * SS, y * SS) for x, y in pts], fill=fill, width=int(w * SS), joint=joint)


def arc(d, box, a0, a1, fill, w):
    x0, y0, x1, y1 = [v * SS for v in box]
    d.arc([x0, y0, x1, y1], a0, a1, fill=fill, width=int(w * SS))


made = []

# ============================================================== app icons, 64
S = 64

# --- Files: the file manager. A folder with a sheet lifting out of it, so it
# --- is not mistaken for the plain folder icon sitting next to it on the desk.
im, d = canvas(S)
rr(d, (6, 13, 58, 50), 5, GOLD)
poly(d, [(6, 18), (24, 18), (30, 25), (6, 25)], GOLD)
rr(d, (20, 4, 46, 34), 3, PAPER)          # the sheet stands well clear of the
rr(d, (24, 10, 42, 12.4), 1.2, STEEL)     # rim, or at panel size this icon is
rr(d, (24, 16, 42, 18.4), 1.2, STEEL)     # the plain folder with a smudge on it
rr(d, (24, 22, 35, 24.4), 1.2, STEEL)
rr(d, (6, 26, 58, 55), 5, GOLD_LT)
made.append(done(im, "icon_files", S))

# --- Folder: the plain one.
im, d = canvas(S)
rr(d, (6, 13, 58, 52), 5, GOLD)
poly(d, [(6, 18), (24, 18), (30, 25), (6, 25)], GOLD)
rr(d, (6, 22, 58, 55), 5, GOLD_LT)
made.append(done(im, "icon_folder", S))

# --- Mail
im, d = canvas(S)
rr(d, (5, 13, 59, 51), 5, BLUE)
poly(d, [(5, 19), (32, 37), (59, 19), (59, 15), (55, 13), (9, 13), (5, 15)], BLUE_LT)
poly(d, [(5, 49), (24, 33), (32, 38), (40, 33), (59, 49), (59, 51), (5, 51)], BLUE_MD)
made.append(done(im, "icon_messages_dummy", S))  # placed then renamed below
os.rename(os.path.join(OUT, "icon_messages_dummy.png"), os.path.join(OUT, "icon_mail.png"))
made[-1] = "icon_mail"

# --- Messages
im, d = canvas(S)
rr(d, (8, 8, 56, 42), 8, GREEN)
poly(d, [(14, 38), (30, 38), (14, 55)], GREEN)
rr(d, (17, 18, 47, 22), 2, GREEN_PALE)
rr(d, (17, 27, 38, 31), 2, GREEN_PALE)
made.append(done(im, "icon_messages", S))

# --- Contacts
im, d = canvas(S)
rr(d, (10, 7, 54, 57), 6, STEEL_DK)
rect(d, (14, 7, 20, 57), STEEL_DKR)
ell(d, (27, 17, 45, 35), SKIN)
poly(d, [(21, 50), (23, 42), (30, 38), (42, 38), (49, 42), (51, 50)], SKIN)
rect(d, (21, 48, 51, 51), SKIN)
made.append(done(im, "icon_contacts", S))

# --- Photos
im, d = canvas(S)
rr(d, (6, 12, 58, 52), 5, STEEL_DKR)
ell(d, (16, 19, 26, 29), YELLOW)
poly(d, [(6, 46), (20, 29), (31, 40), (40, 31), (58, 49), (58, 52), (6, 52)], GREEN_LT)
made.append(done(im, "icon_photos", S))

# --- Notes
im, d = canvas(S)
rr(d, (11, 6, 53, 58), 5, PAPER)
rr(d, (11, 6, 53, 20), 5, ACCENT)
rect(d, (11, 14, 53, 20), ACCENT)
rr(d, (18, 27, 46, 30), 2, STEEL)
rr(d, (18, 36, 46, 39), 2, STEEL)
rr(d, (18, 45, 36, 48), 2, STEEL)
made.append(done(im, "icon_notes", S))

# --- Terminal
im, d = canvas(S)
rr(d, (5, 10, 59, 54), 5, TERM_BG)
rr(d, (5, 10, 59, 24), 5, TERM_BAR)
rect(d, (5, 18, 59, 24), TERM_BAR)
ell(d, (10, 14, 16, 20), (196, 72, 59, 255))
ell(d, (19, 14, 25, 20), (201, 146, 42, 255))
ell(d, (28, 14, 34, 20), GREEN_LT)
line(d, [(15, 30), (24, 37), (15, 44)], GREEN_LT, 3.4)
rr(d, (29, 41, 46, 44), 2, INK)
made.append(done(im, "icon_terminal", S))

# --- Settings
im, d = canvas(S)
teeth = 8
import math
cx = cy = 32
for i in range(teeth):
    a = (360 / teeth) * i
    d.pieslice(
        [(cx - 25) * SS, (cy - 25) * SS, (cx + 25) * SS, (cy + 25) * SS],
        a - 11, a + 11, fill=STEEL)
ell(d, (10, 10, 54, 54), STEEL)
ell(d, (21, 21, 43, 43), SHELL_DK)
ell(d, (26, 26, 38, 38), ACCENT)
made.append(done(im, "icon_settings", S))

# --- Phone / dialler
im, d = canvas(S)
poly(d, [(18, 8), (26, 10), (29, 20), (27, 25), (21, 29),
         (26, 38), (35, 45), (40, 48), (44, 43), (50, 42),
         (59, 46), (60, 53), (58, 58), (50, 60),
         (30, 54), (12, 36), (5, 18), (8, 10)], GREEN)
made.append(done(im, "icon_phone", S))

# --- Applications (the kick-off button)
im, d = canvas(S)
rr(d, (8, 8, 27, 27), 4, ACCENT)
rr(d, (37, 8, 56, 27), 4, STEEL)
rr(d, (8, 37, 27, 56), 4, STEEL)
rr(d, (37, 37, 56, 56), 4, ACCENT_HI)
made.append(done(im, "icon_apps", S))

# --- Document
im, d = canvas(S)
poly(d, [(14, 8), (38, 8), (50, 20), (50, 56), (14, 56)], PAPER)
rr(d, (14, 6, 50, 58), 4, PAPER)
poly(d, [(38, 4), (52, 18), (38, 18)], (169, 180, 189, 255))
rr(d, (21, 26, 43, 29), 2, STEEL)
rr(d, (21, 34, 43, 37), 2, STEEL)
rr(d, (21, 42, 35, 45), 2, STEEL)
made.append(done(im, "icon_doc", S))

# --- Drive / system
im, d = canvas(S)
rr(d, (6, 18, 58, 46), 5, STEEL_DK)
rr(d, (6, 30, 58, 46), 5, (65, 74, 83, 255))
ell(d, (44, 34, 52, 42), GREEN_LT)
rect(d, (12, 35, 34, 38), (98, 108, 118, 255))
made.append(done(im, "icon_drive", S))

# =========================================================== tray glyphs, 32
S = 32

# --- Network: three arcs and a dot.
im, d = canvas(S)
arc(d, (3, 6, 29, 32), 200, 340, INK, 3.0)
arc(d, (8, 12, 24, 28), 200, 340, INK, 3.0)
ell(d, (13.5, 21.5, 18.5, 26.5), INK)
made.append(done(im, "glyph_network", S))

# --- Volume
im, d = canvas(S)
poly(d, [(5, 12), (11, 12), (18, 6), (18, 26), (11, 20), (5, 20)], INK)
arc(d, (16, 8, 26, 24), 300, 60, INK, 2.6)
arc(d, (18, 4, 32, 28), 300, 60, INK, 2.6)
made.append(done(im, "glyph_volume", S))

# --- Battery
im, d = canvas(S)
d.rounded_rectangle([3 * SS, 10 * SS, 26 * SS, 22 * SS], radius=3 * SS,
                    outline=INK, width=int(2 * SS))
rr(d, (6, 13, 19, 19), 1, INK)
rr(d, (27, 13.5, 30, 18.5), 1, INK)
made.append(done(im, "glyph_battery", S))

# --- Power
im, d = canvas(S)
arc(d, (5, 5, 27, 27), 300, 240, INK, 3.0)
rr(d, (14.4, 3, 17.6, 16), 1.6, INK)
made.append(done(im, "glyph_power", S))

# --- Search
im, d = canvas(S)
d.ellipse([4 * SS, 4 * SS, 22 * SS, 22 * SS], outline=INK, width=int(2.8 * SS))
line(d, [(20, 20), (28, 28)], INK, 3.0)
made.append(done(im, "glyph_search", S))

# --- Lock
im, d = canvas(S)
d.arc([9 * SS, 4 * SS, 23 * SS, 20 * SS], 180, 360, fill=INK, width=int(2.8 * SS))
rr(d, (6, 13, 26, 27), 3, INK)
ell(d, (14, 18, 18, 22), SHELL_DK)
made.append(done(im, "glyph_lock", S))

# --- Chevron (expanders, "more" affordances)
im, d = canvas(S)
line(d, [(12, 8), (20, 16), (12, 24)], INK, 3.0)
made.append(done(im, "glyph_chevron", S))

# ------------------------------------------------- the Game Master's glyphs
# Author mode on the desktop is the phone's author mode with room to breathe:
# a toolbar above the list instead of rows wedged into it. These are the five
# verbs it needs, and they are drawn in the accent rather than in ink so that
# a control which CHANGES the mission never looks like one that reads it.
im, d = canvas(S)
rr(d, (14.4, 6, 17.6, 26), 1.6, ACCENT_HI)
rr(d, (6, 14.4, 26, 17.6), 1.6, ACCENT_HI)
made.append(done(im, "glyph_add", S))

im, d = canvas(S)
rr(d, (7, 9, 25, 27), 2.5, ACCENT_HI)
rr(d, (5, 6, 27, 9), 1.5, ACCENT_HI)
rr(d, (12, 3, 20, 6), 1.5, ACCENT_HI)
rr(d, (11.5, 13, 13.5, 23), 1, SHELL_DK)
rr(d, (18.5, 13, 20.5, 23), 1, SHELL_DK)
made.append(done(im, "glyph_delete", S))

im, d = canvas(S)
poly(d, [(5, 27), (7, 20), (21, 6), (26, 11), (12, 25)], ACCENT_HI)
poly(d, [(21, 6), (26, 11), (28, 9), (23, 4)], (255, 255, 255, 235))
poly(d, [(5, 27), (7, 20), (12, 25)], (255, 255, 255, 200))
made.append(done(im, "glyph_edit", S))

im, d = canvas(S)
line(d, [(6, 16), (13, 23), (26, 8)], ACCENT_HI, 3.4)
made.append(done(im, "glyph_check", S))

im, d = canvas(S)
d.rounded_rectangle([4 * SS, 7 * SS, 28 * SS, 25 * SS], radius=3 * SS,
                    outline=ACCENT_HI, width=int(2.2 * SS))
ell(d, (8, 11, 13, 16), ACCENT_HI)
poly(d, [(6, 23), (13, 15), (18, 20), (22, 16), (26, 23)], ACCENT_HI)
made.append(done(im, "glyph_addimage", S))

# ======================================================= window buttons, 24
S = 24

im, d = canvas(S)
rr(d, (5, 11, 19, 13), 1, INK)
made.append(done(im, "glyph_min", S))

im, d = canvas(S)
d.rounded_rectangle([5 * SS, 5 * SS, 19 * SS, 19 * SS], radius=2 * SS,
                    outline=INK, width=int(2 * SS))
made.append(done(im, "glyph_max", S))

im, d = canvas(S)
d.rounded_rectangle([8 * SS, 4 * SS, 20 * SS, 16 * SS], radius=2 * SS,
                    outline=INK, width=int(2 * SS))
rect(d, (4, 8, 16, 20), (0, 0, 0, 0))
d.rounded_rectangle([4 * SS, 8 * SS, 16 * SS, 20 * SS], radius=2 * SS,
                    outline=INK, width=int(2 * SS), fill=SHELL_HI)
made.append(done(im, "glyph_restore", S))

im, d = canvas(S)
line(d, [(6, 6), (18, 18)], INK, 2.4)
line(d, [(18, 6), (6, 18)], INK, 2.4)
made.append(done(im, "glyph_close", S))

# --- resize grip
im, d = canvas(S)
line(d, [(20, 8), (8, 20)], INK, 2.0)
line(d, [(20, 14), (14, 20)], INK, 2.0)
made.append(done(im, "glyph_grip", S))

# ============================================================== surfaces
# A window corner, drawn big so that stretching it across a 900-unit window
# does not turn a 14px radius into an oval. Tinted at runtime like every other
# white shape on this device.
im = Image.new("RGBA", (256 * 2, 256 * 2), (0, 0, 0, 0))
d = ImageDraw.Draw(im)
d.rounded_rectangle([0, 0, 512 - 1, 512 - 1], radius=28, fill=(255, 255, 255, 255))
im.resize((256, 256), Image.LANCZOS).save(os.path.join(OUT, "tile_window.png"))
made.append("tile_window")

# A one-pixel-radius square, for surfaces that want a flat fill with a texture
# slot filled in rather than an untextured widget.
im = Image.new("RGBA", (8, 8), (255, 255, 255, 255))
im.save(os.path.join(OUT, "tile_flat.png"))
made.append("tile_flat")


def vbar(name, top, bottom, hi=None):
    """A vertical gradient strip, 8 wide. Stretched horizontally it cannot
    distort, which is the whole reason bars are built this way."""
    h = 64
    im = Image.new("RGBA", (8, h), (0, 0, 0, 0))
    px = im.load()
    for y in range(h):
        t = y / (h - 1)
        c = tuple(int(top[i] + (bottom[i] - top[i]) * t) for i in range(4))
        for x in range(8):
            px[x, y] = c
    if hi:
        for x in range(8):
            px[x, 0] = hi
    im.save(os.path.join(OUT, name + ".png"))
    made.append(name)


vbar("bar_panel", (42, 47, 52, 250), (26, 30, 34, 250), (255, 255, 255, 38))
vbar("bar_titlebar", (46, 51, 57, 255), (32, 36, 40, 255), (255, 255, 255, 30))
vbar("bar_header", (40, 45, 50, 255), (34, 39, 44, 255))

# --------------------------------------------------------------- wallpaper
# The same routine the HTML preview draws with, at print size. Nothing here is
# a photograph: a desktop background that is somebody's holiday snap reads as a
# stock mock-up the moment a player looks at it.
W, H = 1600, 1000
wp = Image.new("RGBA", (W, H), (0, 0, 0, 255))
d = ImageDraw.Draw(wp)
for y in range(H):
    t = y / (H - 1)
    if t < 0.5:
        u = t / 0.5
        c = (int(29 + (22 - 29) * u), int(38 + (28 - 38) * u), int(46 + (34 - 46) * u), 255)
    else:
        u = (t - 0.5) / 0.5
        c = (int(22 + (16 - 22) * u), int(28 + (20 - 28) * u), int(34 + (26 - 34) * u), 255)
    d.line([(0, y), (W, y)], fill=c)

# A haze band where the ridges meet the sky. Without it the silhouettes sit on
# the gradient like cut paper.
haze = Image.new("RGBA", (W, H), (0, 0, 0, 0))
hd = ImageDraw.Draw(haze)
for i in range(90):
    t = i / 89
    y = (0.44 + 0.26 * t) * H
    hd.line([(0, y), (W, y)], fill=(120, 150, 175, int(13 * (1 - abs(t - 0.5) * 2))))
wp = Image.alpha_composite(wp, haze)
d = ImageDraw.Draw(wp)

for sx, sy, sw, col, edge in [(0.00, 0.62, 0.28, (31, 42, 51, 255), (58, 76, 90, 255)),
                              (0.22, 0.70, 0.24, (26, 36, 44, 255), (48, 64, 76, 255)),
                              (0.44, 0.58, 0.30, (33, 44, 54, 255), (63, 82, 97, 255)),
                              (0.70, 0.68, 0.34, (27, 37, 45, 255), (50, 66, 79, 255))]:
    pts = [(sx * W, H), (sx * W, sy * H), ((sx + sw / 2) * W, (sy - 0.14) * H),
           ((sx + sw) * W, sy * H), ((sx + sw) * W, H)]
    d.polygon(pts, fill=col)
    # One lit edge along the top of each ridge, the width of a hairline. It is
    # the only thing that tells the eye these are solid and not holes.
    d.line([pts[1], pts[2], pts[3]], fill=edge, width=2)

glow = Image.new("RGBA", (W, H), (0, 0, 0, 0))
gd = ImageDraw.Draw(glow)
gx, gy, gr = W * 0.78, H * 0.30, W * 0.42
steps = 110
for i in range(steps, 0, -1):
    r = gr * i / steps
    a = int(52 * (1 - i / steps) ** 2)
    gd.ellipse([gx - r, gy - r, gx + r, gy + r], fill=(194, 99, 20, a))
wp = Image.alpha_composite(wp, glow)

d = ImageDraw.Draw(wp)
# A one-level ordered dither instead of grain.
#
# TWO EARLIER ATTEMPTS AT THIS WERE WRONG. Scattered bright points at alpha 7
# read as a starfield; at alpha 3 they still read as a starfield, only fainter.
# What the gradient actually needs is not texture, it is enough variation to
# stop eight-bit steps banding across a 1600-pixel sky -- which is a 4x4 Bayer
# pattern of plus-one, invisible on its own and fatal to banding.
BAYER = [[0, 1, 0, 1], [1, 0, 1, 0], [0, 1, 1, 0], [1, 0, 0, 1]]
tile = Image.new("RGBA", (4, 4))
tp = tile.load()
for ty in range(4):
    for tx in range(4):
        v = BAYER[ty][tx]
        tp[tx, ty] = (v, v, v, 0)
dither = Image.new("RGBA", (W, H))
for ty in range(0, H, 4):
    for tx in range(0, W, 4):
        dither.paste(tile, (tx, ty))
from PIL import ImageChops
wp = ImageChops.add(wp, dither)
wp.save(os.path.join(OUT, "wallpaper_desktop.png"))
made.append("wallpaper_desktop")

# ---------------------------------------------------------------- the metas
META = '''MetaFileClass {
 Name "{%s}UI/images/MCF_Desktop/%s.edds"
 Configurations {
  PNGResourceClass PC : "{EAB5DE3219F9CBA8}Configs/System/ResourceTypes/PC/TextureColorMap.conf" {
  }
  PNGResourceClass XBOX_ONE : "{91D862F89991BFBE}Configs/System/ResourceTypes/XBOX_ONE/TextureColorMap.conf" {
  }
  PNGResourceClass XBOX_SERIES : "{5FEAED1642ECE679}Configs/System/ResourceTypes/XBOX_SERIES/TextureColorMap.conf" {
  }
  PNGResourceClass PS4 : "{12273E1A0928F0C4}Configs/System/ResourceTypes/PS4/TextureColorMap.conf" {
  }
  PNGResourceClass PS5 : "{531A0D167B1ABD97}Configs/System/ResourceTypes/PS5/TextureColorMap.conf" {
  }
  PNGResourceClass HEADLESS : "{BEAF5CD0C438676E}Configs/System/ResourceTypes/HEADLESS/TextureColorMap.conf" {
  }
 }
}
'''

lines = []
for i, name in enumerate(made):
    guid = "6A1C4F0B39D355%02X" % (i + 1)
    with open(os.path.join(OUT, name + ".edds.meta"), "w") as fh:
        fh.write(META % (guid, name))
    lines.append('"{%s}UI/images/MCF_Desktop/%s.edds"   %s' % (guid, name, name))

with open(os.path.join(OUT, "MANIFEST.txt"), "w") as fh:
    fh.write("MCF desktop art -- GUID reference\n")
    fh.write("=" * 64 + "\n\n")
    fh.write("\n".join(lines) + "\n")

print("files:", len(made))
print("\n".join(lines))
