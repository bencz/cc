/* test_float_arithmetic.c - Teste de aritmetica com double */

int main() {
    double a = 100.5;
    double b = 25.25;
    
    double sum = a + b;
    double diff = a - b;
    double prod = a * b;
    double quot = a / b;
    
    double neg = -a;
    
    int i = (int)sum;
    
    return i;
}
