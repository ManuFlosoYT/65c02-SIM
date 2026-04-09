import os

_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

def write_bin(bytecode, input_path):
    """
    Write raw SID bytecode to a .sid file directly.

    Args:
        bytecode: list of integer byte values
        input_path: original MIDI file path (used to derive output filename)
    """
    filename_only = os.path.splitext(os.path.basename(input_path))[0].replace(" ", "") + ".sid"
    # Salida directa a la carpeta output/midi, asumiendo root del emulador a 2 niveles 
    project_root = os.path.dirname(os.path.dirname(_SCRIPT_DIR))
    output_dir = os.path.join(project_root, "output", "midi")
    
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        
    output_file = os.path.join(output_dir, filename_only)

    print(f"Writing raw SID binary to {output_file}...")

    with open(output_file, "wb") as f:
        f.write(bytes(bytecode))

    size_bytes = len(bytecode)
    print(f"Generated {output_file}")
    print(f"File size: {size_bytes} bytes ({size_bytes / 1024.0:.1f} KB)")

    return output_file
