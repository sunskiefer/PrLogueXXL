"""PrLogueXXL: the L.F. COMP VU meter, a live filmstrip (layout.conf `meter ... strip=`).

Collage from the prologue photo: the meter's own glass, bezel and needle cover are cut from the photo and blended
over a redrawn (sharp, larger) scale, so it reads at plugin size. One frame per needle position; the engine sets
the meter parameter (0..1 = needle position, a VU's linear-amplitude scale: +3 VU at 1, 0 VU at 10^(-3/20)).
"""
import math, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

SRC = __file__.rsplit('/', 2)[0] + '/art/photos/prologue_top.jpg'
PHOTO_C = (1601.0, 328.5)   # the VU's centre in the photo
PHOTO_R = 22.5              # its bezel radius in the photo
SS = 4                      # supersampling
SPAN = 42.0                 # needle swing, degrees each side of vertical
PIVOT = 0.40                # pivot below the centre, in radii
ARC = 1.12                  # scale radius from the pivot, in radii


def pos_of_db(db): return 10 ** ((db - 3) / 20.0)          # needle position of a VU reading


def ang(v): return math.radians(-SPAN + 2 * SPAN * max(0.0, min(1.0, v)))


def photo_layer(D):
    """the photo's meter, red needle taken out, scaled to D: supplies glass, bezel and cover texture"""
    im = Image.open(SRC).convert('RGB')
    cx, cy = PHOTO_C
    r = PHOTO_R * 1.12
    c = im.crop((int(cx - r), int(cy - r), int(cx + r), int(cy + r)))
    a = np.asarray(c, dtype=np.float32)
    red = (a[..., 0] - a[..., 1:].mean(axis=2)) > 28
    g = a[..., 1:].mean(axis=2)
    a[red] = np.stack([g[red]] * 3, axis=1) * 1.0 + 30
    c = Image.fromarray(np.clip(a, 0, 255).astype('uint8'), 'RGB')
    return c.resize((D, D), Image.BICUBIC).filter(ImageFilter.GaussianBlur(D / 260))


