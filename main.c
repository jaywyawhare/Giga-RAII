#include <stdio.h>
#include <string.h>
#include "raii.h"

int main(void) {
    managed_array(char, buf, 1024) {
        strcpy(buf, "Hello, Giga-RAII!");
        printf("%s\n", buf);
    }
    return 0;
}
