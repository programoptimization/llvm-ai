int main() {
  int a = 1;
  int b = 2;
  int c = a + b;

  while (a < b) {
    b++;
    c = a + b;
  }

  b = 42;

  return c;
}

// 1: {a}
// 42: {b}
