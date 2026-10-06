#!/usr/bin/env python3
import os
import io
import sys

THRESHOLD = 128

USAGE = """CrossSmudge Icon Converter & Viewer

Usage:
  # View an icon in the terminal (ASCII art) and optionally save a preview:
  python scripts/convert_icon.py view <icon.raw> [--out preview.png] [--scale 8]

  # Convert icon.raw -> editable PNG:
  python scripts/convert_icon.py raw2png <icon.raw> [output.png] [--scale 1|4|8]

  # Convert PNG/SVG -> icon.raw (for apps on SD card):
  python scripts/convert_icon.py png2raw <input.png|input.svg> [output.raw] [width height]

  # Generate C header in src/components/icons/ (for built-in firmware icons):
  python scripts/convert_icon.py header <input.png|input.svg> <output_name> [width height]

  # Legacy format (generates src/components/icons/<output_name>.h):
  python scripts/convert_icon.py <input.png|input.svg> <output_name> <width> <height>
"""

def svg_to_png_bytes(svg_path, width, height):
    import cairosvg
    with open(svg_path, "rb") as f:
        svg_data = f.read()
    return cairosvg.svg2png(bytestring=svg_data, output_width=width, output_height=height)

def load_image_for_display(path, width=32, height=32):
    from PIL import Image

    ext = os.path.splitext(path)[1].lower()
    if ext == ".svg":
        png_bytes = svg_to_png_bytes(path, width, height)
        img = Image.open(io.BytesIO(png_bytes))
    else:
        img = Image.open(path)

    img = img.convert("RGBA")
    img = img.resize((width, height), Image.LANCZOS if hasattr(Image, "LANCZOS") else Image.NEAREST)

    # Flatten alpha against pure white background
    background = Image.new("RGBA", img.size, (255, 255, 255, 255))
    background.paste(img, mask=img.split()[3])
    img = background.convert("L")

    # Rotate 90 degrees counterclockwise to match CrossSmudge GfxRenderer::drawIcon portrait transform
    rot = img.rotate(90, expand=True)
    return rot

def image_to_packed_bytes(rot_img, threshold=THRESHOLD):
    width, height = rot_img.size
    pixels = rot_img.tobytes()
    packed = bytearray()
    for y in range(height):
        for x in range(0, width, 8):
            byte = 0
            for b in range(8):
                if x + b < width:
                    v = pixels[y * width + x + b]
                    # 1 for white/transparent, 0 for black/drawn
                    bit = 1 if v >= threshold else 0
                    byte |= (bit << (7 - b))
            packed.append(byte)
    return bytes(packed)

def raw_bytes_to_image(raw_bytes, width=32, height=32):
    from PIL import Image
    rot_img = Image.new("1", (height, width), 1)
    bytes_per_row = (height + 7) // 8
    for y in range(width):
        for col in range(bytes_per_row):
            byte_val = raw_bytes[y * bytes_per_row + col]
            for bit in range(8):
                x = col * 8 + bit
                if x < height:
                    pixel = (byte_val >> (7 - bit)) & 1
                    rot_img.putpixel((x, y), pixel)
    # Rotate 90 degrees clockwise to return to standard upright orientation
    return rot_img.rotate(-90, expand=True)

def render_ascii(img):
    lines = []
    w, h = img.size
    lines.append("+" + "-" * (w * 2) + "+")
    for y in range(h):
        row = ["|"]
        for x in range(w):
            pixel = img.getpixel((x, y))
            # 0 is black (drawn), 1 is white
            row.append("██" if pixel == 0 else "  ")
        row.append("|")
        lines.append("".join(row))
    lines.append("+" + "-" * (w * 2) + "+")
    return "\n".join(lines)

def make_c_array(packed_bytes, array_name, width, height):
    c = f"#pragma once\n#include <cstdint>\n\n"
    c += f"// size: {width}x{height}\n"
    c += f"static const uint8_t {array_name}[] = {{\n    "
    for i, v in enumerate(packed_bytes):
        c += f"0x{v:02X}, "
        if (i + 1) % 16 == 0:
            c += "\n    "
    c = c.rstrip(", \n") + "\n};\n"
    return c

