/* test_do_while.c - Teste de do-while loop */
int main() {
    int count = 0;
    int n = 5;
    
    do {
        count = count + n;
        n = n - 1;
    } while (n > 0);
    
    return count;
}
