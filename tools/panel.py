#!/usr/bin/env python3
"""PrLogueXXL panel: the four pages of the collage (tools/collage.py) as an MPC skin.

Each page's static parts (photo panel, wood, section titles, labels, screen frames) become its background image;
every control becomes a skin widget drawn from the same photo parts: knob filmstrips (the photo's cap with its
pointer turning), lever-switch filmstrips (one frame per position), button and LED images, the 7-segment display's
pictures, the wheels, the OLED's live text and the L.F. COMP meter (tools/vu.py).

    python3 tools/panel.py            writes art/skin/*.png and layout.conf
    python3 tools/panel.py --preview  also writes build/preview/page1..4.png (background + controls at their defaults)
"""
import json, math, os, sys
from PIL import Image, ImageDraw, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import collage as C   # noqa: E402
import vu             # noqa: E402

OUT = os.path.join(ROOT, 'art', 'skin')
REL = 'art/skin/'
PARAMS = {p['key']: p for p in json.load(open(os.path.join(ROOT, 'params.json')))['params']}
YO = 86       # the plugin area starts at y=86 of the 1280x800 layout
DIM = '2a2a2c'   # MPC's own value text under knobs: hidden in the panel colour (the OLED shows values, as on the prologue)
_made = set()


def lever_len(n): return 40 + (30 if n < 4 else 26) * (n - 1)
def lever_spot(y, n, i): return y + 15 + (lever_len(n) - 30) * i / max(1, n - 1)


def save(img, name):
    if name not in _made:
        img.save(os.path.join(OUT, name))
        _made.add(name)
    return REL + name


def default_of(key):
    p = PARAMS[key]
    if 'options' in p:
        return p.get('default', 0) / max(1, len(p['options']) - 1)
    lo, hi = p.get('min', 0), p.get('max', 1)
    return (p.get('default', lo) - lo) / (hi - lo) if hi > lo else 0


