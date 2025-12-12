/* test_float.c - Teste de operacoes com ponto flutuante (double) */

double add_doubles(double a, double b) {
    return a + b;
}

double sub_doubles(double a, double b) {
    return a - b;
}

double mul_doubles(double a, double b) {
    return a * b;
}

double div_doubles(double a, double b) {
    return a / b;
}

double negate_double(double a) {
    return -a;
}

int double_to_int(double a) {
    return (int)a;
}

double int_to_double(int a) {
    return (double)a;
}

int main() {
    double x = 10.5;
    double y = 3.2;
    
    double sum = add_doubles(x, y);
    double diff = sub_doubles(x, y);
    double prod = mul_doubles(x, y);
    double quot = div_doubles(x, y);
    double neg = negate_double(x);
    
    int i = double_to_int(x);
    double d = int_to_double(42);
    
    return 0;
}
