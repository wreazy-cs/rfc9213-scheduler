/**
 * @file priority_queue.h
 * @brief Min-Heap tabanli Oncelikli Kuyruk (Priority Queue) - API Gateway Request Scheduler
 *
 * Bu modul, RFC 9213 ile belirlenen oncelik degerlerine gore
 * API Gateway isteklerini siraya alir ve Min-Heap algoritmasi ile
 * O(log n) insert/extract islemleri gerceklestirir.
 *
 * Min-Heap ozelligi:
 *   - Kok dugum daima en kucuk heap_key degerine sahiptir
 *   - Insert: O(log n) - bubble up
 *   - Extract-Min: O(log n) - bubble down (heapify)
 *   - Peek: O(1)
 *   - Bellek: O(n) - dinamik dizi
 */

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <stddef.h>
#include "rfc9213_parser.h"

/* Maksimum istek ID uzunlugu */
#define MAX_REQUEST_ID_LEN  64
/* Maksimum endpoint uzunlugu */
#define MAX_ENDPOINT_LEN   128
/* Baslangic kapasitesi (dinamik buyur) */
#define PQ_INITIAL_CAPACITY 16

/**
 * @brief API Gateway isteği
 */
typedef struct {
    char            request_id[MAX_REQUEST_ID_LEN];  /* Benzersiz istek ID */
    char            endpoint[MAX_ENDPOINT_LEN];       /* Hedef endpoint */
    RFC9213Priority priority;                         /* RFC 9213 onceligi */
    int             heap_key;                         /* Min-Heap siralama anahtari */
    long long       timestamp_ms;                     /* Eklenme zamani (ms) */
    int             retry_count;                      /* Yeniden deneme sayaci */
} ApiRequest;

/**
 * @brief Min-Heap tabanli Oncelikli Kuyruk yapisi
 *
 * Dinamik buyuyebilen dizi ile heap saklanir.
 * Bellek: malloc/realloc/free ile yonetilir.
 */
typedef struct {
    ApiRequest *data;    /* Heap dizisi (1-indexli degil, 0-indexli) */
    int         size;    /* Mevcut eleman sayisi */
    int         capacity;/* Ayrilan kapasite */
} PriorityQueue;

/* ==================== Yasam Dongusu ==================== */

/**
 * @brief Yeni bir PriorityQueue olusturur ve hafizayi ayirir
 * @return PriorityQueue* Basarida pointer, basarisizlikta NULL
 */
PriorityQueue *pq_create(void);

/**
 * @brief PriorityQueue'yu yok eder ve tum hafizayi serbest birakir
 * @param pq Silinecek kuyruk
 */
void pq_destroy(PriorityQueue *pq);

/* ==================== Temel Islemler ==================== */

/**
 * @brief Kuyruğa yeni bir istek ekler (O(log n))
 *
 * Eğer kapasite doluysa otomatik olarak 2x buyutur (realloc).
 *
 * @param pq Hedef kuyruk
 * @param request Eklenecek istek
 * @return int 0=basarili, -1=hata
 */
int pq_insert(PriorityQueue *pq, const ApiRequest *request);

/**
 * @brief En yuksek oncelikli istegi kuyruktan cikarir (O(log n))
 *
 * Min-Heap'ten kok cikarilir, son eleman koke alinir,
 * ardindan heapify-down uygulanir.
 *
 * @param pq Kaynak kuyruk
 * @param out Cikarilan istegin yazilacagi adres
 * @return int 0=basarili, -1=bos kuyruk veya hata
 */
int pq_extract_min(PriorityQueue *pq, ApiRequest *out);

/**
 * @brief En yuksek oncelikli istegi cikarMAdan goriintuler (O(1))
 *
 * @param pq Kuyruk
 * @return const ApiRequest* Kok elemani pointer, bos ise NULL
 */
const ApiRequest *pq_peek(const PriorityQueue *pq);

/* ==================== Yardimci Islemler ==================== */

/**
 * @brief Kuyrugun bos olup olmadigini kontrol eder
 * @param pq Kuyruk
 * @return int 1=bos, 0=dolu
 */
int pq_is_empty(const PriorityQueue *pq);

/**
 * @brief Kuyrukta kac istek oldugunu dondurur
 * @param pq Kuyruk
 * @return int Eleman sayisi
 */
int pq_size(const PriorityQueue *pq);

/**
 * @brief Min-Heap ozelligini dogrular (test amacli)
 *
 * Her ebeveyn degerinin cocuklarindan <= oldugunu kontrol eder.
 *
 * @param pq Kuyruk
 * @return int 1=gecerli heap, 0=ihlal var
 */
int pq_verify_heap_property(const PriorityQueue *pq);

/**
 * @brief Kuyrugun tum icerigini stdout'a yazdirir
 * @param pq Kuyruk
 */
void pq_print(const PriorityQueue *pq);

/* ==================== ApiRequest Yardimcilari ==================== */

/**
 * @brief Yeni bir ApiRequest olusturur
 *
 * @param request_id Benzersiz istek kimlik kodu
 * @param endpoint   Hedef URL/endpoint
 * @param priority_header RFC 9213 Priority header degeri
 * @param timestamp_ms Zaman damgasi (milisaniye)
 * @return ApiRequest Doldurulmus istek yapisi
 */
ApiRequest make_request(const char *request_id,
                        const char *endpoint,
                        const char *priority_header,
                        long long   timestamp_ms);

/**
 * @brief ApiRequest icin okunabilir string olusturur
 * @param req ApiRequest
 * @param buf Yazilacak buffer
 * @param buf_size Buffer boyutu
 */
void request_to_string(const ApiRequest *req, char *buf, int buf_size);

#endif /* PRIORITY_QUEUE_H */
