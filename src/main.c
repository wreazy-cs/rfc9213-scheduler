/**
 * @file main.c
 * @brief RFC 9213 Uyumlu API Gateway Oncelikli Istek Zamanlayicisi - Demo
 *
 * Bu program, RFC 9213 Extensible HTTP Priorities standardina uygun
 * Priority header'lari olan HTTP isteklerini Min-Heap tabanli
 * Priority Queue ile siraya alip islemektedir.
 *
 * Demo Senaryolari:
 *   1. Temel Islevsellik - cesitli onceliklerle istek ekleme/cikarma
 *   2. Stres Testi       - cok sayida rastgele istek
 *   3. RFC 9213 Parser   - cesitli header formatlari
 *   4. Heap Dogrulama    - heap ozelliginin kontrolu
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "priority_queue.h"
#include "rfc9213_parser.h"

/* Renk kodlari (terminal destekliyorsa) */
#define CLR_RESET  "\033[0m"
#define CLR_GREEN  "\033[32m"
#define CLR_YELLOW "\033[33m"
#define CLR_CYAN   "\033[36m"
#define CLR_RED    "\033[31m"
#define CLR_BOLD   "\033[1m"

/* Zaman damgasi yardimcisi (basit ms sayaci) */
static long long get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

/* ==================== Demo 1: Temel Islevsellik ==================== */

static void demo_temel_islevsellik(void) {
    printf(CLR_BOLD CLR_CYAN
           "\n========================================\n"
           "  DEMO 1: Temel Islevsellik\n"
           "========================================\n" CLR_RESET);

    PriorityQueue *pq = pq_create();
    if (!pq) { fprintf(stderr, "Kuyruk olusturulamadi!\n"); return; }

    printf("\n[+] Cesitli RFC 9213 oncelikleriyle istekler ekleniyor...\n\n");

    /* Farkli urgency seviyeleriyle istekler */
    struct {
        const char *id;
        const char *endpoint;
        const char *priority;
    } requests[] = {
        { "REQ-001", "/api/payments",       "u=0"        },  /* En yuksek: kritik odeme */
        { "REQ-002", "/api/feed",            "u=7, i"     },  /* En dusuk: arkaplan feed */
        { "REQ-003", "/api/auth",            "u=1"        },  /* Yuksek: kimlik dogrulama */
        { "REQ-004", "/api/notifications",   "u=5, i=?1"  },  /* Dusuk: bildirimler */
        { "REQ-005", "/api/search",          "u=3"        },  /* Orta: arama */
        { "REQ-006", "/api/user/profile",    "u=2, i=?0"  },  /* Orta-yuksek: profil */
        { "REQ-007", "/api/analytics",       "u=6"        },  /* Dusuk: analitik */
        { "REQ-008", "/api/health",          "u=0, i=?1"  },  /* Kritik ama incremental */
        { "REQ-009", "/api/static/assets",   ""           },  /* Default: u=3 */
        { "REQ-010", "/api/stream",          "u=4, i"     },  /* Orta-dusuk: stream */
    };

    int n = (int)(sizeof(requests) / sizeof(requests[0]));
    long long base_ts = get_timestamp_ms();

    for (int i = 0; i < n; i++) {
        ApiRequest req = make_request(
            requests[i].id,
            requests[i].endpoint,
            requests[i].priority,
            base_ts + i * 10
        );
        char req_str[256];
        request_to_string(&req, req_str, sizeof(req_str));
        printf("  Ekleniyor: %s\n", req_str);
        pq_insert(pq, &req);
    }

    printf(CLR_GREEN "\n[OK] %d istek kuyruğa eklendi.\n" CLR_RESET, pq_size(pq));

    /* Heap ozelligini dogrula */
    if (pq_verify_heap_property(pq)) {
        printf(CLR_GREEN "[OK] Min-Heap ozelligi gecerli.\n" CLR_RESET);
    } else {
        printf(CLR_RED "[HATA] Heap ozelligi ihlali!\n" CLR_RESET);
    }

    printf("\n[*] Mevcut heap durumu:\n");
    pq_print(pq);

    printf("\n[+] Istekler oncelik sirasina gore isleniyor...\n\n");
    printf("%-5s %-20s %-30s %-10s\n",
           "Sira", "RequestID", "Endpoint", "HeapKey");
    printf("%-5s %-20s %-30s %-10s\n",
           "----", "-------------------", "-----------------------------", "----------");

    int order = 1;
    ApiRequest extracted;
    while (pq_extract_min(pq, &extracted) == 0) {
        printf("%-5d %-20s %-30s %-10d\n",
               order++,
               extracted.request_id,
               extracted.endpoint,
               extracted.heap_key);
    }

    printf(CLR_GREEN "\n[OK] Tum istekler siraliyla islendi.\n" CLR_RESET);
    pq_destroy(pq);
}

