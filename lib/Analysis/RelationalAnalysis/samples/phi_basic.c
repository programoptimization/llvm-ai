int main() {
    int x = 42;
	int y = 1;

    for (int i = 0; i < 1; i++) {
        x = 43;
    }

    return x;
}

// 43: {x}
// constant assignment in for loop works, no computations possible
