// OPT: "--cs_depth=0"
int baz(int x) {
  return x * x;
}

int foo(int x, int y) {
  return x + y;
}

int main() {
  // Explicitily evaluate baz(2) before baz(3) since the argument
  // evaluation order is not defined until C++20 and this will lead
  // to platform dependent behaviour which produces different ordered
  // test output.
  int a = baz(2);
  int b = baz(3);
  return foo(a, b);
}
