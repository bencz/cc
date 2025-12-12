/* test_include_stdlib.c - Teste de #include <stdlib.h> */
#include <stdlib.h>

int main() {
    int max = RAND_MAX;
    int path_len = MAX_PATH;
    return max > 0 ? 1 : 0;
}
