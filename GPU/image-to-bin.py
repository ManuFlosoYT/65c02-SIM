import sys
import os
from PIL import Image

# --- CONFIGURACIÓN DE HARDWARE ---
VRAM_WIDTH = 100    # Ancho visible
VRAM_HEIGHT = 75    # Alto visible
STRIDE = 128        # Salto de memoria por fila (addressing & 0x7F)

# --- RUTAS ---
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
OUTPUT_DIR = os.path.join(os.path.dirname(BASE_DIR), "output")

def quantize_color_2bit(value):
    """Convierte 0-255 a 0-3"""
    res = value // 64
    if res > 3: res = 3
    return res

def convert_image(image_path):
    if not os.path.exists(image_path):
        print(f"Error: File '{image_path}' not found.")
        return

    filename = os.path.basename(image_path)
    name, _ = os.path.splitext(filename)
    
    # Crear carpeta output en la raíz si no existe
    if not os.path.exists(OUTPUT_DIR):
        try:
            os.makedirs(OUTPUT_DIR)
            print(f"Created output folder: {OUTPUT_DIR}")
        except OSError as e:
            print(f"Error creating output folder: {e}")
            return
    
    output_path = os.path.join(OUTPUT_DIR, f"{name}.bin")

    try:
        # 1. Cargar y procesar imagen
        im_source = Image.open(image_path).convert("RGB")
        im_visible = im_source.resize((VRAM_WIDTH, VRAM_HEIGHT), Image.Resampling.LANCZOS)
        
        print(f"Processing: {filename}")
        print(f"  - Destination: {output_path}")

        # 2. Preparar Buffer
        total_memory_size = VRAM_HEIGHT * STRIDE
        memory_buffer = bytearray(total_memory_size)
        
        pixels = im_visible.load()

        # 3. Conversión RRGGBB
        for y in range(VRAM_HEIGHT):
            for x in range(VRAM_WIDTH):
                r, g, b = pixels[x, y]
                
                # Cuantizar a 2 bits
                r2 = quantize_color_2bit(r)
                g2 = quantize_color_2bit(g)
                b2 = quantize_color_2bit(b)
                
                # Empaquetar: R(bits 4-5) | G(bits 2-3) | B(bits 0-1)
                packed_byte = (r2 << 4) | (g2 << 2) | b2
                
                # Dirección de memoria: (y * 128) + x
                memory_address = (y * STRIDE) + x
                
                memory_buffer[memory_address] = packed_byte

        # 4. Guardar
        with open(output_path, "wb") as f:
            f.write(memory_buffer)

        print(f"-> SUCCESS. Generated at: {output_path}")

    except Exception as e:
        print(f"Unexpected error: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Internal use only.")
    else:
        convert_image(sys.argv[1])