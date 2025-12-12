/* test_nested_loops.c - Teste de loops aninhados */
int main() {
    int sum = 0;
    int i;
    int j;
    
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            sum = sum + i + j;
        }
    }
    
    return sum;
}
