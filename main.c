#include <stdio.h>                                                                                                                                                        
#include <string.h>
#include "raii.h"                                                                                                                                                         
                                                                                 
int main(void) {
    managed_malloc(buf, 1024) {
        strcpy(buf, "Hello, Giga-RAII!");
        printf("%s\n", (char *)buf);
    }
    return 0;
}