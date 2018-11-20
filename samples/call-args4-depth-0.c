// OPT: "--cs_depth=0"
int baz(int x) {
  return x * x;
}

int foo(int x) {
  int z = x * 2;
  return baz(z);
}

int main() {
  return foo(10);
}
