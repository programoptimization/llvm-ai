int baz(int i, int j) {
  return (i - 2) * (j + 4);
}

int foo(int x, int y) {
  x = x + 2;
  y = y - 4;

  return baz(x, y);
}

int main() {
  int a = 12;
  int b = 6;

  return foo(a - 2, b + 4);
}
