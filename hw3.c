#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

// ==========================================
// 상수 및 구조체 정의
// ==========================================
#define MENU_COUNT 5

typedef enum {
    STATE_IDLE,
    STATE_MENU,
    STATE_PAYMENT,
    STATE_CUP_INSERT,
    STATE_WATER_FILL,
    STATE_COOKING,
    STATE_DONE,
    NUM_STATES
} State;

typedef struct {
    int id;
    char name[50];
    int price;
    int water_ml;
    int cook_time_sec; // 1분 = 1초로 압축 반영
    int stock;
} Ramen;

typedef struct {
    Ramen menu[MENU_COUNT];
    State current_state;
    int selected_idx;    // 선택된 메뉴의 인덱스 (-1은 없음)
    int inserted_money;   // 현재 투입된 금액
} Kiosk;

// ==========================================
// 유틸리티 및 보조 함수 선언
// ==========================================
void clear_screen();
void wait_enter();
int read_int(const char *prompt);
void print_header(const char *title);
void calculate_change(int change);
void print_progress(const char *prefix, int current, int total, const char *unit);
bool is_all_sold_out(Kiosk *kiosk);

// ==========================================
// 상태 핸들러 함수 선언 (R2, R3)
// ==========================================
State handle_idle(Kiosk *kiosk);
State handle_menu(Kiosk *kiosk);
State handle_payment(Kiosk *kiosk);
State handle_cup_insert(Kiosk *kiosk);
State handle_water_fill(Kiosk *kiosk);
State handle_cooking(Kiosk *kiosk);
State handle_done(Kiosk *kiosk);

// ==========================================
// 메인 루프 (R2: 함수 포인터 디스패치 적용)
// ==========================================
int main() {
    // 템플릿 기본 데이터 초기화 (재고 각 5개씩)
    Kiosk kiosk = {
        .menu = {
            {1, "신라면", 1500, 555, 3, 5},
            {2, "진라면", 1500, 500, 3, 5},
            {3, "너구리", 1700, 600, 5, 5},
            {4, "짜파게티", 1700, 550, 5, 5},
            {5, "안성탕면", 1500, 550, 4, 5}
        },
        .current_state = STATE_IDLE,
        .selected_idx = -1,
        .inserted_money = 0
    };

    // 함수 포인터 배열 정의 (R2)
    State (*handlers[NUM_STATES])(Kiosk *) = {
        [STATE_IDLE]       = handle_idle,
        [STATE_MENU]       = handle_menu,
        [STATE_PAYMENT]    = handle_payment,
        [STATE_CUP_INSERT] = handle_cup_insert,
        [STATE_WATER_FILL] = handle_water_fill,
        [STATE_COOKING]    = handle_cooking,
        [STATE_DONE]       = handle_done
    };

    // 메인 상태 머신 루프
    while (1) {
        // 전체 품절 검사 후 IDLE 상태에서 종료 처리 (R6)
        if (kiosk.current_state == STATE_IDLE && is_all_sold_out(&kiosk)) {
            clear_screen();
            print_header("SYSTEM TERMINATED");
            printf(" 모든 라면이 품절되었습니다.\n");
            printf(" 자판기 프로그램을 종료합니다.\n");
            break;
        }

        // 함수 포인터 디스패치를 통한 상태 전이 (R2)
        State next_state = handlers[kiosk.current_state](&kiosk);
        kiosk.current_state = next_state;
    }

    return 0;
}

// ==========================================
// 상태 핸들러 함수 구현
// ==========================================

// IDLE: 대기 화면 (R6 전체 품절 대응 포함)
State handle_idle(Kiosk *kiosk) {
    clear_screen();
    print_header("WELCOME TO RAMEN KIOSK");
    printf(" 안녕하세요! 즉석라면 자판기입니다.\n");
    printf(" 서비스를 시작하려면 '1'을 입력하세요.\n\n");
    
    int choice = read_int("> ");
    if (choice == 1) {
        kiosk->selected_idx = -1;
        kiosk->inserted_money = 0;
        return STATE_MENU;
    } else {
        printf(" [오류] 잘못된 입력입니다. 다시 시도하세요.\n");
        wait_enter();
        return STATE_IDLE;
    }
}

