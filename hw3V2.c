/*
 * 과제 3: 즉석라면 자판기 (Ramen Kiosk Simulator)
 *
 * 이 파일은 과제의 시작 템플릿입니다.
 * TODO로 표시된 부분을 구현하세요.
 *
 * 컴파일:
 * gcc -Wall -Wextra -std=c11 -o kiosk ramen_kiosk.c -lm
 *
 * 실행:
 * ./kiosk
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   /* usleep */
#include <time.h>

/* ============================================================ *
 * 상수
 * ============================================================ */

#define MENU_COUNT      5
#define INITIAL_STOCK   5

/* 화폐 단위 (큰 단위부터, 거스름돈 계산용) */
#define DENOM_COUNT     4
static const int DENOMINATIONS[DENOM_COUNT] = {5000, 1000, 500, 100};

/* 시간 시뮬레이션 파라미터 */
#define WATER_FILL_RATE_ML_PER_SEC  100   /* 100ml / 초 */
#define COOK_MINUTE_TO_SEC           1    /* 조리 시간 1분 -> 1초 압축 */
#define PROGRESS_BAR_WIDTH          24    /* 진행률 바의 너비 */

/* ============================================================ *
 * 상태 머신 정의
 * ============================================================ */

typedef enum {
    STATE_IDLE = 0,
    STATE_MENU,
    STATE_PAYMENT,
    STATE_CUP_INSERT,
    STATE_WATER_FILL,
    STATE_COOKING,
    STATE_DONE,
    STATE_EXIT,         /* 프로그램 종료를 위한 특수 상태 */
    STATE_COUNT
} State;

/* ============================================================ *
 * 데이터 구조
 * ============================================================ */

typedef struct {
    int  id;
    char name[32];
    int  price;        /* 원 */
    int  water_ml;     /* 필요한 물량 (ml) */
    int  cook_min;     /* 조리 시간 (분) */
    int  stock;        /* 남은 재고 */
} Ramen;

typedef struct {
    State  state;          /* 현재 상태 */
    Ramen *menu;           /* 메뉴 배열 */
    int    menu_count;     /* 메뉴 개수 */
    int    selected;       /* 선택된 메뉴 인덱스 (-1: 미선택) */
    int    inserted;       /* 투입된 금액 (원) */
    int    sales_total;    /* 누적 매출 (종료 시 출력용) */
    int    sales_count;    /* 누적 판매 수량 (종료 시 출력용) */
} Kiosk;

/* ============================================================ *
 * 메뉴 데이터 (전역)
 * ============================================================ */

static Ramen g_menu[MENU_COUNT] = {
    {1, "신라면",   1500, 550, 3, INITIAL_STOCK},
    {2, "진라면",   1500, 500, 3, INITIAL_STOCK},
    {3, "너구리",   1700, 600, 5, INITIAL_STOCK},
    {4, "짜파게티", 1700, 550, 5, INITIAL_STOCK},
    {5, "안성탕면", 1500, 550, 4, INITIAL_STOCK},
};

/* ============================================================ *
 * 유틸리티 함수 (이미 구현됨, 그대로 사용해도 됨)
 * ============================================================ */

static void clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

static void wait_enter(const char *msg) {
    int c;
    printf("%s", msg);
    fflush(stdout);
    while ((c = getchar()) != '\n' && c != EOF) { /* drain */ }
}

static int read_int(const char *prompt, int min, int max) {
    int value;
    char buf[64];
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) {
            clearerr(stdin);
            continue;
        }
        if (sscanf(buf, "%d", &value) == 1 && value >= min && value <= max) {
            return value;
        }
        printf("  [!] 잘못된 입력입니다. %d ~ %d 범위의 숫자를 입력하세요.\n", min, max);
    }
}

static void print_header(const char *title) {
    printf("==============================================\n");
    printf("  %s\n", title);
    printf("==============================================\n");
}

/* ============================================================ *
 * 보조 함수 (학생 구현완료)
 * ============================================================ */

/* 거스름돈을 단위별 개수로 계산 (Greedy) */
static void calculate_change(int amount, int counts[DENOM_COUNT]) {
    for (int i = 0; i < DENOM_COUNT; i++) {
        counts[i] = amount / DENOMINATIONS[i];
        amount %= DENOMINATIONS[i];
    }
}

