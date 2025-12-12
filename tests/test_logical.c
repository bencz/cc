/* test_logical.c - Teste de operadores logicos */
int main() {
    int a = 1;
    int b = 0;
    int c = 5;
    
    int and_result = a && c;
    int or_result = b || c;
    int not_result = !b;
    
    if (a && c) {
        return 1;
    }
    return 0;
}
