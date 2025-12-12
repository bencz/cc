/* test_float_compare.c - Teste de comparacoes com double */

int compare_doubles(double a, double b) {
    if (a > b) return 1;
    if (a < b) return -1;
    return 0;
}

int main() {
    double x = 10.5;
    double y = 3.2;
    double z = 10.5;
    
    int r1 = compare_doubles(x, y);
    int r2 = compare_doubles(y, x);
    int r3 = compare_doubles(x, z);
    
    return r1 + r2 + r3;
}
