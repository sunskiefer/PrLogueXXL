#!/usr/bin/env python3
"""PrLogueXXL page collage: parts cut from the prologue photos, composed onto 1280x628 MPC pages."""
from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageChops
import math, sys

SRC = __import__('os').path.join(__import__('os').path.dirname(__file__), '..', 'art', 'photos', 'prologue_top.jpg')
FONT = __import__('os').path.join(__import__('os').path.dirname(__file__), '..', 'art', 'fonts', 'BarlowSemiCondensed-SemiBold.ttf')
FONTB = __import__('os').path.join(__import__('os').path.dirname(__file__), '..', 'art', 'fonts', 'BarlowSemiCondensed-Bold.ttf')
W, H = 1280, 628
im = Image.open(SRC).convert('RGB')
import vu
VU_X, VU_Y, VU_D = 1128, 196, 220   # L.F. COMP meter centre and size on page 1 (layout.conf meter uses the same)


def crop(cx, cy, w, h):
    return im.crop((int(round(cx - w / 2)), int(round(cy - h / 2)), int(round(cx + w / 2)), int(round(cy + h / 2))))


def up(img, scale):
    out = img.resize((max(1, int(img.size[0] * scale)), max(1, int(img.size[1] * scale))), Image.LANCZOS)
    return out.filter(ImageFilter.UnsharpMask(radius=2, percent=90, threshold=2))


def round_mask(size, r_in, r_out):
    """alpha: 1 inside r_in, fading to 0 at r_out (centred)"""
    import numpy as np
    w, h = size
    yy, xx = np.mgrid[0:h, 0:w]
    d = np.hypot(xx + .5 - w / 2, yy + .5 - h / 2)
    a = np.clip((r_out - d) / max(1e-6, r_out - r_in), 0, 1) * 255
    return Image.fromarray(a.astype('uint8'), 'L')


# ---------------------------------------------------------------- parts
_KCACHE = {}


def knob_layers(cx, cy, r, diameter):
    """the photo's knob in two layers: the cap with its pointer patched out (the photo's own lighting and texture),
    and the pointer line itself, which rotates over it"""
    import numpy as np
    key = (cx, cy, r, diameter)
    if key in _KCACHE: return _KCACHE[key]
    s = diameter / (2 * r)
    box = r * 2.8
    c = up(crop(cx, cy, box, box), s).convert('RGBA')
    n = c.size[0]
    a = np.asarray(c, dtype=np.float32)
    yy, xx = np.mgrid[0:n, 0:n]
    rx, ry = (xx + .5 - n / 2) / (r * s), (yy + .5 - n / 2) / (r * s)   # in knob radii
    band = (np.abs(rx) < 0.17) & (ry > -0.98) & (ry < -0.02)
    lum = a[..., :3].mean(axis=2)
    ptr_a = np.clip((lum - 110) / 80, 0, 1) * band
    # the cap: the whole pointer slot (line and its dark outline) replaced by the mirrored lower half
    rot = np.asarray(c.transpose(Image.FLIP_TOP_BOTTOM), dtype=np.float32)   # the lower half, mirrored: no pointer there
    slot = ((np.abs(rx) < 0.26) & (ry > -1.02) & (ry < 0.04)).astype(np.float32)
    slot = np.asarray(Image.fromarray((slot * 255).astype('uint8')).filter(ImageFilter.GaussianBlur(max(1, r * s * 0.05))), dtype=np.float32) / 255
    fill = slot[..., None]
    cap = a * (1 - fill) + rot * fill
    disc = np.clip((1.10 - np.hypot(rx, ry)) / 0.08, 0, 1)
    cap[..., 3] = disc * 255
    cap_img = Image.fromarray(cap.astype('uint8'), 'RGBA')
    ptr = np.zeros_like(a)
    ptr[..., :3] = 245
    ptr[..., 3] = ptr_a * 255
    ptr_img = Image.fromarray(ptr.astype('uint8'), 'RGBA')
    shadow = Image.new('RGBA', c.size, (0, 0, 0, 0))
    R = r * s
    ImageDraw.Draw(shadow).ellipse((n / 2 - R * 1.12, n / 2 - R * 1.0, n / 2 + R * 1.12, n / 2 + R * 1.24), fill=(0, 0, 0, 175))
    shadow = shadow.filter(ImageFilter.GaussianBlur(R * 0.12))
    _KCACHE[key] = (shadow, cap_img, ptr_img)
    return _KCACHE[key]


