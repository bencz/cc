/* test_struct.c - Teste de estruturas */
typedef struct _Point {
    int x;
    int y;
} Point;

int main() {
    Point p;
    p.x = 10;
    p.y = 20;
    return p.x + p.y;
}
