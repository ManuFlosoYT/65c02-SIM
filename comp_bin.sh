#!/bin/bash

mkdir -p ./Programas/build

if [ -z "$1" ]; then
    echo "Error: Parametro vacio, introduce el nombre del binario sin extension"
    exit 1
fi

if [ "$#" -gt 1 ]; then
    echo "Error: Hay parametros de mas, introduce solo el nombre del binario sin extension"
    exit 1
fi

SOURCE="./Programas/$1.c"

if [ ! -f "$SOURCE" ]; then
    echo "Error: El fichero no existe, revisa que este en la carpeta Programas y que el nombre sea correcto (sin extension)"
    exit 1
fi

cl65 -O --cpu 65C02 -t none -C Linker/memoria.cfg -o "./Programas/build/$1.bin" -m "./Programas/build/$1.map" -l "./Programas/build/$1.lst" Linker/inicio.s "./Programas/$1.c"
mv ./Programas/$1.o ./Programas/build/$1.o
echo "Binario compilado correctamente"