SMALL = (258.1, 334.4, 16.4)   # MASTER knob cap: centre, radius (photo px)
BIG = (969.0, 332.7, 26.3)     # CUTOFF


def place_knob(page, x, y, diameter, value, big=False, pointer=True):
    cx, cy, r = BIG if big else SMALL
    shadow, cap, ptr = knob_layers(cx, cy, r, diameter)
    for layer in (shadow, cap) + ((ptr.rotate(150 - 300 * value, resample=Image.BICUBIC),) if pointer else ()):
        page.alpha_composite(layer, (int(x - layer.size[0] / 2), int(y - layer.size[1] / 2)))


def lever_part(size):
    c = crop(1098, 502, 18, 18)
    c = up(c, size / 14)
    c.putalpha(round_mask(c.size, size * 0.5, size * 0.62))
    return c


def place_switch(page, x, y, n, pos, length=None):
    """a lever switch: the slot drawn in the photo's colours, the real lever from the photo at position pos of n"""
    length = length or (40 + 30 * (n - 1))
    slot = Image.new('RGBA', (30, length), (0, 0, 0, 0))
    d = ImageDraw.Draw(slot)
    d.rounded_rectangle((0, 0, 29, length - 1), radius=14, fill=(8, 8, 9, 255), outline=(44, 44, 48, 255), width=2)
    d.rounded_rectangle((5, 4, 24, length - 5), radius=10, fill=(16, 16, 18, 255))
    page.alpha_composite(slot, (int(x - 15), int(y)))
    lv = lever_part(26)
    ly = y + 15 + (length - 30) * (pos / max(1, n - 1))
    page.alpha_composite(lv, (int(x - lv.size[0] / 2), int(ly - lv.size[1] / 2)))


def place_button(page, x, y, on, w=76):
    c = crop(1062 if on else 1100, 572, 32, 16)
    c = up(c, w / 32).convert('RGBA')
    page.alpha_composite(c, (int(x - c.size[0] / 2), int(y - c.size[1] / 2)))


def place_led(page, x, y, on, size=26):
    c = crop(514, 333.5, 14, 14) if on else crop(512.5, 322.5, 12, 12)
    c = up(c, size / c.size[0]).convert('RGBA')
    c.putalpha(round_mask(c.size, size * 0.32, size * 0.5))
    page.alpha_composite(c, (int(x - c.size[0] / 2), int(y - c.size[1] / 2)))


def place_vu(page, x, y, d):
    c = crop(1597, 330, 50, 50)
    c = up(c, d / 46).convert('RGBA')
    c.putalpha(round_mask(c.size, d * 0.5, d * 0.56))
    page.alpha_composite(c, (int(x - c.size[0] / 2), int(y - c.size[1] / 2)))


VM_ICONS = [(424, 578, 447, 598), (465, 578, 478, 598), (498, 578, 520, 598), (540, 578, 549, 598)]   # POLY MONO UNISON CHORD


def place_icon(page, x, y, box, scale):
    """a printed panel icon from the photo: its white lines kept, the panel around them made transparent"""
    import numpy as np
    c = up(im.crop(box), scale).convert('RGBA')
    a = np.asarray(c, dtype=np.float32)
    lum = a[..., :3].mean(axis=2)
    a[..., 3] = np.clip((lum - 90) / 80, 0, 1) * 255
    a[..., :3] = 240
    c = Image.fromarray(a.astype('uint8'), 'RGBA')
    page.alpha_composite(c, (int(x - c.size[0] / 2), int(y - c.size[1] / 2)))


