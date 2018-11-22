// OPT: "--cs_depth=1"

int foo() {
  return foo();
}

int main() {
  return foo();
}
