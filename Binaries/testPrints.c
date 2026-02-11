#include "bios.h"
#include "lcd.h"


int main(void) {
    unsigned int n;

    // ---------------------------------------------------------
    // 1. INICIALIZACIÓN
    // ---------------------------------------------------------
    print_str("[BIOS] Inicio.\n");
    
    lcd_inicializar();
    lcd_imprimir("Iniciado...");
    
    print_str("[BIOS] LCD listo. Esperando...\n");
    delay(1000); 

    // ---------------------------------------------------------
    // 2. TEXTO Y SALTO
    // ---------------------------------------------------------
    lcd_limpiar(); /* Limpiamos lo anterior */
    
    lcd_imprimir("Linea 1");
    lcd_salto_linea();
    lcd_imprimir("Linea 2");
    
    print_str("[BIOS] Texto escrito (2 lineas).\n");
    delay(1000); 

    // ---------------------------------------------------------
    // 3. PRUEBA DE LIMPIEZA
    // ---------------------------------------------------------
    lcd_limpiar();
    
    print_str("[BIOS] Pantalla Clear.\n");
    /* Pausa para que veas la pantalla vacia */
    delay(1000); 

    // ---------------------------------------------------------
    // 4. CONTEO DE NUMEROS
    // ---------------------------------------------------------
    lcd_imprimir("Nums: ");
    
    for (n = 0; n <= 9; n++) {
        lcd_imprimir_numero(n);
        lcd_imprimir_caracter(' '); 
        
        bios_putchar('.'); /* Feedback visual en consola */
        
        /* Usamos el delay masivo entre CADA número */
        delay(1000);
    }
    
    bios_putchar('\n'); 

    // ---------------------------------------------------------
    // 5. FIN
    // ---------------------------------------------------------
    lcd_limpiar();
    lcd_imprimir("Fin.");
    print_str("[BIOS] Fin del programa.\n");
    
    return 0;
}