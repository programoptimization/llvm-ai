int main() {
  int a = 10;
  int b = 5;
  int c = 20;
  while (a > b) {
    if (b > 0)
      b--;
    a--;
    c--;
  }
  return c;
}
