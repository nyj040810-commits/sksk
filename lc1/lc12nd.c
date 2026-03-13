#include<stdio.h>

int main()
{
 int num1, num2;
 printf("첫 번째 정수 : ");
 scanf("%d", &num1);
 printf("두 번째 정수 : ");
 scanf("%d", &num2);
 printf("입력한 정수는 각각 [%d]와 [%d]입니다.\n"
,
num1, num2);
 return 0; 
}