// MENU: 메뉴 선택 화면 (R6 재고 예외 처리)
State handle_menu(Kiosk *kiosk) {
    clear_screen();
    print_header("MENU SELECTION");
    
    for (int i = 0; i < MENU_COUNT; i++) {
        printf(" [%d] %-12s | 가격: %d원 | 물량: %dml | 조리시간: %d분 ", 
               kiosk->menu[i].id, kiosk->menu[i].name, kiosk->menu[i].price, 
               kiosk->menu[i].water_ml, kiosk->menu[i].cook_time_sec);
        if (kiosk->menu[i].stock > 0) {
            printf("[재고: %d개]\n", kiosk->menu[i].stock);
        } else {
            printf("[품절]\n");
        }
    }
    printf(" [0] 취소 (처음으로 돌아가기)\n\n");

    int choice = read_int("메뉴 번호를 선택하세요 > ");
    if (choice == 0) return STATE_IDLE;

    if (choice >= 1 && choice <= MENU_COUNT) {
        int idx = choice - 1;
        if (kiosk->menu[idx].stock <= 0) {
            printf(" [오류] 해당 상품은 품절되었습니다. 다른 메뉴를 골라주세요.\n");
            wait_enter();
            return STATE_MENU;
        }
        kiosk->selected_idx = idx;
        return STATE_PAYMENT;
    } else {
        printf(" [오류] 범위 밖의 번호입니다. 올바른 번호를 입력하세요.\n");
        wait_enter();
        return STATE_MENU;
    }
}

// PAYMENT: 결제 처리 (R4 분할 투입 및 환불)
State handle_payment(Kiosk *kiosk) {
    clear_screen();
    print_header("PAYMENT SYSTEM");
    Ramen selected = kiosk->menu[kiosk->selected_idx];
    
    printf(" 선택하신 메뉴: %s (%d원)\n", selected.name, selected.price);
    printf(" 현재 투입된 금액: %d원\n", kiosk->inserted_money);
    printf(" 남은 결제 금액: %d원\n\n", (selected.price - kiosk->inserted_money > 0) ? (selected.price - kiosk->inserted_money) : 0);
    
    printf(" [금액 투입 단위: 100, 500, 1000, 5000]\n");
    printf(" [0] 결제 취소 및 전액 환불\n\n");

    int money = read_int("금액을 투입하세요 > ");
    
    if (money == 0) {
        printf(" [취소] 결제가 취소되었습니다. 투입 금액 %d원을 환불합니다.\n", kiosk->inserted_money);
        if (kiosk->inserted_money > 0) {
            calculate_change(kiosk->inserted_money);
        }
        wait_enter();
        return STATE_IDLE;
    }

    if (money == 100 || money == 500 || money == 1000 || money == 5000) {
        kiosk->inserted_money += money;
        
        if (kiosk->inserted_money >= selected.price) {
            int change = kiosk->inserted_money - selected.price;
            printf(" [완료] 결제가 정상 처리되었습니다.\n");
            if (change > 0) {
                printf(" 거스름돈 %d원이 반환됩니다.\n", change);
                calculate_change(change);
            }
            wait_enter();
            return STATE_CUP_INSERT;
        }
        return STATE_PAYMENT;
    } else {
        printf(" [오류] 투입 불가능한 화폐 단위입니다.\n");
        wait_enter();
        return STATE_PAYMENT;
    }
}

// CUP_INSERT: 전용 컵 투입 대기
State handle_cup_insert(Kiosk *kiosk) {
    clear_screen();
    print_header("INSERT CUP");
    printf(" 전용 컵을 자판기에 올바르게 안착시켜 주세요.\n");
    printf(" [1] 컵 투입 완료\n");
    printf(" [0] 취소 및 환불\n\n");

    int choice = read_int("> ");
    if (choice == 1) {
        return STATE_WATER_FILL;
    } else if (choice == 0) {
        // 이 단계에서 취소 시 메뉴 가격만큼을 다시 그대로 환불함
        int refund = kiosk->menu[kiosk->selected_idx].price;
        printf(" [취소] 작업을 취소합니다. 결제 금액 %d원을 환불합니다.\n", refund);
        calculate_change(refund);
        wait_enter();
        return STATE_IDLE;
    } else {
        printf(" [오류] 잘못된 입력입니다.\n");
        wait_enter();
        return STATE_CUP_INSERT;
    }
}

// WATER_FILL: 초당 100ml 속도로 급수 (R5 진행률 실시간 제어)
State handle_water_fill(Kiosk *kiosk) {
    clear_screen();
    print_header("WATER FILLING");
    
    int target_water = kiosk->menu[kiosk->selected_idx].water_ml;
    int current_water = 0;
    
    while (current_water < target_water) {
        current_water += 100;
        if (current_water > target_water) current_water = target_water;
        
        print_progress("조리법 물 주입 중", current_water, target_water, "ml");
        usleep(1000000); // 1초 대기
    }
    printf("\n\n [완료] 물 주입이 완료되었습니다!\n");
    wait_enter();
    return STATE_COOKING;
}

