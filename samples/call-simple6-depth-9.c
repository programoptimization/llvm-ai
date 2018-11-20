// OPT: "--cs_depth=9"
int baz() {
  return 5;
}

int foo() {
  int x = baz();
  return x + 5;
}

int main() {
  return foo();
}
