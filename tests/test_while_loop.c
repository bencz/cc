/* test_while_loop.c - Teste de while loop */
int main() {
    int count = 0;
    int n = 10;
    
    while (n > 0) {
        count = count + n;
        n = n - 1;
    }
    
    return count;
}
