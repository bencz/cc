/* test_include_stdarg.c - Teste de #include <stdarg.h> */
#include <stdarg.h>

int sum_args(int count, ...) {
    int total = 0;
    int i;
    for (i = 0; i < count; i++) {
        total = total + 1;
    }
    return total;
}

int main() {
    return sum_args(3);
}
