#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// 숫자를 가위, 바위, 보 문자열로 변환하는 함수 (모듈화 시도)
const char* getRPSName(int choice) {
    if (choice == 1) return "가위";
    else if (choice == 2) return "바위";
    else if (choice == 3) return "보";
    return "알수없음";
}

// NPC들 간의 자동 대전을 처리하는 함수 (새로운 시도)
// 반환값: 1번 플레이어 승리 시 1, 2번 플레이어 승리 시 2
int npcMatch(const char* p1, const char* p2) {
    int c1, c2;
    while(1) { // 승부가 날 때까지 반복 (재경기 구현)
        c1 = rand() % 3 + 1;
        c2 = rand() % 3 + 1;
        
        if (c1 == c2) continue; // 비기면 다시
        
        // 가위바위보 승패 로직
        if ((c1 == 1 && c2 == 3) || (c1 == 2 && c2 == 1) || (c1 == 3 && c2 == 2)) {
            return 1; // p1 승리
        } else {
            return 2; // p2 승리
        }
    }
}

// 플레이어와 상대방의 대전을 처리하는 함수
// 승리 시 1, 패배 시 0 반환
int playerMatch(const char* oppName, const char* roundName) {
    int userChoice, oppChoice;
    
    printf("\n%s\n", roundName);
    printf("상대 : %s\n", oppName);
    
    while(1) {
        printf("1: 가위\n2: 바위\n3: 보\n");
        printf("선택 > ");
        
        // 입력 예외 처리: 문자를 입력하거나 범위를 벗어난 숫자 입력 시 방어
        if (scanf("%d", &userChoice) != 1) {
            while(getchar() != '\n'); // 입력 버퍼 비우기 (새롭게 알게 된 개념)
            printf("1,2,3 중에서 입력하세요 >\n");
            continue;
        }
        
        if (userChoice < 1 || userChoice > 3) {
            printf("1,2,3 중에서 입력하세요 >\n");
            continue;
        }
        
        oppChoice = rand() % 3 + 1; // 1~3 난수 생성
        
        printf("나: %s\n", getRPSName(userChoice));
        printf("%s: %s\n", oppName, getRPSName(oppChoice));
        
        if (userChoice == oppChoice) {
            printf("=> 비겼습니다! 재경기!\n\n");
        } else if ((userChoice == 1 && oppChoice == 3) || 
                   (userChoice == 2 && oppChoice == 1) || 
                   (userChoice == 3 && oppChoice == 2)) {
            printf("=> 이겼습니다!\n");
            return 1;
        } else {
            printf("=> 졌습니다...\n");
            return 0;
        }
    }
}

int main() {
    srand((unsigned int)time(NULL)); // 매번 다른 랜덤값을 위한 시드 설정
    
    // 8명의 참가자 배열 선언
    const char* players[8] = {"플레이어", "철수", "영희", "민준", "지아", "현우", "수빈", "태양"};
    const char* semiFinalists[4];
    const char* finalists[2];
    
    printf("가위바위보 토너먼트\n");
    printf("비길 경우 재경기\n");
    printf("[대진표 ]\n");
    printf("8강\n");
    printf("[1] %s VS [2] %s\n", players[0], players[1]);
    printf("[3] %s VS [4] %s\n", players[2], players[3]);
    printf("[5] %s VS [6] %s\n", players[4], players[5]);
    printf("[7] %s VS [8] %s\n\n", players[6], players[7]);
    
    printf("당신은 [1] 플레이어입니다.\n");
    
    // --- 8강 (Quarterfinals) ---
    // 1경기: 플레이어 vs 철수
    if (playerMatch(players[1], "8강") == 0) {
        printf("아쉽습니다. 다음 기회에!\n");
        return 0;
    }
    printf("=> 4강 진출!\n");
    semiFinalists[0] = players[0]; // 플레이어 진출
    
    // 나머지 8강 자동 진행 (AI/NPC 경기)
    semiFinalists[1] = (npcMatch(players[2], players[3]) == 1) ? players[2] : players[3];
    semiFinalists[2] = (npcMatch(players[4], players[5]) == 1) ? players[4] : players[5];
    semiFinalists[3] = (npcMatch(players[6], players[7]) == 1) ? players[6] : players[7];
    
    // --- 4강 (Semifinals) ---
    // 플레이어 vs 8강 2경기 승자
    if (playerMatch(semiFinalists[1], "4강") == 0) {
        printf("아쉽습니다. 다음 기회에!\n");
        return 0;
    }
    printf("=> 결승 진출!\n");
    finalists[0] = semiFinalists[0]; // 플레이어 결승 진출
    
    // 나머지 4강 자동 진행
    finalists[1] = (npcMatch(semiFinalists[2], semiFinalists[3]) == 1) ? semiFinalists[2] : semiFinalists[3];
    
    // --- 결승 (Final) ---
    if (playerMatch(finalists[1], "결승") == 0) {
        printf("아쉽습니다. 다음 기회에!\n");
        return 0;
    }
    
    printf("축하합니다! 우승!\n");
    return 0;
}