/* ==================== Demo 2: RFC 9213 Parser Testleri ==================== */

static void demo_rfc9213_parser(void) {
    printf(CLR_BOLD CLR_CYAN
           "\n========================================\n"
           "  DEMO 2: RFC 9213 Parser Testleri\n"
           "========================================\n" CLR_RESET);

    struct {
        const char *header;
        const char *aciklama;
    } test_cases[] = {
        { "u=0",        "Maksimum oncelik, non-incremental"        },
        { "u=7",        "Minimum oncelik, non-incremental"         },
        { "u=3",        "Varsayilan urgency, non-incremental"      },
        { "u=1, i",     "Yuksek oncelik, incremental (bare flag)"  },
        { "u=3, i=?1",  "Orta oncelik, incremental=true"          },
        { "u=5, i=?0",  "Dusuk oncelik, incremental=false"        },
        { "",           "Bos header -> RFC 9213 varsayilanlari"    },
        { "u=2, i, ext=foo", "Bilinmeyen token ignore edilmeli"    },
        { "u=0, i=?1",  "Kritik + incremental"                    },
    };

    int n = (int)(sizeof(test_cases) / sizeof(test_cases[0]));

    printf("\n%-25s %-8s %-12s %-10s %s\n",
           "Header", "Urgency", "Incremental", "HeapKey", "Aciklama");
    printf("%-25s %-8s %-12s %-10s %s\n",
           "-------------------------", "-------", "-----------",
           "---------", "-----------------------------------");

    for (int i = 0; i < n; i++) {
        RFC9213Priority p = rfc9213_parse(test_cases[i].header);
        int key = rfc9213_to_heap_key(p);
        printf("%-25s %-8d %-12s %-10d %s\n",
               test_cases[i].header[0] ? test_cases[i].header : "(bos)",
               p.urgency,
               p.incremental ? "true" : "false",
               key,
               test_cases[i].aciklama);
    }
}

/* ==================== Demo 3: Stres Testi ==================== */

