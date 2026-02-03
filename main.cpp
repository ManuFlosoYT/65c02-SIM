#include <cstdio>
#include <iostream>

#include "Componentes/CPU.h"
#include "Componentes/LCD.h"
#include "Componentes/Mem.h"

void CargarBinario(Mem& mem, const std::string& bin) {
    std::string ruta = "Programas/build/" + bin + ".bin";

    FILE* fichero = fopen(ruta.c_str(), "rb");
    if (fichero == nullptr) {
        std::cerr << "Error al abrir el archivo " << ruta << std::endl;
        return;
    }

    // asegurar que el bin tiene tamaño correcto
    fseek(fichero, 0, SEEK_END);
    long fileSize = ftell(fichero);
    if (fileSize != mem.ROM_SIZE) {
        std::cerr << "Error: El archivo " << ruta << " no tiene tamaño "
                  << mem.ROM_SIZE << std::endl;
        return;
    }
    fseek(fichero, 0, SEEK_SET);

    // leer el bin en la memoria (0x8000-0xFFFF)
    size_t bytesRead = fread(mem.memoria + 0x8000, 1, mem.ROM_SIZE, fichero);
    if (bytesRead == 0) {
        std::cerr << "Error al leer el archivo " << ruta << std::endl;
        return;
    }

    fclose(fichero);
}

int main(int argc, char* argv[]) {
    Mem mem{};
    CPU cpu{};
    LCD lcd;
    cpu.Reset(mem);
    lcd.init(mem);

    std::string bin;
    if (argc > 1) {
        bin = argv[1];
    } else {
        std::cout << "Introduce el nombre del binario sin extension:";
        std::cin >> bin;
    }

    CargarBinario(mem, bin);

    std::cout << "--- Lanzando programa ---\n";

    cpu.Ejecutar(mem);

    std::cout << "\n--- Ejecucion finalizada ---\n";
}