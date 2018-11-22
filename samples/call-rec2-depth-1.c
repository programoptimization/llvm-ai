// OPT: "--cs_depth=1"

int foo(int x) {
  return foo(x + 1);
}

int main() {
  return foo(0);
}
