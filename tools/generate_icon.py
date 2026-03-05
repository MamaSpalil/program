#!/usr/bin/env python3
"""
Generate VT (Virtual Trade System) application icon.

Produces resources/app_icon.ico with 5 sizes: 16, 32, 48, 64, 256 px.

Design:
  - Square with rounded corners (radius=4px scaled)
  - Dark-blue gradient background #0A0E1A → #1A2440
  - "VT" white bold text centered
  - Three mini candles (green/red/green) below text
  - Border #2D6BE4 1px

Requirements: pip install Pillow
"""

import os
import sys

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("Pillow is required: pip install Pillow", file=sys.stderr)
    sys.exit(1)


def lerp_color(c1, c2, t):
    """Linear interpolation between two RGB tuples."""
    return tuple(int(a + (b - a) * t) for a, b in zip(c1, c2))


def draw_icon(size):
    """Render a single icon at the given pixel size."""
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Colours
    bg_top = (0x0A, 0x0E, 0x1A)
    bg_bot = (0x1A, 0x24, 0x40)
    border_color = (0x2D, 0x6B, 0xE4)
    white = (255, 255, 255)
    green = (0x00, 0xC8, 0x50)
    red = (0xE0, 0x40, 0x40)

    radius = max(1, int(size * 4 / 64))

    # --- Background gradient ---
    for y in range(size):
        t = y / max(1, size - 1)
        color = lerp_color(bg_top, bg_bot, t)
        draw.line([(0, y), (size - 1, y)], fill=color)

    # Clip to rounded rectangle by masking corners
    mask = Image.new("L", (size, size), 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.rounded_rectangle([0, 0, size - 1, size - 1], radius=radius, fill=255)
    img.putalpha(mask)

    # --- Border ---
    draw.rounded_rectangle(
        [0, 0, size - 1, size - 1],
        radius=radius,
        outline=border_color,
        width=max(1, size // 64),
    )

    # --- "VT" text ---
    font_size = int(size * 0.45)
    try:
        font = ImageFont.truetype("arial.ttf", font_size)
    except OSError:
        try:
            font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", font_size)
        except OSError:
            font = ImageFont.load_default()

    text = "VT"
    bbox = draw.textbbox((0, 0), text, font=font)
    tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
    tx = (size - tw) // 2 - bbox[0]
    ty = int(size * 0.12) - bbox[1]
    draw.text((tx, ty), text, fill=white, font=font)

    # --- Mini candles ---
    candle_w = max(1, int(size * 2 / 64))
    candle_gap = max(1, int(size * 4 / 64))
    candle_area_top = ty + th + max(1, int(size * 0.05))
    candle_area_bot = size - max(2, int(size * 0.12))
    candle_h = candle_area_bot - candle_area_top

    colors = [green, red, green]
    heights = [0.8, 0.5, 0.65]  # relative heights
    total_w = 3 * candle_w + 2 * candle_gap
    start_x = (size - total_w) // 2

    for i, (c, h_frac) in enumerate(zip(colors, heights)):
        cx = start_x + i * (candle_w + candle_gap)
        ch = max(1, int(candle_h * h_frac))
        cy = candle_area_bot - ch
        draw.rectangle([cx, cy, cx + candle_w - 1, candle_area_bot - 1], fill=c)

    return img


def main():
    sizes = [16, 32, 48, 64, 256]
    images = [draw_icon(s) for s in sizes]

    out_dir = os.path.join(os.path.dirname(__file__), "..", "resources")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, "app_icon.ico")

    # Pillow ICO: save the largest first, append the rest
    images[-1].save(
        out_path,
        format="ICO",
        append_images=images[:-1],
    )
    print(f"Icon saved to {out_path} ({len(sizes)} sizes: {sizes})")


if __name__ == "__main__":
    main()
