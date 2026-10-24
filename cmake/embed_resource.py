#!/usr/bin/env python3
import argparse
import os
import struct
import subprocess
import sys


def embed_binary(input_file: str, var_name: str, output_header: str) -> None:
    data = b""
    if os.path.exists(input_file):
        with open(input_file, "rb") as f:
            data = f.read()
    else:
        print(f"Warning: {input_file} not found, generating empty vector.")

    out = [
        "#pragma once\n",
        "#include <vector>\n",
        "#include <cstdint>\n\n",
        f"static const std::vector<uint8_t> {var_name} = {{\n",
    ]

    chunk_size = 16
    for i in range(0, len(data), chunk_size):
        chunk = data[i : i + chunk_size]
        hex_str = ", ".join(f"0x{b:02x}" for b in chunk)
        out.append(f"    {hex_str},\n")

    out.append("};\n")

    os.makedirs(os.path.dirname(os.path.abspath(output_header)), exist_ok=True)
    with open(output_header, "w", encoding="utf-8") as f:
        f.writelines(out)

    print(f"Generated {output_header} with {len(data)} bytes.")


def embed_vfs(
    llvm_mos_root: str,
    linker_dir: str,
    libs_dir: str,
    output_header: str,
    microdos_libs_dir: str | None = None,
) -> None:
    embed_dirs: list[tuple[str, str]] = [
        (linker_dir, "Linker"),
        (libs_dir, "include/Libs"),
    ]

    if microdos_libs_dir and os.path.exists(microdos_libs_dir):
        embed_dirs.append((microdos_libs_dir, "include/Libs"))

    if llvm_mos_root and os.path.exists(llvm_mos_root):
        common_platform = os.path.join(llvm_mos_root, "mos-platform", "common")
        if os.path.exists(common_platform):
            embed_dirs.append((common_platform, "mos-platform/common"))

    files_to_process: list[tuple[str, str]] = []
    for physical_dir, virtual_prefix in embed_dirs:
        if not os.path.exists(physical_dir):
            continue
        for root, _, files in os.walk(physical_dir):
            for fname in files:
                full_path = os.path.join(root, fname)
                rel_path = os.path.relpath(full_path, physical_dir)
                virtual_path = f"{virtual_prefix}/{rel_path}".replace("\\", "/")
                files_to_process.append((full_path, virtual_path))

    out = [
        "#pragma once\n",
        "#include <vector>\n",
        "#include <string>\n",
        "#include <map>\n",
        "#include <cstdint>\n\n",
        "static const std::map<std::string, std::vector<uint8_t>> embedded_llvm_mos_files = {\n",
    ]

    chunk_size = 32
    for full_path, vpath in files_to_process:
        try:
            with open(full_path, "rb") as f:
                data = f.read()
        except OSError:
            continue

        out.append(f'  {{"{vpath}", {{\n')
        for i in range(0, len(data), chunk_size):
            chunk = data[i : i + chunk_size]
            hex_str = ", ".join(f"0x{b:02x}" for b in chunk)
            out.append(f"    {hex_str},\n")
        out.append("  }},\n")

    out.append("};\n")

    os.makedirs(os.path.dirname(os.path.abspath(output_header)), exist_ok=True)
    with open(output_header, "w", encoding="utf-8") as f:
        f.writelines(out)

    print(f"Generated {output_header} with {len(files_to_process)} embedded files.")


def embed_shaders(vert_file: str, frag_file: str, output_header: str) -> None:
    with open(vert_file, "r", encoding="utf-8") as f:
        vert_content = f.read()
    with open(frag_file, "r", encoding="utf-8") as f:
        frag_content = f.read()

    content = (
        "#pragma once\n\n"
        f'static const char* kCRTVertSrc = R"glsl(\n{vert_content}\n)glsl";\n\n'
        f'static const char* kCRTFragSrc = R"glsl(\n{frag_content}\n)glsl";\n'
    )

    os.makedirs(os.path.dirname(os.path.abspath(output_header)), exist_ok=True)
    with open(output_header, "w", encoding="utf-8") as f:
        f.write(content)


