#include <stdio.h>
#include <stdlib.h>
#include <alloc.h>
#include <string.h>

int main() {

    // Example usage of alloc(), dealloc(), and allocopt() and allocinfo()

    allocopt(FIRST_FIT, INCREMENT);

    alloc(1);
    struct allocinfo currStats = allocinfo();
    printf("Free size: %d\n\n", currStats.free_size);

    char* str1 = (char*)alloc((int)((strlen("Hello, world!")+1)*sizeof(char)));
    strcpy(str1, "Hello, world!");
    printf("%s\n", str1);
    currStats = allocinfo();
    printf("Free size: %d\n\n", currStats.free_size);

    char* str2 = (char*)alloc((int)((strlen("Hello again!")+1)*sizeof(char)));
    strcpy(str2, "Hello again!");
    printf("%s\n", str2);
    currStats = allocinfo();
    printf("Free size: %d\n\n", currStats.free_size);

    char* str3 = (char*)alloc((int)((strlen("Hello yet again!")+1)*sizeof(char)));
    strcpy(str3, "Hello yet again!");
    printf("%s\n", str3);
    currStats = allocinfo();
    printf("Free size: %d\n\n", currStats.free_size);
    
    printf("dealloc(str1)\n");
    dealloc(str1);
    currStats = allocinfo();
    printf("Free size: %d\n\n", currStats.free_size);


    printf("dealloc() str2 and str3\n");
    dealloc(str2);
    dealloc(str3);
    currStats = allocinfo();
    printf("Free size: %d\n", currStats.free_size);

    return 0;

}
