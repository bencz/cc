/* test_include_string.c - Teste de #include <string.h> */
#include <string.h>

int my_strlen(char *s) {
    int len = 0;
    while (*s != 0) {
        len = len + 1;
        s = s + 1;
    }
    return len;
}

int main() {
    char str[10];
    str[0] = 'H';
    str[1] = 'e';
    str[2] = 'l';
    str[3] = 'l';
    str[4] = 'o';
    str[5] = 0;
    return my_strlen(str);
}