def main():
    args = sys.argv[1:]
    if not args or any(a in ("-h", "--help") for a in args):
        print(USAGE)
        sys.exit(0)

    cmd = args[0].lower()

    if cmd == "view":
        if len(args) < 2:
            print("Usage: python scripts/convert_icon.py view <icon.raw> [--out preview.png] [--scale 8]")
            sys.exit(1)
        raw_path = args[1]
        out_path = None
        scale = 8
        i = 2
        while i < len(args):
            if args[i] == "--out" and i + 1 < len(args):
                out_path = args[i + 1]
                i += 2
            elif args[i] == "--scale" and i + 1 < len(args):
                scale = int(args[i + 1])
                i += 2
            else:
                i += 1

        with open(raw_path, "rb") as f:
            raw_bytes = f.read()

        img = raw_bytes_to_image(raw_bytes, 32, 32)
        print(f"\n--- Icon Preview: {raw_path} (32x32) ---")
        print(render_ascii(img))
        print("-------------------------------------------\n")

        if out_path:
            from PIL import Image
            preview = img.resize((32 * scale, 32 * scale), Image.NEAREST)
            preview.save(out_path)
            print(f"Saved scaled preview ({32 * scale}x{32 * scale}) to {out_path}")

    elif cmd == "raw2png":
        if len(args) < 2:
            print("Usage: python scripts/convert_icon.py raw2png <icon.raw> [output.png] [--scale 1|4|8]")
            sys.exit(1)
        raw_path = args[1]
        out_path = None
        scale = 1
        i = 2
        while i < len(args):
            if args[i] == "--scale" and i + 1 < len(args):
                scale = int(args[i + 1])
                i += 2
            elif not args[i].startswith("--") and out_path is None:
                out_path = args[i]
                i += 1
            else:
                i += 1

        if out_path is None:
            base, _ = os.path.splitext(raw_path)
            out_path = base + ".png"

        with open(raw_path, "rb") as f:
            raw_bytes = f.read()

        img = raw_bytes_to_image(raw_bytes, 32, 32)
        from PIL import Image
        if scale > 1:
            img = img.resize((32 * scale, 32 * scale), Image.NEAREST)
        img.save(out_path)
        print(f"Exported {raw_path} -> {out_path} ({img.width}x{img.height})")

    elif cmd == "png2raw":
        if len(args) < 2:
            print("Usage: python scripts/convert_icon.py png2raw <input.png|svg> [output.raw] [width height]")
            sys.exit(1)
        input_path = args[1]
        out_path = None
        width, height = 32, 32
        non_flags = [a for a in args[2:] if not a.startswith("--")]
        if len(non_flags) == 1:
            out_path = non_flags[0]
        elif len(non_flags) == 2:
            width, height = int(non_flags[0]), int(non_flags[1])
        elif len(non_flags) >= 3:
            out_path = non_flags[0]
            width, height = int(non_flags[1]), int(non_flags[2])

        if out_path is None:
            base, _ = os.path.splitext(input_path)
            out_path = base + ".raw"

        rot_img = load_image_for_display(input_path, width, height)
        packed = image_to_packed_bytes(rot_img)
        with open(out_path, "wb") as f:
            f.write(packed)
        print(f"Converted {input_path} -> {out_path} ({len(packed)} bytes, {width}x{height})")

    elif cmd == "header":
        if len(args) < 3:
            print("Usage: python scripts/convert_icon.py header <input.png|svg> <output_name> [width height]")
            sys.exit(1)
        input_path, output_name = args[1], args[2]
        width = int(args[3]) if len(args) > 3 else 32
        height = int(args[4]) if len(args) > 4 else 32
        array_name = output_name[0].upper() + output_name[1:] + "Icon"
        rot_img = load_image_for_display(input_path, width, height)
        packed = image_to_packed_bytes(rot_img)
        c_array = make_c_array(packed, array_name, width, height)

        project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        output_dir = os.path.join(project_root, "src", "components", "icons")
        os.makedirs(output_dir, exist_ok=True)
        output_path = os.path.join(output_dir, f"{output_name}.h")
        with open(output_path, "w") as f:
            f.write(c_array)
        print(f"Generated C header: {output_path} ({len(packed)} bytes)")

    elif len(args) == 4 and not args[0].startswith("-"):
        # Legacy syntax: input.png output_name width height
        input_path, output_name, width, height = args
        width, height = int(width), int(height)
        array_name = output_name[0].upper() + output_name[1:] + "Icon"
        rot_img = load_image_for_display(input_path, width, height)
        packed = image_to_packed_bytes(rot_img)
        c_array = make_c_array(packed, array_name, width, height)

        project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        output_dir = os.path.join(project_root, "src", "components", "icons")
        os.makedirs(output_dir, exist_ok=True)
        output_path = os.path.join(output_dir, f"{output_name}.h")
        with open(output_path, "w") as f:
            f.write(c_array)
        print(f"Wrote {output_path}")

    else:
        print(f"Unknown command or invalid arguments: {' '.join(args)}")
        print(USAGE)
        sys.exit(1)

if __name__ == "__main__":
    main()
