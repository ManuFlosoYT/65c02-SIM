#include <bitset>
#include <cstdio>
#include <iomanip>
#include <iostream>

#include "Componentes/CPU.h"
#include "Componentes/Mem.h"

int main(int argc, char* argv[]) {
    Mem mem{};
    CPU cpu{};
    cpu.Reset(mem);

    const int MAX_BIN_SIZE = (mem.MAX_MEM / 2) + 1;

    std::string bin;
    if (argc > 1) {
        bin = argv[1];
    } else {
        std::cout << "Introduce el nombre del binario sin extension:";
        std::cin >> bin;
    }

    std::string ruta = "Programas/" + bin + ".bin";

    FILE* fichero = fopen(ruta.c_str(), "rb");
    if (fichero == nullptr) {
        std::cerr << "Error al abrir el archivo " << ruta << std::endl;
        return 1;
    }

    // asegurar que el bin tiene tamaño correcto

    fseek(fichero, 0, SEEK_END);
    long fileSize = ftell(fichero);
    if (fileSize != MAX_BIN_SIZE) {
        std::cerr << "Error: El archivo " << ruta << " no tiene tamaño "
                  << MAX_BIN_SIZE << std::endl;
        return 1;
    }
    fseek(fichero, 0, SEEK_SET);

    // leer el bin en la memoria (0x8000-0xFFFF)
    size_t bytesRead = fread(mem.memoria + 0x8000, 1, MAX_BIN_SIZE, fichero);
    if (bytesRead == 0) {
        std::cerr << "Error al leer el archivo " << ruta << std::endl;
        return 1;
    }

    fclose(fichero);

    std::cout << "--- Lanzando programa ---" << std::endl;

    cpu.Ejecutar(mem);

    std::cout << "--- Ejecucion finalizada ---" << std::endl;

    std::cout << "0x6767: 0x" << std::hex << std::uppercase << std::setw(2)
              << std::setfill('0') << (int)mem.memoria[0x6767] << " ("
              << std::bitset<8>(mem.memoria[0x6767]) << ")" << std::endl;

    std::cout << "PC: 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << cpu.PC << std::endl;
    std::cout << "SP: 0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << cpu.SP << std::endl;

    const int32_t flags = cpu.GetStatus();

    std::cout << "Flags: 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
              << flags << " (" << std::bitset<8>(flags) << ")" << std::endl;
}