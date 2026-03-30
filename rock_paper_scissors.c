#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void) {
    const char *names[] = {"가위", "바위", "보"};
    int user;

    srand((unsigned)time(NULL));

    printf("가위(0) 바위(1) 보(2)를 선택하세요: ");
    if (scanf("%d", &user) != 1) {
        printf("잘못된 입력입니다. 프로그램을 종료합니다.\n");
        return 1;
    }

    if (user < 0 || user > 2) {
        printf("0, 1, 2 중 하나를 입력해야 합니다.\n");
        return 1;
    }

    int computer = rand() % 3;

    printf("플레이어: %s, 컴퓨터: %s\n", names[user], names[computer]);

    if (user == computer) {
        printf("결과: 무승부\n");
    } else if ((user == 0 && computer == 2) ||
               (user == 1 && computer == 0) ||
               (user == 2 && computer == 1)) {
        printf("결과: 플레이어 승\n");
    } else {
        printf("결과: 컴퓨터 승\n");
    }

    return 0;
}
