/* test_bitwise.c - Teste de operacoes bitwise */
int main() {
    int a = 0x0F;
    int b = 0xF0;
    int and_result = a & b;
    int or_result = a | b;
    int xor_result = a ^ b;
    int shift_left = a << 4;
    int shift_right = b >> 4;
    return and_result + or_result + xor_result + shift_left + shift_right;
}
