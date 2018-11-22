int main() {
  int a = 5;
  int b = 2;
  int c = a + b;
  for (int i = b; i < 20; i++) {
    b = i;
    c = i % a;
  }
  return c;
}

// 5: {a}