static void demo_stres_testi(void) {
    printf(CLR_BOLD CLR_CYAN
           "\n========================================\n"
           "  DEMO 3: Stres Testi (1000 istek)\n"
           "========================================\n" CLR_RESET);

    PriorityQueue *pq = pq_create();
    if (!pq) return;

    int n = 1000;
    srand(42); /* Tekrar edilebilir rastgelelik */

    long long base_ts = get_timestamp_ms();

    printf("[+] %d rastgele istek ekleniyor...\n", n);

    for (int i = 0; i < n; i++) {
        char id[MAX_REQUEST_ID_LEN];
        char endpoint[MAX_ENDPOINT_LEN];
        char priority_header[32];

        int urgency     = rand() % 8;          /* 0-7 */
        int incremental = rand() % 2;          /* 0 veya 1 */

        snprintf(id,       sizeof(id),       "REQ-%04d", i + 1);
        snprintf(endpoint, sizeof(endpoint), "/api/resource/%d", rand() % 50);
        snprintf(priority_header, sizeof(priority_header),
                 incremental ? "u=%d, i" : "u=%d", urgency);

        ApiRequest req = make_request(id, endpoint, priority_header, base_ts + i);
        if (pq_insert(pq, &req) != 0) {
            fprintf(stderr, "[HATA] Insert basarisiz: %s\n", id);
        }
    }

    printf("[OK] %d istek eklendi. Kapasite: %d\n", pq_size(pq), pq->capacity);

    /* Heap dogrula */
    if (pq_verify_heap_property(pq)) {
        printf(CLR_GREEN "[OK] %d eleman sonrasi heap ozelligi gecerli.\n" CLR_RESET, n);
    } else {
        printf(CLR_RED "[HATA] Heap ozelligi ihlali!\n" CLR_RESET);
    }

    /* Ilk 5 en yuksek oncelikli istegi goster */
    printf("\n[*] Ilk 5 en yuksek oncelikli istek:\n");
    printf("%-5s %-15s %-30s %-10s\n",
           "Sira", "RequestID", "Endpoint", "HeapKey");

    int shown = 0;
    ApiRequest extracted;
    while (shown < 5 && pq_extract_min(pq, &extracted) == 0) {
        printf("%-5d %-15s %-30s %-10d\n",
               shown + 1,
               extracted.request_id,
               extracted.endpoint,
               extracted.heap_key);
        shown++;
    }

    /* Kalanini temizle */
    int remaining = pq_size(pq);
    while (pq_extract_min(pq, &extracted) == 0) {}

    printf("[OK] Toplam %d istek islendi.\n", shown + remaining);
    pq_destroy(pq);
}

/* ==================== Demo 4: Dinamik Kapasite ==================== */

static void demo_dinamik_kapasite(void) {
    printf(CLR_BOLD CLR_CYAN
           "\n========================================\n"
           "  DEMO 4: Dinamik Bellek Yonetimi\n"
           "========================================\n" CLR_RESET);

    PriorityQueue *pq = pq_create();
    if (!pq) return;

    printf("[*] Baslangic kapasitesi: %d\n", pq->capacity);
    printf("[+] Kapasite asimlarini tetiklemek icin istek ekleniyor...\n\n");

    int prev_cap = pq->capacity;
    long long ts = get_timestamp_ms();

    for (int i = 0; i < 100; i++) {
        char id[32], endpoint[64];
        snprintf(id, sizeof(id), "CAP-%03d", i);
        snprintf(endpoint, sizeof(endpoint), "/api/test/%d", i);

        ApiRequest req = make_request(id, endpoint, "u=3", ts + i);
        pq_insert(pq, &req);

        if (pq->capacity != prev_cap) {
            printf("  [realloc] Boyut %d -> %d (size=%d)\n",
                   prev_cap, pq->capacity, pq->size);
            prev_cap = pq->capacity;
        }
    }

    printf("\n[OK] %d eleman, %d kapasite. Heap gecerli: %s\n",
           pq->size, pq->capacity,
           pq_verify_heap_property(pq) ? CLR_GREEN "EVET" CLR_RESET
                                       : CLR_RED "HAYIR" CLR_RESET);

    pq_destroy(pq);
    printf(CLR_GREEN "[OK] Bellek tam olarak serbest birakildi.\n" CLR_RESET);
}

/* ==================== MAIN ==================== */

int main(void) {
    printf(CLR_BOLD
           "\n╔══════════════════════════════════════════════════════╗\n"
           "║  RFC 9213 Uyumlu API Gateway Oncelikli Zamanlayici  ║\n"
           "║  Min-Heap Tabanli Priority Queue Implementasyonu    ║\n"
           "╚══════════════════════════════════════════════════════╝\n"
           CLR_RESET);

    demo_temel_islevsellik();
    demo_rfc9213_parser();
    demo_stres_testi();
    demo_dinamik_kapasite();

    printf(CLR_BOLD CLR_GREEN
           "\n[TAMAMLANDI] Tum demolar basarıyla calıstı.\n"
           CLR_RESET);

    return 0;
}
