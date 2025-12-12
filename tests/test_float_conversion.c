/* test_float_conversion.c - Teste de conversoes int/double */

int main() {
    int i = 42;
    double d = (double)i;
    
    double pi = 3.14159;
    int truncated = (int)pi;
    
    double neg = -7.8;
    int neg_int = (int)neg;
    
    int sum = truncated + neg_int;
    
    return sum;
}
