int main() {
    int a, b, c;
    b = 0; // 0 = b
    a = b; // 0 = a = b
    c = b; // 0 = a = b = c

    for (int i = 0; i < 3; i++) {
        a = 2; // 0 = b = c, 2 = a
        c = 2; // 0 = b, 2 = a = c
        b = 1; // 1 = b, 2 = a = c
    }

    return a + c;
}

// a: {a,c} (equality in for-loop detected!)