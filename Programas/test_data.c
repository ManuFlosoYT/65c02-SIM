unsigned char test_array[12] = {'H', 'o', 'l', 'a', ' ', 'M', 'u', 'n', 'd', 'o', '\n', '\0'};

int main(void) {
    (*(volatile unsigned char*)0x6000) = test_array[0];
    (*(volatile unsigned char*)0x6000) = test_array[1];
    (*(volatile unsigned char*)0x6000) = test_array[2];
    (*(volatile unsigned char*)0x6000) = test_array[3];
    (*(volatile unsigned char*)0x6000) = test_array[4];
    (*(volatile unsigned char*)0x6000) = test_array[5];
    (*(volatile unsigned char*)0x6000) = test_array[6];
    (*(volatile unsigned char*)0x6000) = test_array[7];
    (*(volatile unsigned char*)0x6000) = test_array[8];
    (*(volatile unsigned char*)0x6000) = test_array[9];
    (*(volatile unsigned char*)0x6000) = test_array[10];
    (*(volatile unsigned char*)0x6000) = test_array[11];
    return 0;
}
