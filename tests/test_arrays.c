/* test_arrays.c - Teste de arrays */
int main() {
    int arr[5];
    int i;
    int sum = 0;
    
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    arr[3] = 4;
    arr[4] = 5;
    
    for (i = 0; i < 5; i++) {
        sum = sum + arr[i];
    }
    
    return sum;
}