def frame(D, v, photo=None):
    S = D * SS
    R = S / 2 / 1.12                       # bezel radius inside the frame
    C = S / 2
    px, py = C, C + PIVOT * R
    img = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    # face: silver-blue like the photo, lighter at the top
    face = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    fa = np.zeros((S, S, 4), np.float32)
    yy, xx = np.mgrid[0:S, 0:S].astype(np.float32)
    t = np.clip((yy - (C - R)) / (2 * R), 0, 1)
    fa[..., 0] = 222 - 60 * t; fa[..., 1] = 226 - 58 * t; fa[..., 2] = 236 - 52 * t; fa[..., 3] = 255
    face = Image.fromarray(fa.astype('uint8'), 'RGBA')
    m = Image.new('L', (S, S), 0)
    ImageDraw.Draw(m).ellipse((C - R * 0.93, C - R * 0.93, C + R * 0.93, C + R * 0.93), fill=255)
    img.paste(face, (0, 0), m)
    d = ImageDraw.Draw(img)
    # scale: arc, ticks at VU readings, red zone from 0 VU up
    ar = ARC * R
    def pt(a, rr): return (px + rr * math.sin(a), py - rr * math.cos(a))
    box = (px - ar, py - ar, px + ar, py + ar)
    a0, a1, az = ang(0), ang(1), ang(pos_of_db(0))
    d.arc(box, math.degrees(a0) - 90, math.degrees(az) - 90, fill=(40, 40, 44), width=int(S * 0.008))
    d.arc((px - ar + S * 0.022, py - ar + S * 0.022, px + ar - S * 0.022, py + ar - S * 0.022),
          math.degrees(az) - 90, math.degrees(a1) - 90, fill=(205, 38, 34), width=int(S * 0.03))
    for db in (-20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3):
        a = ang(pos_of_db(db))
        big = db in (-20, -10, -5, 0, 3)
        col = (205, 38, 34) if db > 0 else (40, 40, 44)
        d.line([pt(a, ar - S * (0.055 if big else 0.03)), pt(a, ar)], fill=col, width=int(S * (0.011 if big else 0.007)))
    # needle (red, from the pivot past the scale)
    a = ang(v)
    nd = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(nd).line([pt(a, 0), pt(a, ar + S * 0.01)], fill=(214, 30, 26, 255), width=int(S * 0.010))
    img.alpha_composite(nd.filter(ImageFilter.GaussianBlur(S * 0.004)), (int(S * 0.006), int(S * 0.008)))   # shadow-ish
    img.alpha_composite(nd)
    # the needle cover (the hump) in the photo's grey
    hy = lambda x: (-0.30 if abs(x) < 0.26 else (-0.30 + (abs(x) - 0.26) / 0.36 * 0.24 if abs(x) < 0.62 else -0.06))
    poly = [(C + x * R, C + hy(x) * R) for x in np.linspace(-1, 1, 81)] + [(C + R, C + R), (C - R, C + R)]
    cover = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(cover).polygon(poly, fill=(116, 118, 116, 255))
    cm = Image.new('L', (S, S), 0)
    ImageDraw.Draw(cm).ellipse((C - R * 0.93, C - R * 0.93, C + R * 0.93, C + R * 0.93), fill=255)
    sh = Image.new('RGBA', (S, S), (0, 0, 0, 0))
    ImageDraw.Draw(sh).polygon([(x, y - S * 0.012) for x, y in poly[:81]] + [(C + R, C + R), (C - R, C + R)], fill=(0, 0, 0, 90))
    img.alpha_composite(Image.composite(sh.filter(ImageFilter.GaussianBlur(S * 0.01)), Image.new('RGBA', (S, S), (0, 0, 0, 0)), cm))
    img.alpha_composite(Image.composite(cover, Image.new('RGBA', (S, S), (0, 0, 0, 0)), cm))
    img = img.resize((D, D), Image.LANCZOS)
    # blend the photo meter over it: its glass, bezel ring and cover texture
    ph = (photo or photo_layer(D)).convert('RGBA')
    pa = np.asarray(ph, np.float32); ia = np.asarray(img, np.float32)
    yy, xx = np.mgrid[0:D, 0:D].astype(np.float32)
    rr = np.hypot(xx - D / 2 + .5, yy - D / 2 + .5) / (D / 2 / 1.12)
    inner = np.clip((0.93 - rr) / 0.03, 0, 1)          # 1 on the face
    ring = np.clip((1.06 - rr) / 0.04, 0, 1) * (1 - inner)
    out = ia.copy()
    # face: drawn scale, softened with 22 % photo (keeps its tone and reflections)
    out[..., :3] = ia[..., :3] * (1 - 0.22 * inner[..., None]) + pa[..., :3] * 0.22 * inner[..., None]
    out[..., :3] = out[..., :3] * (1 - ring[..., None]) + pa[..., :3] * ring[..., None] * 1.08
    out[..., 3] = np.maximum(ia[..., 3] * inner, ring * 255)
    # glass: a soft highlight top-left, as on the photo
    gl = np.clip(1 - np.hypot((xx - D * 0.38) / (D * 0.30), (yy - D * 0.30) / (D * 0.16)), 0, 1) ** 2 * 60 * inner
    out[..., :3] += gl[..., None]
    o = Image.fromarray(np.clip(out, 0, 255).astype('uint8'), 'RGBA')
    sh = Image.new('RGBA', (D, D), (0, 0, 0, 0))
    ImageDraw.Draw(sh).ellipse((D * .06, D * .09, D * .96, D * .99), fill=(0, 0, 0, 120))
    sh = sh.filter(ImageFilter.GaussianBlur(D * 0.03))
    sh.alpha_composite(o)
    return sh


def strip(D, frames):
    ph = photo_layer(D)
    s = Image.new('RGBA', (D, D * frames), (0, 0, 0, 0))
    for i in range(frames):
        s.alpha_composite(frame(D, i / (frames - 1), ph), (0, i * D))
    return s


if __name__ == '__main__':
    D = int(sys.argv[2]) if len(sys.argv) > 2 else 190
    N = int(sys.argv[3]) if len(sys.argv) > 3 else 61
    strip(D, N).save(sys.argv[1])
