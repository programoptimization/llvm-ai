

int main(int argc, char** argv) {
    int a = argc % 7 + 10;

    while (a != 20) ++a;

    int b = a;
    int c;
    if (b < 20) {
        c = argc % 355;
    } else {
        c = 5;
    }
    return c;
}
