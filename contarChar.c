#include <stdio.h>
#include <string.h>

void main()
{
    char s[1000];
    int count=0;
     while (fgets(s, 1024, stdin))
        count += strlen(s);
    printf("%d\n",count);
}