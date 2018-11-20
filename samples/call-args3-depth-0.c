// OPT: "--cs_depth=0"
int baz(int x) {
  return x * x;
}

int foo(int x, int y) {
  return x + y;
}

int main() {
  return foo(baz(2), baz(3));
}