# ---------------------------------------------------------------- panel, wood, keys
def panel():
    tile = im.crop((1660, 240, 1940, 470))
    tile = tile.resize((tile.size[0] * 2, tile.size[1] * 2), Image.LANCZOS)
    tw, th = tile.size
    bg = Image.new('RGB', (W, H))
    flips = [tile, tile.transpose(Image.FLIP_LEFT_RIGHT)]
    for yy in range(0, H, th):
        for xi, xx in enumerate(range(0, W, tw)):
            t = flips[xi % 2]
            if (yy // th) % 2: t = t.transpose(Image.FLIP_TOP_BOTTOM)
            bg.paste(t, (xx, yy))
    bg = bg.filter(ImageFilter.GaussianBlur(0.6))
    bg = Image.blend(bg, Image.new('RGB', (W, H), (26, 26, 28)), 0.25)   # even out the photo's light falloff
    wood = im.crop((10, 215, 44, 985)).resize((26, H), Image.LANCZOS)
    page = bg.convert('RGBA')
    page.paste(wood, (0, 0))
    page.paste(wood.transpose(Image.FLIP_LEFT_RIGHT), (W - 26, 0))
    return page


def keyboard(page, top):
    k = im.crop((55, 652, 1213, 900))   # 22 white keys, C to A
    k = k.resize((W - 52, int(k.size[1] * (W - 52) / k.size[0])), Image.LANCZOS)
    k = k.crop((0, 0, k.size[0], H - top))
    page.paste(k, (26, top))


# ---------------------------------------------------------------- text
def font(sz, bold=False): return ImageFont.truetype(FONTB if bold else FONT, sz)


def text(page, x, y, s, sz=19, anchor='mt', color=(236, 236, 236), track=1.5, bold=False):
    """centred (anchor mt) / left (lt) text with letter spacing, as the panel prints it"""
    f = font(sz, bold)
    d = ImageDraw.Draw(page)
    s = s.upper()
    widths = [d.textlength(ch, font=f) for ch in s]
    total = sum(widths) + track * (len(s) - 1)
    xx = x - total / 2 if anchor == 'mt' else x
    for ch, w in zip(s, widths):
        d.text((xx, y), ch, font=f, fill=color)
        xx += w + track


def section(page, x0, x1, y, title):
    d = ImageDraw.Draw(page)
    text(page, (x0 + x1) / 2, y, title, 21, bold=True)
    f = font(21, True)
    tw = d.textlength(title.upper(), font=f) + 2 * len(title) + 18
    mid = (x0 + x1) / 2
    yy = y + 12
    if mid - tw / 2 - 10 > x0: d.line((x0, yy, mid - tw / 2 - 6, yy), fill=(200, 200, 200), width=2)
    if mid + tw / 2 + 10 < x1: d.line((mid + tw / 2 + 6, yy, x1, yy), fill=(200, 200, 200), width=2)


def vline(page, x, y0, y1):
    ImageDraw.Draw(page).line((x, y0, x, y1), fill=(190, 190, 190), width=2)


def knob(page, x, y, d, v, label, big=False, pointer=True, lsz=19):
    place_knob(page, x, y, d, v, big, pointer)
    lines = label.split('\n')
    for i, l in enumerate(lines): text(page, x, y + d * 0.62 + 6 + i * (lsz + 2), l, lsz)


def switch(page, x, y, n, pos, opts=None, label=None, icons=None):
    length = 40 + 30 * (n - 1)
    place_switch(page, x, y, n, pos, length)
    for i in range(n):
        yy = y + 15 + (length - 30) * i / max(1, n - 1)
        if opts: text(page, x + 24, yy - 9, opts[i], 16, anchor='lt')
        if icons: icons[i](page, x + 26, yy)
    if label: text(page, x + 8, y + length + 10, label, 19)


def ico_saw(page, x, y):
    ImageDraw.Draw(page).line([(x, y + 7), (x + 11, y - 7), (x + 11, y + 7), (x + 22, y - 7)], fill=(236, 236, 236), width=2)
def ico_tri(page, x, y):
    ImageDraw.Draw(page).line([(x, y + 7), (x + 7, y - 7), (x + 14, y + 7), (x + 21, y - 7)], fill=(236, 236, 236), width=2)
def ico_sqr(page, x, y):
    ImageDraw.Draw(page).line([(x, y + 7), (x, y - 6), (x + 11, y - 6), (x + 11, y + 7), (x + 22, y + 7), (x + 22, y - 6)], fill=(236, 236, 236), width=2)
def ico_usr(page, x, y): text(page, x, y - 9, 'USR', 16, anchor='lt')
def ico_vpm(page, x, y):
    pts = [(x + i, y - 6 * math.sin(i / 22 * 2 * math.pi * 1.5) * (1 - i / 30)) for i in range(0, 24)]
    ImageDraw.Draw(page).line(pts, fill=(236, 236, 236), width=2)
def ico_noise(page, x, y):
    import random
    rnd = random.Random(3)
    ImageDraw.Draw(page).line([(x + i * 2, y + rnd.uniform(-7, 7)) for i in range(12)], fill=(236, 236, 236), width=2)


# ---------------------------------------------------------------- the two screens
SEG = {  # 7-segment patterns, segments a b c d e f g
    'a': 'abcefg', 'b': 'cdefg', 'c': 'deg', 'd': 'bcdeg', 'e': 'adefg', 'f': 'aefg', 'g': 'abcdfg', 'h': 'cefg',
    'i': 'c', 'j': 'bcd', 'l': 'def', 'n': 'ceg', 'o': 'cdeg', 'p': 'abefg', 'q': 'abcfg', 'r': 'eg', 's': 'acdfg',
    't': 'defg', 'u': 'cde', 'y': 'bcdfg', 'A': 'abcefg', 'E': 'adefg', 'H': 'bcefg', 'C': 'adef', 'U': 'bcdef',
    '0': 'abcdef', '1': 'bc', '2': 'abdeg', '3': 'abcdg', '4': 'bcfg', '5': 'acdfg', '6': 'acdefg', '7': 'abc',
    '8': 'abcdefg', '9': 'abcdfg', '-': 'g', ' ': ''}


def led_display(page, x, y, w, h, s):
    glass = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(glass)
    d.rounded_rectangle((0, 0, w - 1, h - 1), radius=10, fill=(18, 18, 20, 255), outline=(70, 70, 76, 255), width=3)
    d.rounded_rectangle((12, 10, w - 13, h - 11), radius=6, fill=(14, 2, 2, 255))
    page.alpha_composite(glass, (x, y))
    lit = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    dl = ImageDraw.Draw(lit)
    n = 6
    cw = (w - 60) / n
    ch = h - 44
    t = max(5, cw * 0.14)
    for i, c in enumerate(s[:n].ljust(n)):
        segs = SEG.get(c, SEG.get(c.lower(), ''))
        x0 = 30 + i * cw + cw * 0.12 + 6
        sk = 0.12 * ch   # italic slant
        def P(px, py): return (x0 + px + sk * (1 - py / ch), 22 + py)
        sw = cw * 0.64
        geo = {'a': (P(t, 0), P(sw - t, 0)), 'g': (P(t, ch / 2), P(sw - t, ch / 2)), 'd': (P(t, ch), P(sw - t, ch)),
               'f': (P(0, t), P(0, ch / 2 - t)), 'b': (P(sw, t), P(sw, ch / 2 - t)),
               'e': (P(0, ch / 2 + t), P(0, ch - t)), 'c': (P(sw, ch / 2 + t), P(sw, ch - t))}
        for sg in 'abcdefg':
            if sg in segs: dl.line(geo[sg], fill=(255, 40, 26, 255), width=int(t))
    glow = lit.filter(ImageFilter.GaussianBlur(5))
    page.alpha_composite(glow, (x, y))
    page.alpha_composite(lit, (x, y))


def oled(page, x, y, w, h, title, value):
    g = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(g)
    d.rounded_rectangle((0, 0, w - 1, h - 1), radius=12, fill=(20, 20, 22, 255), outline=(78, 78, 84, 255), width=3)
    d.rounded_rectangle((12, 12, w - 13, h - 13), radius=6, fill=(3, 3, 4, 255))
    pts = []
    for i in range(w - 50):
        ph = i / (w - 50) * 3.0
        tri = 4 * abs(ph - math.floor(ph + 0.5)) - 1          # -1 .. 1
        pts.append((25 + i, h / 2 + 14 - (h * 0.28) * tri))
    lay = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    ImageDraw.Draw(lay).line(pts, fill=(246, 246, 246, 255), width=3)
    g.alpha_composite(lay.filter(ImageFilter.GaussianBlur(2.2)))
    g.alpha_composite(lay)
    page.alpha_composite(g, (x, y))
    text(page, x + 28, y + 22, title, 24, anchor='lt', color=(246, 246, 246))
    f = font(42, True)
    ImageDraw.Draw(page).text((x + w - 28, y + h - 24), value, font=f, fill=(246, 246, 246), anchor='rb')


# ---------------------------------------------------------------- pages
def page1():
    """perform page: effects, the OLED, PROGRAM/VALUE, program/edit buttons and the L.F. COMP with its live VU"""
    p = panel()
    section(p, 40, 380, 16, 'Mod Effect')
    switch(p, 62, 74, 3, 0, ['Select', 'On', 'Off'])
    knob(p, 220, 128, 84, .45, 'Speed'); knob(p, 326, 128, 84, .6, 'Depth')
    section(p, 40, 380, 316, 'Delay / Reverb')
    switch(p, 62, 374, 3, 1, ['Reverb', 'Delay', 'Off'])
    knob(p, 220, 428, 84, .35, 'Time'); knob(p, 326, 428, 84, .5, 'Depth')
    vline(p, 396, 20, 600)
    oled(p, 414, 24, 400, 290, 'Cutoff', '614')
    knob(p, 902, 150, 130, .5, 'Program /\nValue', big=True, pointer=False, lsz=21)
    text(p, 700, 350, 'Program Sort / Edit Page', 20)
    for i in range(8):
        place_button(p, 448 + i * 72, 400, i == 0, 64); text(p, 448 + i * 72, 420, str(i + 1), 18)
    text(p, 590, 494, 'Edit', 20)
    for i, n in enumerate(['Edit Mode', 'Write', 'Exit', 'Shift']):
        place_button(p, 464 + i * 84, 540, n == 'Shift', 64); text(p, 464 + i * 84, 560, n, 17)
    text(p, 902, 494, 'Octave', 20)
    place_button(p, 866, 540, False, 64); place_button(p, 938, 540, True, 64)
    text(p, 866, 560, 'Down', 17); text(p, 938, 560, 'Up', 17)
    vline(p, 990, 20, 600)
    section(p, 1006, 1250, 16, 'L.F. Comp.')
    f = vu.frame(VU_D, 0.6)                     # the meter at rest; on the device the live filmstrip sits here
    p.alpha_composite(f, (VU_X - VU_D // 2, VU_Y - VU_D // 2))
    switch(p, 1030, 400, 2, 0, ['On', 'Off'])
    knob(p, 1172, 440, 92, .4, 'Gain')
    return p


def page2():
    p = panel()
    wheel = im.crop((88, 392, 186, 528))   # the pitch and mod wheels, in their wells
    wheel = up(wheel, 3.0).convert('RGBA')
    m = Image.new('L', wheel.size, 0)
    dm = ImageDraw.Draw(m)
    for x0 in (14, 186):   # the two wells only, so the photo's lighter panel around them doesn't show
        dm.rounded_rectangle((x0, 4, x0 + 96, wheel.size[1] - 8), radius=10, fill=255)
    wheel.putalpha(m.filter(ImageFilter.GaussianBlur(2)))
    p.alpha_composite(wheel, (36, 80))
    text(p, 112, 500, 'Pitch', 19); text(p, 226, 500, 'Mod', 19)
    vline(p, 330, 24, 600)
    knob(p, 420, 92, 72, .7, 'Master')
    section(p, 350, 500, 190, 'Timbre')
    knob(p, 420, 280, 66, .5, 'Main/Sub\nBalance')
    switch(p, 360, 420, 3, 2, ['Split', 'X-Fade', 'Layer'], 'Type')
    switch(p, 470, 420, 3, 0, None, 'Panel')
    for i, n in enumerate(['Main', '+', 'Sub']):
        place_led(p, 504, 435 + i * 30, i == 0, 22); text(p, 518, 426 + i * 30, n, 16, anchor='lt')
    place_button(p, 420, 568, True, 76); text(p, 420, 590, 'Sub On / PGM Fetch', 15)
    vline(p, 560, 24, 600)
    knob(p, 650, 92, 72, .1, 'Portamento'); knob(p, 650, 270, 72, .4, 'Voice Spread'); knob(p, 650, 448, 72, .3, 'Voice Mode\nDepth')
    vline(p, 740, 24, 600)
    section(p, 770, 1240, 30, 'Voice Mode')
    place_button(p, 820, 120, False, 80); text(p, 820, 142, 'Select', 17)
    for i, n in enumerate(['Poly', 'Mono', 'Unison', 'Chord']):
        xx = 940 + i * 92
        text(p, xx, 72, n, 18); place_led(p, xx, 110, i == 0, 28)
        place_icon(p, xx, 158, VM_ICONS[i], 3.2)
    section(p, 770, 1240, 250, 'Arpeggiator')
    for i, n in enumerate(['On/Latch', 'Tempo', 'Range', 'Type']):
        place_button(p, 830 + i * 112, 320, i == 0, 92); text(p, 830 + i * 112, 342, n, 17)
    return p


def page3():
    p = panel()
    section(p, 230, 830, 14, 'VCO 1')
    section(p, 230, 830, 210, 'VCO 2')
    section(p, 230, 830, 410, 'Multi Engine')
    rows = [(64, [ico_saw, ico_tri, ico_sqr], 0, 1), (262, [ico_saw, ico_tri, ico_sqr], 2, 1), (458, [ico_usr, ico_vpm, ico_noise], 1, 2)]
    for y, ic, wpos, opos in rows:
        switch(p, 70, y, 3, wpos, None, 'Wave' if y < 400 else 'Engine', ic)
        switch(p, 168, y, 4, opos, None, 'Octave')
        for i in range(4): place_led(p, 210, y + 15 + i * 30, i == opos + (0 if y < 400 else 0), 20)
    knob(p, 300, 104, 70, .5, 'Pitch'); knob(p, 420, 104, 70, .3, 'Shape')
    switch(p, 530, 50, 3, 1, ['All', 'VCO 1+2', 'VCO 2'], 'Pitch EG')
    knob(p, 740, 104, 70, .5, 'Int')
    knob(p, 300, 300, 70, .55, 'Pitch'); knob(p, 420, 300, 70, .6, 'Shape')
    switch(p, 530, 258, 2, 0, ['Sync', 'Ring'])
    knob(p, 740, 300, 70, .2, 'Cross Mod\nDepth')
    led_display(p, 250, 452, 360, 120, 'throAt')
    knob(p, 680, 500, 76, 0, 'Type', pointer=False); knob(p, 790, 500, 70, .45, 'Shape')
    vline(p, 880, 20, 600)
    section(p, 900, 1240, 14, 'Mixer')
    knob(p, 1070, 104, 72, .8, 'VCO 1'); knob(p, 1070, 300, 72, .6, 'VCO 2'); knob(p, 1070, 496, 72, .4, 'Multi')
    return p


def page4():
    p = panel()
    section(p, 46, 380, 14, 'VCF')
    knob(p, 213, 118, 124, .6, 'Cutoff', big=True)
    knob(p, 110, 330, 72, .3, 'Resonance'); knob(p, 300, 330, 72, .5, 'EG Int')
    switch(p, 92, 450, 3, 2, None, 'Drive'); switch(p, 200, 450, 3, 0, None, 'Low Cut'); switch(p, 308, 450, 3, 1, None, 'Keytrack')
    vline(p, 400, 20, 600)
    section(p, 420, 1240, 14, 'Amp EG')
    for i, (n, v) in enumerate([('Attack', .05), ('Decay', .4), ('Sustain', .8), ('Release', .35)]): knob(p, 520 + i * 200, 100, 70, v, n)
    section(p, 420, 1240, 206, 'EG')
    for i, (n, v) in enumerate([('Attack', .1), ('Decay', .5), ('Sustain', .2), ('Release', .3)]): knob(p, 520 + i * 200, 292, 70, v, n)
    section(p, 420, 1240, 398, 'LFO')
    switch(p, 470, 450, 3, 1, None, 'Wave', [ico_saw, ico_tri, ico_sqr])
    switch(p, 590, 450, 3, 0, ['Fast', 'Slow', 'BPM'], 'Mode')
    knob(p, 760, 492, 72, .45, 'Rate'); knob(p, 930, 492, 72, .25, 'Int')
    switch(p, 1080, 450, 3, 2, ['Pitch', 'Shape', 'Cutoff'], 'Target')
    return p


if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else '.'
    for n, f in [('page1', page1), ('page2', page2), ('page3', page3), ('page4', page4)]:
        f().convert('RGB').save('%s/%s.png' % (out, n))
        print(n)
