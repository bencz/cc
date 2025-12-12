/* test_switch.c - Teste de switch/case */
int main() {
    int x = 2;
    int result;
    
    switch (x) {
        case 1:
            result = 10;
            break;
        case 2:
            result = 20;
            break;
        case 3:
            result = 30;
            break;
        default:
            result = 0;
    }
    
    return result;
}