// COOKING: 압축된 시간 기반 조리 시뮬레이션 (R5 \r 활용 캐리지 리턴)
State handle_cooking(Kiosk *kiosk) {
    clear_screen();
    print_header("COOKING IN PROGRESS");
    
    int total_time = kiosk->menu[kiosk->selected_idx].cook_time_sec;
    
    // 0.5초 단위 구동 체계 구축으로 매끄러운 바 시뮬레이션
    for (int half_sec = 0; half_sec <= total_time * 2; half_sec++) {
        double current_sec = half_sec * 0.5;
        // 캐리지 리턴 매커니즘 정밀 제어
        printf("\r[조리중] ");
        int bar_count = (int)((current_sec / total_time) * 20);
        printf("[");
        for (int i = 0; i < 20; i++) {
            if (i < bar_count) printf("#");
            else printf("-");
        }
        printf("] %.0f%% (%.1f/%d.0초)", (current_sec / total_time) * 100, current_sec, total_time);
        fflush(stdout);
        
        if (half_sec < total_time * 2) {
            usleep(500000); // 0.5초 대기
        }
    }
    
    // 조리가 최종 정상 완료된 시점에 재고 1 차감 처리
    kiosk->menu[kiosk->selected_idx].stock--;
    printf("\n\n [완료] 라면 조리가 끝났습니다!\n");
    wait_enter();
    return STATE_DONE;
}

// DONE: 수거 대기 화면
State handle_done(Kiosk *kiosk) {
    clear_screen();
    print_header("TAKE YOUR RAMEN");
    printf(" 맛있는 %s이(가) 완성되었습니다.\n", kiosk->menu[kiosk->selected_idx].name);
    printf(" 화상 위험이 있으니 조심히 꺼내주세요.\n\n");
    printf(" [1] 라면 수거 완료\n\n");

    int choice = read_int("> ");
    if (choice == 1) {
        printf(" 이용해 주셔서 감사합니다. 맛있게 드세요!\n");
        wait_enter();
        return STATE_IDLE;
    } else {
        printf(" [오류] 라면을 안전하게 수거한 후 1번을 입력해 주세요.\n");
        wait_enter();
        return STATE_DONE;
    }
}

// ==========================================
// 보조 유틸리티 함수 구현부
// ==========================================

void clear_screen() {
    // 크로스 플랫폼 터미널 초기화 규칙 적용
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void wait_enter() {
    printf(" Press Enter to continue...");
    while (getchar() != '\n');
    getchar(); // 버퍼 비우기 및 엔터 대기
}

int read_int(const char *prompt) {
    int value;
    char buffer[100];
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            // 정수 치환 정밀 검사 수행 및 예외 필터링 (R6)
            if (sscanf(buffer, "%d", &value) == 1) {
                return value;
            }
        }
        printf(" [오류] 잘못된 문자 입력입니다. 숫자로 다시 입력해 주세요.\n");
    }
}

void print_header(const char *title) {
    // R7: 유니코드 박스 기호 대신 오직 순수 ASCII 체계 기반 타이틀 컴포넌트 출력
    printf("========================================\n");
    printf("   %s\n", title);
    printf("========================================\n");
}

void calculate_change(int change) {
    // R4: 화폐 단위별 잔돈 최소 개수 연산 로직 구현
    int units[] = {5000, 1000, 500, 100};
    int counts[4] = {0};

    for (int i = 0; i < 4; i++) {
        counts[i] = change / units[i];
        change %= units[i];
    }

    printf(" [잔돈 반환 상세 안내]\n");
    if (counts[0] > 0) printf("  - 5000원 지폐: %d장\n", counts[0]);
    if (counts[1] > 0) printf("  - 1000원 지폐: %d장\n", counts[1]);
    if (counts[2] > 0) printf("  - 500원 동전: %d개\n", counts[2]);
    if (counts[3] > 0) printf("  - 100원 동전: %d개\n", counts[3]);
    printf("----------------------------------------\n");
}

void print_progress(const char *prefix, int current, int total, const char *unit) {
    // R5: 물 주입 프로세스 전용 한 줄 진행률 컴포넌트
    printf("\r[%s] ", prefix);
    int bar_count = (int)(((double)current / total) * 20);
    printf("[");
    for (int i = 0; i < 20; i++) {
        if (i < bar_count) printf("#");
        else printf("-");
    }
    printf("] %d%% (%d/%d%s)", (int)(((double)current / total) * 100), current, total, unit);
    fflush(stdout);
}

bool is_all_sold_out(Kiosk *kiosk) {
    // R6: 전체 메뉴의 소진 여부를 연산하여 실시간 시스템 수명주기 통제
    for (int i = 0; i < MENU_COUNT; i++) {
        if (kiosk->menu[i].stock > 0) return false;
    }
    return true;
}