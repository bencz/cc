/* test_comparison.c - Teste de operadores de comparacao */
int main() {
    int a = 10;
    int b = 20;
    int result = 0;
    
    if (a < b) result = result + 1;
    if (a <= b) result = result + 2;
    if (b > a) result = result + 4;
    if (b >= a) result = result + 8;
    if (a == 10) result = result + 16;
    if (a != b) result = result + 32;
    
    return result;
}
