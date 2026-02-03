#define PORTB 0x6000

unsigned char test_array[13] = "Hola Mundo!\n";

int main(void) {
    int i;
    for (i = 0; i < sizeof(test_array); i++) {
        (*(volatile unsigned char*)PORTB) = test_array[i];
    }
    return 0;
}
