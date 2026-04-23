/**
 * @file priority_queue.c
 * @brief Min-Heap tabanli Oncelikli Kuyruk - Implementasyon
 *
 * ALGORITMA ACIKLAMASI:
 * =====================
 * Min-Heap, tam ikili agac (complete binary tree) ozelligi koruyan
 * bir veri yapisidir. Dizide su sekilde saklanir (0-indexli):
 *
 *   Ebeveyn(i)       = (i - 1) / 2
 *   Sol cocuk(i)     = 2 * i + 1
 *   Sag cocuk(i)     = 2 * i + 2
 *
 * Heap Ozelligi: data[ebeveyn].heap_key <= data[cocuk].heap_key
 *
 * INSERT (bubble-up / sift-up):
 *   1. Yeni eleman dizinin sonuna eklenir
 *   2. Ebeveyninden kucukse yer degistir
 *   3. Kok'e ulasana veya ebeveyninden buyuk/esit olana kadar tekrarla
 *   Karmasiklik: O(log n)
 *
 * EXTRACT-MIN (bubble-down / heapify-down / sift-down):
 *   1. Kok alinir (minimum eleman)
 *   2. Dizinin son elemani koke tasinir, boyut azaltilir
 *   3. Kok iki cocugundan kucuk olana kadar asagi indirilir
 *   Karmasiklik: O(log n)
 *
 * NEDEN DIZI, LINKED LIST DEGIL?
 *   - Dizi: cache-friendly, O(1) rastgele erisim, daha az pointer overhead
 *   - Linked list ile heap: pointer takibi, cache miss, ekstra bellek
 *
 * DINAMIK BELLEK YONETIMI:
 *   - Baslangic kapasitesi PQ_INITIAL_CAPACITY (16)
 *   - Dolunca realloc ile 2x buyutulur (amortized O(1) insert)
 *   - pq_destroy ile tam temizlik yapilir (sifir bellek sizintisi)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "priority_queue.h"
#include "rfc9213_parser.h"

/* ==================== Ic Yardimci Fonksiyonlar ==================== */

/**
 * @brief Iki ApiRequest'i yer degistirir
 */
static void swap(ApiRequest *a, ApiRequest *b) {
    ApiRequest tmp = *a;
    *a = *b;
    *b = tmp;
}

/**
 * @brief Ebeveyn indeksini hesaplar (0-indexli heap)
 */
static int parent(int i) {
    return (i - 1) / 2;
}

/**
 * @brief Sol cocuk indeksini hesaplar
 */
static int left_child(int i) {
    return 2 * i + 1;
}

/**
 * @brief Sag cocuk indeksini hesaplar
 */
static int right_child(int i) {
    return 2 * i + 2;
}

/**
 * @brief Bubble-Up (Sift-Up): Yeni eklenen elemani dogru konuma tasir
 *
 * INSERT islemi sonrasinda cagrilir.
 * Eleman, heap ozelligini saglayan konuma ulasana kadar yukarı tasınır.
 *
 * @param pq Kuyruk
 * @param i  Baslangiç indeksi (yeni eklenen eleman)
 */
static void bubble_up(PriorityQueue *pq, int i) {
    /*
     * Kok'e ulasmadikca ve ebeveyn bizden buyukse yukari cik.
     * Heap ozelligi: ebeveyn <= cocuk
     * Ihlal: ebeveyn > cocuk -> yer degistir
     */
    while (i > 0 &&
           pq->data[parent(i)].heap_key > pq->data[i].heap_key) {
        swap(&pq->data[parent(i)], &pq->data[i]);
        i = parent(i);
    }
}

/**
 * @brief Heapify-Down (Sift-Down): Kok'ten asagi heap ozelligini restore eder
 *
 * EXTRACT-MIN sonrasinda cagrilir.
 * En kucuk cocugu bulur, ebeveynden kucukse yer degistirir.
 *
 * @param pq Kuyruk
 * @param i  Baslangiç indeksi (genellikle 0 = kok)
 */
static void heapify_down(PriorityQueue *pq, int i) {
    int smallest = i;
    int left      = left_child(i);
    int right     = right_child(i);
    int n         = pq->size;

    /*
     * Sol cocuk varsa ve benden kucukse -> smallest = sol
     */
    if (left < n && pq->data[left].heap_key < pq->data[smallest].heap_key) {
        smallest = left;
    }

    /*
     * Sag cocuk varsa ve su ana kadar en kucukten de kucukse -> smallest = sag
     */
    if (right < n && pq->data[right].heap_key < pq->data[smallest].heap_key) {
        smallest = right;
    }

    /*
     * Eger ben zaten en kucuk degilsem: yer degistir ve recursif devam et
     */
    if (smallest != i) {
        swap(&pq->data[i], &pq->data[smallest]);
        heapify_down(pq, smallest);
    }
    /* smallest == i ise heap ozelligi saglanmis, dur */
}

/* ==================== Yasam Dongusu ==================== */