class Page:
    def __init__(self, name):
        self.name = name
        self.bg = C.panel()
        self.lines = []
        self.preview = []   # (image, x, y) at defaults

    # ---- knobs: the photo's cap, the pointer turning through 300 degrees
    def knob(self, key, x, y, d, label, big=False, pointer=True, lsz=19):
        # The frame holds the cap and its shadow; MPC's strip is 2r+10 square, 128 frames. 128 px frames put the
        # strip exactly on MPC's 16384 px image limit, where it drifts up and down the screen as it turns (device,
        # 2026-10-23: Cutoff and Program/Value; docs/NOTES.md "Knob filmstrips over 16384 px"). r <= 58 is safe.
        s = min(126, int(d * 1.25) + 2)
        d = min(d, int((s - 2) / 1.25))
        r = (s - 10) // 2
        name = 'knob_%d%s%s.png' % (d, 'b' if big else '', '' if pointer else 'e')
        if name not in _made:
            n = 64
            strip = Image.new('RGBA', (s, s * n), (0, 0, 0, 0))
            for i in range(n):
                fr = Image.new('RGBA', (s, s), (0, 0, 0, 0))
                C.place_knob(fr, s / 2, s / 2, d, i / (n - 1), big, pointer)
                strip.alpha_composite(fr, (0, i * s))
            save(strip, name)
        for i, l in enumerate(label.split('\n')):
            C.text(self.bg, x, y + d * 0.62 + 6 + i * (lsz + 2), l, lsz)
        self.lines.append('knob cx=%d cy=%d r=%d label="" key=%s strip=%s%s ns=0 vs=12 bw=%d ink_dim=%s' %
                          (x, y + YO, r, key, REL, name, s, DIM))
        fr = Image.new('RGBA', (s, s), (0, 0, 0, 0))
        C.place_knob(fr, s / 2, s / 2, d, default_of(key), big, pointer)
        self.preview.append((fr, x - s // 2, y - s // 2))

    # ---- lever switches: a vertical slider whose filmstrip has one frame per option (drag up / down, or a Q-Link);
    # pos[k] = the lever position (0 = top) of option k
    def lever(self, key, x, y, n, pos, opts=None, label=None, icons=None):
        length = lever_len(n)
        w = 36
        name = 'lever_%d_%s.png' % (n, ''.join(str(p) for p in pos))
        if name not in _made:
            strip = Image.new('RGBA', (w, length * len(pos)), (0, 0, 0, 0))
            for k, p in enumerate(pos):
                fr = Image.new('RGBA', (w, length), (0, 0, 0, 0))
                C.place_switch(fr, w / 2, 0, n, p, length)
                strip.alpha_composite(fr, (0, k * length))
            save(strip, name)
        for i in range(n):
            yy = lever_spot(y, n, i)
            if opts: C.text(self.bg, x + 24, yy - 9, opts[i], 16, anchor='lt')
            if icons: icons[i](self.bg, x + 26, yy)
        if label: C.text(self.bg, x + 8, y + length + 10, label, 19)
        cy = y + length // 2
        self.lines.append('slider_v cx=%d cy=%d w=%d h=%d label="" key=%s strip=%s%s ns=0 vs=12 bw=%d' %
                          (x, cy + YO, w, length, key, REL, name, w))
        fr = Image.new('RGBA', (w, length), (0, 0, 0, 0))
        k = int(round(default_of(key) * (len(pos) - 1)))
        C.place_switch(fr, w / 2, 0, n, pos[k], length)
        self.preview.append((fr, x - w // 2, y))

    # ---- buttons (the photo's buttons, lit and unlit)
    def _btn_imgs(self, w):
        names = []
        for on in (0, 1):
            name = 'button_%d_%s.png' % (w, 'on' if on else 'off')
            if name not in _made:
                c = C.crop(1062 if on else 1100, 572, 32, 16)
                save(C.up(c, w / 32).convert('RGBA'), name)
            names.append(name)
        return names

    def button(self, key, x, y, w, label=None, lsz=17, toggle=False):
        off, on = self._btn_imgs(w)
        bw, bh = Image.open(os.path.join(OUT, off)).size
        if label is not None:
            C.text(self.bg, x, y + 20, label, lsz)
        if toggle:
            self.lines.append('toggle cx=%d cy=%d label="" key=%s img=%s%s img_on=%s%s ns=0' % (x, y + YO, key, REL, off, REL, on))
        else:
            self.lines.append('button cx=%d cy=%d w=%d h=%d label="" key=%s img=%s%s img_on=%s%s' % (x, y + YO, bw, bh, key, REL, off, REL, on))
        self.preview.append((Image.open(os.path.join(OUT, off)).convert('RGBA'), x - bw // 2, y - bh // 2))

    # ---- LED groups: one picture per option of `key`; leds[k] = list of (x, y, size) lit for option k
    def leds(self, key, spots, lit_of):
        xs = [x for x, y, sz in spots]; ys = [y for x, y, sz in spots]; m = max(sz for x, y, sz in spots)
        x0, y0 = int(min(xs) - m), int(min(ys) - m)
        w, h = int(max(xs) - min(xs) + 2 * m), int(max(ys) - min(ys) + 2 * m)
        files = []
        nopt = len(PARAMS[key]['options'])
        for k in range(nopt):
            img = Image.new('RGBA', (w, h), (0, 0, 0, 0))
            for j, (x, y, sz) in enumerate(spots):
                C.place_led(img, x - x0, y - y0, j in lit_of(k), sz)
            files.append(save(img, 'leds_%s_%d.png' % (key, k)))
        self.lines.append('picture x=%d y=%d w=%d h=%d key=%s files="%s"' % (x0, y0 + 86, w, h, key, ','.join(files)))
        self.preview.append((Image.open(os.path.join(ROOT, files[int(round(default_of(key) * (nopt - 1)))])).convert('RGBA'), x0, y0))

    def text(self, *a, **k): C.text(self.bg, *a, **k)
    def section(self, *a): C.section(self.bg, *a)
    def vline(self, *a): C.vline(self.bg, *a)

    def write(self, idx):
        name = save(self.bg.convert('RGB'), 'bg_page%d.png' % idx)
        return name


def oled_frame(page, x, y, w, h):
    """the OLED: black glass with the oscilloscope trace low in it; the two live text lines go on top"""
    g = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(g)
    d.rounded_rectangle((0, 0, w - 1, h - 1), radius=12, fill=(20, 20, 22, 255), outline=(78, 78, 84, 255), width=3)
    d.rounded_rectangle((12, 12, w - 13, h - 13), radius=6, fill=(3, 3, 4, 255))
    pts = []
    for i in range(w - 50):
        ph = i / (w - 50) * 3.0
        tri = 4 * abs(ph - math.floor(ph + 0.5)) - 1
        pts.append((25 + i, h * 0.72 - (h * 0.14) * tri))
    lay = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    ImageDraw.Draw(lay).line(pts, fill=(200, 200, 200, 255), width=3)
    g.alpha_composite(lay.filter(ImageFilter.GaussianBlur(2.2)))
    g.alpha_composite(lay)
    page.bg.alpha_composite(g, (x, y))


SEGTXT = (["HIGH", "LO", "PEAK", "dECI"] +
          ["SIn1", "SIn2", "SIn3", "SIn4", "SAU1", "SAU2", "SqU1", "SqU2", "FAt1", "FAt2", "Air1", "Air2", "dEC1", "dEC2",
           "CrEEP", "throAt"] + ["USr %d" % i if i < 10 else "USr%d" % i for i in range(1, 17)])


def seg_display(page, key, x, y, w, h):
    files = []
    for k, s in enumerate(SEGTXT):
        img = Image.new('RGBA', (w, h), (0, 0, 0, 0))
        C.led_display(img, 0, 0, w, h, s)
        files.append(save(img, 'seg_%02d.png' % k))
    blank = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    C.led_display(blank, 0, 0, w, h, '')
    page.bg.alpha_composite(blank, (x, y))
    page.lines.append('picture x=%d y=%d w=%d h=%d key=%s files="%s"' % (x, y + 86, w, h, key, ','.join(files)))
    page.preview.append((Image.open(os.path.join(ROOT, files[0])).convert('RGBA'), x, y))


def wheels(page):
    """the pitch and mod wheels: the photo's wheels in their wells, with a light index line at the value"""
    src = C.im.crop((88, 392, 186, 528))
    src = C.up(src, 3.0).convert('RGBA')
    W_, H_ = src.size
    for key, x0, cx in (('bend', 14, 98), ('modwheel', 186, 270)):
        well = src.crop((x0, 4, x0 + 96, H_ - 8))
        ww, hh = well.size
        m = Image.new('L', well.size, 0)
        ImageDraw.Draw(m).rounded_rectangle((0, 0, ww - 1, hh - 1), radius=10, fill=255)
        well.putalpha(m.filter(ImageFilter.GaussianBlur(2)))
        n = 48
        strip = Image.new('RGBA', (ww, hh * n), (0, 0, 0, 0))
        for i in range(n):
            fr = well.copy()
            yy = int(hh * 0.88 - (hh * 0.76) * i / (n - 1))   # minimum at the bottom of the travel
            lay = Image.new('RGBA', fr.size, (0, 0, 0, 0))
            dl = ImageDraw.Draw(lay)
            dl.rectangle((14, yy - 2, ww - 15, yy + 2), fill=(235, 235, 235, 230))
            fr.alpha_composite(lay.filter(ImageFilter.GaussianBlur(0.8)))
            strip.alpha_composite(fr, (0, i * hh))
        name = save(strip, 'wheel_%s.png' % key)
        cy = 80 + 4 + hh // 2
        page.lines.append('slider_v cx=%d cy=%d w=%d h=%d label="" key=%s strip=%s ns=0 vs=12 ink_dim=%s' % (cx, cy + YO, ww, hh, key, name, DIM))
        k = n // 2 if key == 'bend' else 0
        page.preview.append((strip.crop((0, hh * k, ww, hh * (k + 1))), cx - ww // 2, cy - hh // 2))


def build():
    os.makedirs(OUT, exist_ok=True)
    pages = []
    # ------------------------------------------------------------ page 1: MAIN
    p = Page('MAIN')
    p.section(40, 380, 16, 'Mod Effect')
    p.lever('modfx_sw', 62, 74, 3, [2, 1, 0], ['Select', 'On', 'Off'])
    p.knob('modfx_speed', 220, 128, 84, 'Speed'); p.knob('modfx_depth', 326, 128, 84, 'Depth')
    p.section(40, 380, 316, 'Delay / Reverb')
    p.lever('dlrv_sw', 62, 374, 3, [2, 1, 0], ['Reverb', 'Delay', 'Off'])
    p.knob('dlrv_time', 220, 428, 84, 'Time'); p.knob('dlrv_depth', 326, 428, 84, 'Depth')
    p.vline(396, 20, 600)
    oled_frame(p, 414, 24, 400, 290)
    p.lines.append('readout cx=614 cy=164 w=360 h=56 label="" key=oled1 tsize=30 tcolor=f6f6f6 talign=left')
    p.lines.append('readout cx=614 cy=224 w=360 h=56 label="" key=oled2 tsize=30 tcolor=f6f6f6 talign=left')
    p.knob('value', 902, 150, 118, 'Program /\nValue', big=True, pointer=False, lsz=21)
    p.text(700, 350, 'Program Sort / Edit Page', 20)
    for i in range(8):
        p.button('pgm%d' % (i + 1), 448 + i * 72, 400, 64, str(i + 1), 18)
    p.text(590, 494, 'Edit', 20)
    for i, (k, n) in enumerate([('edit', 'Edit Mode'), ('write', 'Write'), ('exit', 'Exit')]):
        p.button(k, 464 + i * 84, 540, 64, n)
    p.button('shift', 464 + 3 * 84, 540, 64, 'Shift', toggle=True)
    p.text(902, 494, 'Octave', 20)
    p.button('oct_dn', 866, 540, 64, 'Down'); p.button('oct_up', 938, 540, 64, 'Up')
    p.vline(990, 20, 600)
    p.section(1006, 1250, 16, 'L.F. Comp.')
    VU = 220
    p.lines.append('meter cx=1128 cy=%d w=%d h=%d key=lfc_vu strip=art/vu_strip.png frames=55' % (196 + 86, VU, VU))
    p.preview.append((vu.frame(VU, 0.708), 1128 - VU // 2, 196 - VU // 2))
    p.lever('lfc_sw', 1030, 400, 2, [1, 0], ['On', 'Off'])
    p.knob('lfc_gain', 1172, 440, 92, 'Gain')
    p.lines.append('qlinks "MAIN" = modfx_speed,modfx_depth,dlrv_time,dlrv_depth,modfx_sw,dlrv_sw,value,-,'
                   'lfc_sw,lfc_gain,shift,-,-,-,-,-')
    pages.append(p)
    # ------------------------------------------------------------ page 2: VOICE
    p = Page('VOICE')
    wheels(p)
    p.text(98, 500, 'Pitch', 19); p.text(270, 500, 'Mod', 19)
    p.vline(330, 24, 600)
    p.knob('master', 420, 92, 72, 'Master')
    p.section(350, 540, 190, 'Timbre')
    p.knob('balance', 420, 280, 66, 'Main/Sub\nBalance')
    p.lever('timbre_type', 360, 420, 3, [2, 1, 0], ['Split', 'X-Fade', 'Layer'], 'Type')
    p.lever('edit_timbre', 466, 420, 3, [0, 1, 2], None, 'Panel')
    for i, n in enumerate(['Main', '+', 'Sub']): p.text(514, 426 + i * 30, n, 16, anchor='lt')
    p.leds('edit_timbre', [(500, 435 + i * 30, 22) for i in range(3)], lambda k: [k] if k != 1 else [0, 2])
    p.button('sub_on', 420, 580, 76, None, toggle=True); p.text(420, 600, 'Sub On / PGM Fetch', 16)
    p.vline(560, 24, 600)
    p.knob('portamento', 650, 92, 72, 'Portamento'); p.knob('voice_spread', 650, 270, 72, 'Voice Spread')
    p.knob('vm_depth', 650, 448, 72, 'Voice Mode\nDepth')
    p.vline(740, 24, 600)
    p.section(770, 1240, 30, 'Voice Mode')
    p.button('vm_select', 820, 120, 80, 'Select')
    for i, n in enumerate(['Poly', 'Mono', 'Unison', 'Chord']):
        xx = 940 + i * 92
        p.text(xx, 72, n, 18)
        C.place_icon(p.bg, xx, 158, C.VM_ICONS[i], 3.2)
    p.leds('vm_type', [(940 + i * 92, 110, 28) for i in range(4)], lambda k: [k])
    p.section(770, 1240, 250, 'Arpeggiator')
    for i, (k, n) in enumerate([('arp_btn', 'On/Latch'), ('arp_tempo_btn', 'Tempo'), ('arp_range_btn', 'Range'), ('arp_type_btn', 'Type')]):
        p.button(k, 830 + i * 112, 320, 92, n)
    p.lines.append('qlinks "VOICE" = master,balance,portamento,voice_spread,vm_depth,timbre_type,edit_timbre,sub_on,'
                   'modwheel,bend,-,-,-,-,-,-')
    pages.append(p)
    # ------------------------------------------------------------ page 3: OSC
    p = Page('OSC')
    p.section(230, 830, 14, 'VCO 1')
    p.section(230, 830, 210, 'VCO 2')
    p.section(230, 830, 410, 'Multi Engine')
    for row, (y, wk, ok, ic, lab) in enumerate([(64, 'vco1_wave', 'vco1_oct', [C.ico_saw, C.ico_tri, C.ico_sqr], 'Wave'),
                                               (262, 'vco2_wave', 'vco2_oct', [C.ico_saw, C.ico_tri, C.ico_sqr], 'Wave'),
                                               (458, 'multi_type', 'multi_oct', [C.ico_usr, C.ico_vpm, C.ico_noise], 'Engine')]):
        p.lever(wk, 70, y, 3, [2, 1, 0], None, lab, ic)
        p.lever(ok, 160, y, 4, [3, 2, 1, 0], None, 'Octave')
        for i, n in enumerate(["16'", "8'", "4'", "2'"]): p.text(226, lever_spot(y, 4, i) - 9, n, 16, anchor='lt')
        p.leds(ok, [(206, lever_spot(y, 4, i), 18) for i in range(4)], lambda k: [3 - k])
    p.knob('vco1_pitch', 310, 104, 70, 'Pitch'); p.knob('vco1_shape', 420, 104, 70, 'Shape')
    p.lever('peg_target', 530, 50, 3, [2, 1, 0], ['All', 'VCO 1+2', 'VCO 2'], 'Pitch EG')
    p.knob('peg_int', 740, 104, 70, 'Int')
    p.knob('vco2_pitch', 310, 300, 70, 'Pitch'); p.knob('vco2_shape', 420, 300, 70, 'Shape')
    p.lever('sync_ring', 530, 246, 3, [2, 1, 0], ['Ring', 'Off', 'Sync'])
    p.knob('cross_mod', 740, 300, 70, 'Cross Mod\nDepth')
    seg_display(p, 'me_disp', 250, 452, 360, 120)
    p.knob('multi_sel', 680, 500, 76, 'Type', pointer=False); p.knob('multi_shape', 790, 500, 70, 'Shape')
    p.vline(880, 20, 600)
    p.section(900, 1240, 14, 'Mixer')
    p.knob('vco1_level', 1070, 104, 72, 'VCO 1'); p.knob('vco2_level', 1070, 300, 72, 'VCO 2')
    p.knob('multi_level', 1070, 496, 72, 'Multi')
    p.lines.append('qlinks "OSC" = vco1_pitch,vco1_shape,vco2_pitch,vco2_shape,peg_int,cross_mod,multi_sel,multi_shape,'
                   'vco1_level,vco2_level,multi_level,vco1_wave,vco2_wave,multi_type,sync_ring,peg_target')
    pages.append(p)
    # ------------------------------------------------------------ page 4: FILTER
    p = Page('FILTER')
    p.section(46, 380, 14, 'VCF')
    p.knob('cutoff', 213, 118, 118, 'Cutoff', big=True)
    p.knob('resonance', 110, 330, 72, 'Resonance'); p.knob('cutoff_eg', 300, 330, 72, 'EG Int')
    p.lever('drive', 84, 450, 3, [2, 1, 0], ['100', '50', '0'], 'Drive')
    p.lever('low_cut', 200, 450, 2, [1, 0], ['On', 'Off'], 'Low Cut')
    p.lever('keytrack', 300, 450, 3, [2, 1, 0], ['100', '50', '0'], 'Keytrack')
    p.vline(400, 20, 600)
    p.section(420, 1240, 14, 'Amp EG')
    for i, (k, n) in enumerate([('amp_a', 'Attack'), ('amp_d', 'Decay'), ('amp_s', 'Sustain'), ('amp_r', 'Release')]):
        p.knob(k, 520 + i * 200, 100, 70, n)
    p.section(420, 1240, 206, 'EG')
    for i, (k, n) in enumerate([('eg_a', 'Attack'), ('eg_d', 'Decay'), ('eg_s', 'Sustain'), ('eg_r', 'Release')]):
        p.knob(k, 520 + i * 200, 292, 70, n)
    p.section(420, 1240, 398, 'LFO')
    p.lever('lfo_wave', 470, 450, 3, [2, 1, 0], None, 'Wave', [C.ico_saw, C.ico_tri, C.ico_sqr])
    p.lever('lfo_mode', 590, 450, 3, [2, 1, 0], ['Fast', 'Slow', 'BPM'], 'Mode')
    p.knob('lfo_rate', 760, 492, 72, 'Rate'); p.knob('lfo_int', 930, 492, 72, 'Int')
    p.lever('lfo_target', 1080, 450, 3, [2, 1, 0], ['Pitch', 'Shape', 'Cutoff'], 'Target')
    p.lines.append('qlinks "FILTER" = cutoff,resonance,cutoff_eg,keytrack,amp_a,amp_d,amp_s,amp_r,eg_a,eg_d,eg_s,eg_r,'
                   'lfo_rate,lfo_int,lfo_target,lfo_mode')
    pages.append(p)
    return pages


def write_layout(pages):
    out = ['# PrLogueXXL -- page layout (mpc-vst-plugins layout.conf syntax: tools/shadow_skin.py, docs/SKIN_STUDIO.md).',
           '# GENERATED by tools/panel.py from the collage pages: edit that, then run `python3 tools/panel.py`.',
           'theme_bg=1a1a1c', 'theme_panel=1a1a1c', 'theme_ink=ececec', 'theme_ink_dim=%s' % DIM, 'theme_accent=e03a26',
           'theme_lcd=030304', 'theme_box=030304', 'theme_line=030304', '']
    for i, p in enumerate(pages):
        bg = p.write(i + 1)
        out.append('[tab %s]' % p.name)
        out.append('art file=%s x=0 y=86 w=1280 h=628 fit=stretch' % bg)
        out += p.lines
        out.append('')
    open(os.path.join(ROOT, 'layout.conf'), 'w').write('\n'.join(out))


def write_preview(pages):
    d = os.path.join(ROOT, 'build', 'preview')
    os.makedirs(d, exist_ok=True)
    for i, p in enumerate(pages):
        img = p.bg.copy()
        for fr, x, y in p.preview:
            img.alpha_composite(fr.convert('RGBA'), (int(x), int(y)))
        img.convert('RGB').save(os.path.join(d, 'page%d.png' % (i + 1)))


if __name__ == '__main__':
    pages = build()
    write_layout(pages)
    if '--preview' in sys.argv:
        write_preview(pages)
    print('layout.conf: %d pages, %d images in art/skin' % (len(pages), len(_made)))
