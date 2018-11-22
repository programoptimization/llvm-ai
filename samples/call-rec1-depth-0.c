// OPT: "--cs_depth=0"

int foo() {
  return foo();
}

int main() {
  return foo();
}