PriorityQueue *pq_create(void) {
    PriorityQueue *pq = (PriorityQueue *)malloc(sizeof(PriorityQueue));
    if (!pq) {
        fprintf(stderr, "[HATA] pq_create: PriorityQueue icin bellek ayrilamadi\n");
        return NULL;
    }

    pq->data = (ApiRequest *)malloc(PQ_INITIAL_CAPACITY * sizeof(ApiRequest));
    if (!pq->data) {
        fprintf(stderr, "[HATA] pq_create: data dizisi icin bellek ayrilamadi\n");
        free(pq);
        return NULL;
    }

    pq->size     = 0;
    pq->capacity = PQ_INITIAL_CAPACITY;

    return pq;
}

void pq_destroy(PriorityQueue *pq) {
    if (!pq) return;
    if (pq->data) {
        free(pq->data);
        pq->data = NULL;
    }
    free(pq);
}

/* ==================== Temel Islemler ==================== */

int pq_insert(PriorityQueue *pq, const ApiRequest *request) {
    if (!pq || !request) return -1;

    /* Kapasite kontrolu: doluysa 2x buyut */
    if (pq->size >= pq->capacity) {
        int new_capacity = pq->capacity * 2;
        ApiRequest *new_data = (ApiRequest *)realloc(
            pq->data,
            new_capacity * sizeof(ApiRequest)
        );
        if (!new_data) {
            fprintf(stderr, "[HATA] pq_insert: realloc basarisiz (capacity=%d)\n",
                    new_capacity);
            return -1;
        }
        pq->data     = new_data;
        pq->capacity = new_capacity;
    }

    /* 1. Yeni elemani dizinin sonuna ekle */
    pq->data[pq->size] = *request;
    pq->size++;

    /* 2. Heap ozelligini restore et: bubble-up */
    bubble_up(pq, pq->size - 1);

    return 0;
}

int pq_extract_min(PriorityQueue *pq, ApiRequest *out) {
    if (!pq || !out || pq->size == 0) {
        return -1;
    }

    /* 1. Minimum elemani (kok) al */
    *out = pq->data[0];

    /* 2. Son elemani koke tasI, boyutu azalt */
    pq->data[0] = pq->data[pq->size - 1];
    pq->size--;

    /* 3. Heap ozelligini restore et: heapify-down */
    if (pq->size > 0) {
        heapify_down(pq, 0);
    }

    return 0;
}

const ApiRequest *pq_peek(const PriorityQueue *pq) {
    if (!pq || pq->size == 0) return NULL;
    return &pq->data[0]; /* Kok = minimum = en yuksek oncelik */
}

/* ==================== Yardimci Islemler ==================== */

int pq_is_empty(const PriorityQueue *pq) {
    return (!pq || pq->size == 0) ? 1 : 0;
}

int pq_size(const PriorityQueue *pq) {
    return pq ? pq->size : 0;
}

int pq_verify_heap_property(const PriorityQueue *pq) {
    if (!pq || pq->size <= 1) return 1;

    for (int i = 1; i < pq->size; i++) {
        int p = parent(i);
        if (pq->data[p].heap_key > pq->data[i].heap_key) {
            fprintf(stderr,
                    "[HEAP IHLALI] data[%d].heap_key=%d > data[%d].heap_key=%d\n",
                    p, pq->data[p].heap_key,
                    i, pq->data[i].heap_key);
            return 0;
        }
    }
    return 1;
}

void pq_print(const PriorityQueue *pq) {
    if (!pq) {
        printf("[NULL KUYRUK]\n");
        return;
    }
    printf("PriorityQueue [size=%d, capacity=%d]\n", pq->size, pq->capacity);
    printf("%-4s %-20s %-30s %-10s %-14s\n",
           "Idx", "RequestID", "Endpoint", "HeapKey", "RFC9213(u,i)");
    printf("%-4s %-20s %-30s %-10s %-14s\n",
           "---", "-------------------", "-----------------------------",
           "----------", "--------------");

    for (int i = 0; i < pq->size; i++) {
        char prio_str[64];
        rfc9213_to_string(pq->data[i].priority, prio_str, sizeof(prio_str));
        printf("%-4d %-20s %-30s %-10d %s\n",
               i,
               pq->data[i].request_id,
               pq->data[i].endpoint,
               pq->data[i].heap_key,
               prio_str);
    }
    printf("\n");
}

/* ==================== ApiRequest Yardimcilari ==================== */

ApiRequest make_request(const char *request_id,
                        const char *endpoint,
                        const char *priority_header,
                        long long   timestamp_ms) {
    ApiRequest req;
    memset(&req, 0, sizeof(req));

    /* String kopyalama - buffer overflow koruması */
    strncpy(req.request_id, request_id ? request_id : "unknown",
            MAX_REQUEST_ID_LEN - 1);
    strncpy(req.endpoint, endpoint ? endpoint : "/",
            MAX_ENDPOINT_LEN - 1);

    /* RFC 9213 priority parse */
    req.priority = rfc9213_parse(priority_header);

    /* Heap key hesapla */
    req.heap_key = rfc9213_to_heap_key(req.priority);

    req.timestamp_ms = timestamp_ms;
    req.retry_count  = 0;

    return req;
}

void request_to_string(const ApiRequest *req, char *buf, int buf_size) {
    if (!req || !buf || buf_size <= 0) return;

    char prio_str[64];
    rfc9213_to_string(req->priority, prio_str, sizeof(prio_str));

    snprintf(buf, buf_size,
             "[%s] %s | %s | retry=%d",
             req->request_id,
             req->endpoint,
             prio_str,
             req->retry_count);
}
