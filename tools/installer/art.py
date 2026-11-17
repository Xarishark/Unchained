"""The installer's art and Unchained's icon, drawn here in the Configuration app's look: dark teal with a faint grid and
scanlines, green terminal frames, and the name as a big slanted word (white, a red edge, a black outline). No art from
the game. Windows' own fonts (Impact, Consolas).

  py -3 tools/installer/art.py <out dir> <version>

writes installer-side.bmp (328x628, the welcome and finish pages), installer-header.bmp (300x114, the other pages'
header) and unchained.ico (16 to 256 px).
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

BACK = (5, 22, 24)
BACK2 = (9, 38, 40)
GREEN = (57, 242, 138)
GREEN_DIM = (31, 143, 85)
RED = (224, 52, 58)
WHITE = (235, 248, 240)
FONTS = os.path.join(os.environ.get('WINDIR', r'C:\Windows'), 'Fonts')


def font(name, size):
    return ImageFont.truetype(os.path.join(FONTS, name), size)


def room(w, h):
    """teal, darker at the bottom, a faint grid and the terminal's scanlines"""
    im = Image.new('RGB', (w, h))
    px = im.load()
    for y in range(h):
        t = y / max(1, h - 1)
        c = tuple(int(BACK2[i] * (1 - t) + BACK[i] * t) for i in range(3))
        for x in range(w):
            px[x, y] = c
    d = ImageDraw.Draw(im, 'RGBA')
    step = 20
    for x in range(0, w, step):
        d.line([(x, 0), (x, h)], fill=GREEN + (14,))
    for y in range(0, h, step):
        d.line([(0, y), (w, y)], fill=GREEN + (14,))
    for y in range(0, h, 3):
        d.line([(0, y), (w, y)], fill=(0, 0, 0, 40))
    return im


def word(text, size, slant=0.22):
    """the big slanted word on a clear layer: black outline, red edge down-right, white face, sheared"""
    f = font('impact.ttf', size)
    box = f.getbbox(text)
    w, h = box[2] - box[0], box[3] - box[1]
    pad = size // 4
    layer = Image.new('RGBA', (w + pad * 2 + int(h * slant), h + pad * 2), (0, 0, 0, 0))
    d = ImageDraw.Draw(layer)
    o = max(2, size // 22)
    at = (pad - box[0], pad - box[1])
    for dx in (-o, 0, o):
        for dy in (-o, 0, o):
            if dx or dy:
                d.text((at[0] + dx, at[1] + dy), text, font=f, fill=(0, 0, 0, 255))
    d.text((at[0] + o + 1, at[1] + o + 1), text, font=f, fill=RED + (255,))
    d.text(at, text, font=f, fill=WHITE + (255,))
    # shear right at the top, as the Configuration app's words lean
    lw, lh = layer.size
    return layer.transform((lw, lh), Image.AFFINE, (1, slant, -slant * lh, 0, 1, 0), Image.BICUBIC)


def frame(d, a, b, tab=None, tab_font=None):
    """a green terminal frame with a cut corner and a tab"""
    x0, y0 = a
    x1, y1 = b
    cut = 10
    pts = [(x0, y0), (x1 - cut, y0), (x1, y0 + cut), (x1, y1), (x0, y1), (x0, y0)]
    d.line(pts, fill=GREEN, width=2)
    if tab:
        tw = tab_font.getbbox(tab)[2] + 16
        d.polygon([(x0, y0 - 18), (x0 + tw, y0 - 18), (x0 + tw + 10, y0), (x0, y0)], fill=GREEN)
        d.text((x0 + 8, y0 - 17), tab, font=tab_font, fill=BACK)


def fit(layer, width):
    if layer.width <= width:
        return layer
    return layer.resize((width, int(layer.height * width / layer.width)), Image.LANCZOS)


def side(version):
    w, h = 328, 628
    im = room(w, h)
    big = fit(word('UNCHAINED', 96), w - 16)
    im.paste(big, ((w - big.width) // 2, 40), big)
    d = ImageDraw.Draw(im, 'RGBA')
    mono = font('consola.ttf', 15)
    d.text((22, 40 + big.height + 6), 'WHIPLASH  //  PC PORT', font=mono, fill=GREEN)
    # the three words, faint, down the page
    y = 250
    for text in ('FREEDOM', 'CRAZY', 'MADNESS'):
        lay = fit(word(text, 70), w - 40)
        faint = lay.copy()
        faint.putalpha(faint.getchannel('A').point(lambda v: v * 30 // 100))
        im.paste(faint, ((w - lay.width) // 2, y), faint)
        y += lay.height - 6
    frame(d, (20, h - 120), (w - 20, h - 30), 'SETUP', font('consola.ttf', 13))
    d.text((34, h - 104), 'RELEASE TERMINAL', font=mono, fill=GREEN_DIM)
    d.text((34, h - 82), 'BUILD ' + version, font=mono, fill=GREEN)
    d.text((34, h - 60), 'no game files inside', font=font('consola.ttf', 13), fill=GREEN_DIM)
    return im


def header(version):
    w, h = 300, 114
    im = room(w, h)
    big = fit(word('UNCHAINED', 64), w - 30)
    im.paste(big, (w - big.width - 6, 8), big)
    d = ImageDraw.Draw(im, 'RGBA')
    d.text((w - 196, h - 26), 'BUILD ' + version, font=font('consola.ttf', 13), fill=GREEN)
    d.line([(10, h - 8), (w - 10, h - 8)], fill=GREEN_DIM, width=1)
    return im


def icon():
    """a dark teal tile with a green frame and a slanted U"""
    n = 256
    im = Image.new('RGBA', (n, n), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle([4, 4, n - 5, n - 5], radius=44, fill=BACK2 + (255,), outline=GREEN + (255,), width=10)
    u = word('U', 200, 0.18)
    u = u.resize((int(u.width * 190 / u.height), 190), Image.LANCZOS) if u.height > 190 else u
    im.alpha_composite(u, ((n - u.width) // 2, (n - u.height) // 2 + 4))
    return im


def main():
    out, version = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    side(version).save(os.path.join(out, 'installer-side.bmp'))
    header(version).save(os.path.join(out, 'installer-header.bmp'))
    icon().save(os.path.join(out, 'unchained.ico'), sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128),
                                                           (256, 256)])
    print('art in', out)


if __name__ == '__main__':
    main()