/* 진행률 막대를 한 줄에 출력 (\r로 캐리지 리턴) */
static void print_progress(const char *label, double current, double total) {
    int percentage = (int)((current / total) * 100);
    int fill_width = (int)((current / total) * PROGRESS_BAR_WIDTH);
    if (fill_width > PROGRESS_BAR_WIDTH) fill_width = PROGRESS_BAR_WIDTH;

    printf("\r  %s [", label);
    for (int i = 0; i < PROGRESS_BAR_WIDTH; i++) {
        if (i < fill_width) printf("#");
        else printf("-");
    }
    printf("] %d%% (%.1f/%.1f초)", percentage, current, total);
    fflush(stdout);
}

/* ============================================================ *
 * 상태 핸들러 함수 (학생 구현완료)
 * ============================================================ */

/* IDLE: 대기 화면 및 전소량 체크 */
static State handle_idle(Kiosk *k) {
    int all_sold_out = 1;
    for (int i = 0; i < k->menu_count; i++) {
        if (k->menu[i].stock > 0) {
            all_sold_out = 0;
            break;
        }
    }
    if (all_sold_out) {
        printf("\n  [안내] 모든 메뉴가 품절되었습니다. 시스템을 종료합니다.\n");
        wait_enter("  Press Enter to exit...");
        return STATE_EXIT;
    }

    clear_screen();
    print_header("WELCOME TO RAMEN KIOSK");
    printf("  [1] 메뉴 선택 및 자판기 시작\n");
    printf("  [0] 프로그램 종료\n\n");
    
    int input = read_int("  선택번호를 입력하세요 > ", 0, 1);
    if (input == 1) {
        k->selected = -1;
        k->inserted = 0;
        return STATE_MENU;
    }
    return STATE_EXIT;
}

/* MENU: 메뉴 목록 출력 및 유효성 선택 */
static State handle_menu(Kiosk *k) {
    clear_screen();
    print_header("MENU SELECTION");
    for (int i = 0; i < k->menu_count; i++) {
        printf("  [%d] %-12s | 가격: %d원 | ", k->menu[i].id, k->menu[i].name, k->menu[i].price);
        if (k->menu[i].stock > 0) printf("재고: %d개\n", k->menu[i].stock);
        else printf("[품절]\n");
    }
    printf("  [0] 처음으로 돌아가기 (선택 취소)\n\n");

    int choice = read_int("  메뉴 번호를 선택하세요 > ", 0, k->menu_count);
    if (choice == 0) return STATE_IDLE;

    int idx = choice - 1;
    if (k->menu[idx].stock <= 0) {
        printf("  [!] 선택하신 라면은 품절되었습니다.\n");
        wait_enter("  Press Enter to retry...");
        return STATE_MENU;
    }
    k->selected = idx;
    return STATE_PAYMENT;
}

/* PAYMENT: 분할 금액 누적 투입 및 최적 환불 처리 */
static State handle_payment(Kiosk *k) {
    clear_screen();
    print_header("PAYMENT");
    Ramen sel = k->menu[k->selected];
    printf("  선택 상품: %s (%d원)\n", sel.name, sel.price);
    printf("  투입 금액: %d원\n", k->inserted);
    printf("  남은 금액: %d원\n\n", (sel.price - k->inserted > 0) ? (sel.price - k->inserted) : 0);
    printf("  [1] 100원  [2] 500원  [3] 1000원  [4] 5000원  [0] 결제취소 및 환불\n\n");

    int choice = read_int("  화폐 종류를 선택하세요 > ", 0, 4);
    if (choice == 0) {
        printf("\n  [취소] 결제가 취소되었습니다. 환불 금액: %d원\n", k->inserted);
        if (k->inserted > 0) {
            int counts[DENOM_COUNT] = {0};
            calculate_change(k->inserted, counts);
            for (int i = 0; i < DENOM_COUNT; i++) {
                if (counts[i] > 0) printf("    - %d원 단위: %d개\n", DENOMINATIONS[i], counts[i]);
            }
        }
        wait_enter("\n  처음 화면으로 돌아갑니다. (Enter)...");
        return STATE_IDLE;
    }

    int money_map[] = {0, 100, 500, 1000, 5000};
    k->inserted += money_map[choice];

    if (k->inserted >= sel.price) {
        int change = k->inserted - sel.price;
        printf("\n  [완료] 결제가 성공적으로 이루어졌습니다.\n");
        if (change > 0) {
            printf("   거스름돈 %d원이 반환됩니다:\n", change);
            int counts[DENOM_COUNT] = {0};
            calculate_change(change, counts);
            for (int i = 0; i < DENOM_COUNT; i++) {
                if (counts[i] > 0) printf("    - %d원 단위: %d개\n", DENOMINATIONS[i], counts[i]);
            }
        }
        wait_enter("\n  컵 투입 단계로 이동합니다. (Enter)...");
        return STATE_CUP_INSERT;
    }
    return STATE_PAYMENT;
}

