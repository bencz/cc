/* test_struct_ptr.c - Teste de ponteiro para estrutura */
typedef struct _Node {
    int value;
    int next;
} Node;

int main() {
    Node n;
    Node *p;
    
    n.value = 42;
    n.next = 0;
    
    p = &n;
    return p->value;
}
