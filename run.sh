#!/bin/bash
set -e

if [ -z "$1" ]; then
    echo "Uso: ./run.sh <nombre_bin_sin_extension> o ./run.sh basic"
    exit 1
fi

mkdir -p ./Programas/build

echo "--- Compilando $1 ---"

# Variables iniciales
ASM_FLAGS=""
SOURCES=""
LINKER_CONFIG="Linker/memoria.cfg"

# --- Lógica de Selección ---

if [ "$1" == "basic" ]; then
    # 1. OPCIÓN ESPECIAL: BASIC (Compilación directa ASM)
    echo " > Modo: BASIC Standalone (Sin wrapper C)"
    
    # Definiciones para MSBASIC
    ASM_FLAGS="--asm-define eater -I Linker/msbasic"
    LINKER_CONFIG="Linker/memoria_asm.cfg"
    
    # AQUÍ ESTÁ LA CLAVE: Solo pasamos los archivos .s, ignoramos basic.c
    # Nota: Asegúrate de que Linker/inicio.s sepa saltar a COLD_START si no hay main()
    SOURCES="Linker/bios.s Linker/inicio_basic.s Linker/msbasic/msbasic.s"

elif [ "$1" == "wozmon" ]; then
    # 2. OPCIÓN ESPECIAL: WOZMON (Compilación directa ASM)
    echo " > Modo: WozMon Standalone (Sin wrapper C)"
    LINKER_CONFIG="Linker/memoria_asm.cfg"
    SOURCES="Linker/bios.s Linker/inicio_wozmon.s"

else
    # 2. OPCIÓN ESTÁNDAR: Programas en C
    SOURCE_C="./Programas/$1.c"
    
    if [ ! -f "$SOURCE_C" ]; then
        echo "Error: El fichero $SOURCE_C no existe. Revisa que esté en la carpeta Programas."
        exit 1
    fi
    
    # Incluimos el archivo C en la compilación
    SOURCES="Linker/bios.s Linker/inicio.s $SOURCE_C"
fi

# --- Compilación (cl65) ---

# Se usa la variable $SOURCES que hemos construido arriba
cl65 -O --cpu 65C02 -t none -C $LINKER_CONFIG $ASM_FLAGS \
    -o "./Programas/build/$1.bin" \
    -m "./Programas/build/$1.map" \
    -l "./Programas/build/$1.lst" \
    $SOURCES

# Mover el objeto compilado (si se genera en la raíz) a build para limpieza
# El '2>/dev/null' evita errores si cl65 no generó el .o ahí
mv "./Programas/$1.o" "./Programas/build/$1.o" 2>/dev/null || true

echo "Binario compilado correctamente"
echo ""

# Ejecutar el simulador
./build/SIM_65C02 "$1"