/* CUP_INSERT: 동의 버퍼 엔터 확인 대기 */
static State handle_cup_insert(Kiosk *k) {
    (void)k;
    clear_screen();
    print_header("INSERT CUP");
    printf("\n  전용 용기(컵)를 기기 중앙 조리대에 안착해 주세요.\n\n");
    wait_enter("  [!] 컵을 안착시켰다면 엔터(Enter)키를 눌러주세요 > ");
    return STATE_WATER_FILL;
}

/* WATER_FILL: 실시간 100ml/s 속도 시뮬레이션 */
static State handle_water_fill(Kiosk *k) {
    clear_screen();
    print_header("WATER FILLING");
    double total_sec = (double)k->menu[k->selected].water_ml / WATER_FILL_RATE_ML_PER_SEC;
    double current_sec = 0.0;

    while (current_sec < total_sec) {
        print_progress("[급수중]", current_sec, total_sec);
        usleep(100000);
        current_sec += 0.1;
    }
    print_progress("[급수중]", total_sec, total_sec);
    printf("\n\n  [완료] 정량 급수가 성공적으로 완료되었습니다.\n");
    wait_enter("  조리 단계로 가기 위해 엔터를 누르세요...");
    return STATE_COOKING;
}

/* COOKING: 1분->1초 축소 가열 및 최종 영구 데이터 적층 */
static State handle_cooking(Kiosk *k) {
    clear_screen();
    print_header("COOKING IN PROGRESS");
    double total_sec = (double)k->menu[k->selected].cook_min;
    double current_sec = 0.0;

    while (current_sec < total_sec) {
        print_progress("[조리중]", current_sec, total_sec);
        usleep(100000);
        current_sec += 0.1;
    }
    print_progress("[조리중]", total_sec, total_sec);
    
    // 비가역 조리 정상수렴 완료 시점에만 최종 재고 및 경영 정보 차감/적층
    k->menu[k->selected].stock--;
    k->sales_total += k->menu[k->selected].price;
    k->sales_count++;

    printf("\n\n  [완료] 라면 조리가 성공적으로 마무리되었습니다.\n");
    wait_enter("  수거 단계로 이동하기 위해 엔터를 누르세요...");
    return STATE_DONE;
}

/* DONE: 수거 안내 및 구조체 내부 세션 정보 소거화(Reset) */
static State handle_done(Kiosk *k) {
    clear_screen();
    print_header("TAKE YOUR RAMEN");
    printf("\n  주문하신 [%s] 조리가 완료되었습니다.\n", k->menu[k->selected].name);
    printf("  뜨거우니 용기 상단 가이드 라인을 잡아 안전하게 수거하세요.\n\n");
    wait_enter("  [확인] 수거를 완료했다면 엔터(Enter)를 눌러 세션을 종료하세요 > ");
    
    // 다음 고객 세션을 위한 데이터 인스턴스 초기 자원화
    k->selected = -1;
    k->inserted = 0;
    return STATE_IDLE;
}

/* ============================================================ *
 * 메인 루프 (템플릿 원본과 100% 동일, 변경 없음)
 * ============================================================ */

typedef State (*StateHandler)(Kiosk *);

int main(void) {
    Kiosk kiosk = {
        .state = STATE_IDLE,
        .menu = g_menu,
        .menu_count = MENU_COUNT,
        .selected = -1,
        .inserted = 0,
        .sales_total = 0,
        .sales_count = 0,
    };

    StateHandler handlers[STATE_COUNT] = {
        [STATE_IDLE]        = handle_idle,
        [STATE_MENU]        = handle_menu,
        [STATE_PAYMENT]     = handle_payment,
        [STATE_CUP_INSERT]  = handle_cup_insert,
        [STATE_WATER_FILL]  = handle_water_fill,
        [STATE_COOKING]     = handle_cooking,
        [STATE_DONE]        = handle_done,
        [STATE_EXIT]        = NULL,
    };

    while (kiosk.state != STATE_EXIT) {
        StateHandler h = handlers[kiosk.state];
        if (h == NULL) {
            fprintf(stderr, "FATAL: no handler for state %d\n", kiosk.state);
            return 1;
        }
        kiosk.state = h(&kiosk);
    }

    clear_screen();
    print_header("이용해 주셔서 감사합니다");
    printf("  누적 매출: %d원\n", kiosk.sales_total);
    printf("  판매 수량: %d개\n", kiosk.sales_count);
    printf("==============================================\n");

    return 0;
}