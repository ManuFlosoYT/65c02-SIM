unsigned char test_array[4] = {10, 20, 30, 40};

int main(void) {
    (*(volatile unsigned char*)0x6767) = test_array[0];
    return 0;
}