def embed_icon(magick_exec: str, input_png: str, output_header: str) -> None:
    if not magick_exec:
        sys.exit("ImageMagick executable not provided.")

    os.makedirs(os.path.dirname(os.path.abspath(output_header)), exist_ok=True)
    temp_bin = f"{output_header}.bin"
    try:
        res = subprocess.run(
            [magick_exec, input_png, "-depth", "8", f"RGBA:{temp_bin}"],
            check=False,
        )
        if res.returncode != 0:
            sys.exit("Failed to convert PNG to raw pixels using ImageMagick.")

        with open(temp_bin, "rb") as f:
            data = f.read()
    finally:
        if os.path.exists(temp_bin):
            os.remove(temp_bin)

    hex_array = ", ".join(f"0x{b:02x}" for b in data)
    content = (
        "#pragma once\n"
        "#include <cstdint>\n\n"
        "const uint8_t icon_pixels_bin[] = {\n"
        f"    {hex_array}\n"
        "};\n\n"
        f"const uint32_t icon_pixels_bin_len = {len(data)};\n"
    )

    with open(output_header, "w", encoding="utf-8") as f:
        f.write(content)

    print(f"Generated {output_header} ({len(data)} bytes)")


def embed_cascadia(input_file: str, output_file: str) -> None:
    with open(input_file, "r", encoding="utf-8") as f:
        data = "".join(line.strip() for line in f)

    data = data.replace("??", r"?\?")

    chunk_size = 140
    out = [
        '#include "Frontend/GUI/Style/CascadiaMono.h"\n\n',
        "static const char kCascadiaMonoCompressedBase85[] =\n",
    ]
    for i in range(0, len(data), chunk_size):
        chunk = data[i : i + chunk_size]
        out.append(f'    "{chunk}"\n')
    out.append(";\n\n")
    out.append("const char* GetCascadiaMonoCompressedBase85() noexcept {\n")
    out.append("    return kCascadiaMonoCompressedBase85;\n")
    out.append("}\n")

    os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)
    with open(output_file, "w", encoding="utf-8") as f:
        f.writelines(out)

    print(f"Generated {output_file} ({len(data)} base85 characters)")


def embed_sid_wavetables(input_file: str, output_file: str) -> None:
    with open(input_file, "rb") as f:
        data = f.read()

    expected_bytes = 4 * 4096 * 2
    if len(data) != expected_bytes:
        sys.exit(f"Error: {input_file} size {len(data)} != expected {expected_bytes} bytes.")

    arrays = ["wave6581_PST", "wave6581_PS_", "wave6581_P_T", "wave6581__ST"]
    out = [
        '#include "Hardware/Audio/SIDWaveTables.h"\n\n',
        "namespace Hardware {\n\n",
    ]

    offset = 0
    chunk_size = 8
    for arr_name in arrays:
        values = struct.unpack_from("<4096H", data, offset)
        offset += 4096 * 2
        out.append(f"extern const uint16_t {arr_name}[4096] = {{\n")
        for i in range(0, len(values), chunk_size):
            chunk = values[i : i + chunk_size]
            hex_str = ", ".join(f"0x{v:04x}" for v in chunk)
            out.append(f"    {hex_str},\n")
        out.append("};\n\n")

    out.append("}  // namespace Hardware\n")

    os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)
    with open(output_file, "w", encoding="utf-8") as f:
        f.writelines(out)

    print(f"Generated {output_file} (4 SID wave tables)")


def main() -> None:
    parser = argparse.ArgumentParser(description="Embed build resources into C++ headers.")
    parser.add_argument("--mode", required=True, choices=["binary", "vfs", "shaders", "icon", "cascadia", "sid_wavetables"])
    parser.add_argument("--output", required=True, help="Output header path")
    parser.add_argument("--input", help="Input file path (binary / icon modes)")
    parser.add_argument("--var", help="C++ variable name (binary mode)")
    parser.add_argument("--llvm-mos-root", default="", help="LLVM-MOS root directory (vfs mode)")
    parser.add_argument("--linker-dir", help="Linker directory (vfs mode)")
    parser.add_argument("--libs-dir", help="Libs directory (vfs mode)")
    parser.add_argument("--microdos-libs-dir", default=None, help="MicroDOS Libs directory (vfs mode)")
    parser.add_argument("--vert", help="Vertex shader path (shaders mode)")
    parser.add_argument("--frag", help="Fragment shader path (shaders mode)")
    parser.add_argument("--magick", help="ImageMagick executable (icon mode)")

    args = parser.parse_args()

    if args.mode == "binary":
        embed_binary(args.input, args.var, args.output)
    elif args.mode == "vfs":
        embed_vfs(args.llvm_mos_root, args.linker_dir, args.libs_dir, args.output, args.microdos_libs_dir)
    elif args.mode == "shaders":
        embed_shaders(args.vert, args.frag, args.output)
    elif args.mode == "icon":
        embed_icon(args.magick, args.input, args.output)
    elif args.mode == "cascadia":
        embed_cascadia(args.input, args.output)
    elif args.mode == "sid_wavetables":
        embed_sid_wavetables(args.input, args.output)


if __name__ == "__main__":
    main()
