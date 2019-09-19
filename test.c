#include<stdio.h>
#include<string.h>
#include "structure.h"
char dir[15];
void cp(char *nm)
{
    strcpy(dir,nm);
}

void main()
{
    // scanf(" %[^\n]s",&dir);
    // printf("%s",dir);
// 
    // char a[15];
    // scanf(" %[^\n]s",&a);
    // cp(a);
// 
    // printf("%s",dir);

    printf("%d",sizeof(uint_t));
}