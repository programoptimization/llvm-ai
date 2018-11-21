int test(){
    int i = 0;
    loop:
    i++;
    if(i<10)
        goto loop;
    return i;
}