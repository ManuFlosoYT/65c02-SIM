import os
import sys
from PIL import Image

def main():
    if len(sys.argv) < 3:
        sys.exit(f"Usage: {sys.argv[0]} <input_image> <output_asm>")

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    out_dir = os.path.dirname(output_path)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    with Image.open(input_path) as img:
        img = img.convert("RGBA").resize((16, 16), Image.Resampling.LANCZOS)

        lines = [
            '.section .header.1_icon, "a"',
            ".globl app_icon",
            "app_icon:",
        ]

        for y in range(16):
            row = []
            for x in range(16):
                r, g, b, a = img.getpixel((x, y))
                if a < 128:
                    byte_val = 0
                else:
                    r2 = min(3, r // 64)
                    g2 = min(3, g // 64)
                    b2 = min(3, b // 64)
                    byte_val = (r2 << 4) | (g2 << 2) | b2
                row.append(f"0x{byte_val:02x}")
            lines.append("    .byte " + ", ".join(row))

    with open(output_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")

if __name__ == "__main__":
    main()
