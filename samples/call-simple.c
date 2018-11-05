
int specialfun(int i, int other) {
  
  if (i != 0) {
    i = i + 1;
  }

  for (int c = 0; c < other; ++c) {
    i = i + 1;
  }

  return i;
}

int main() {
  volatile int input = 1;
  int i = specialfun(0, input);
  return 0;
}
