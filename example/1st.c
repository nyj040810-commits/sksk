#include <stdio.h>
#include <time.h>

int main() {
    int birth_year;
    time_t t = time(NULL);
    struct tm *now = localtime(&t);
    int current_year = now->tm_year + 1900;

    printf("출생년도를 입력하세요: ");
    if (scanf("%d", &birth_year) != 1) {
        printf("입력이 올바르지 않습니다. 숫자를 입력해주세요.\n");
        return 1;
    }

    int age = current_year - birth_year;
    if (age < 0) {
        printf("미래에서 오셨나요? 출생년도가 현재보다 큽니다.\n");
        return 1;
    }

    printf("현재 연도는 %d년 입니다.\n", current_year);
    printf("당신의 나이는 %d세입니다.\n", age);
    return 0;
}
