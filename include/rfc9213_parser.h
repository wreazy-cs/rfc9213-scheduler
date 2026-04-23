/**
 * @file rfc9213_parser.h
 * @brief RFC 9213 (Extensible HTTP Priorities) - Priority Header Parser
 *
 * RFC 9213 standardina gore HTTP Priority header degerlerini parse eder.
 * Priority: u=<urgency>, i
 *   - urgency (u): 0-7 arasi tamsayi (0 en yuksek oncelik)
 *   - incremental (i): boolean flag
 */

#ifndef RFC9213_PARSER_H
#define RFC9213_PARSER_H

#include <stdint.h>

/* RFC 9213 Sabitleri */
#define RFC9213_MIN_URGENCY     0
#define RFC9213_MAX_URGENCY     7
#define RFC9213_DEFAULT_URGENCY 3
#define RFC9213_DEFAULT_INC     0

/**
 * @brief Parse edilmis RFC 9213 oncelik degeri
 */
typedef struct {
    uint8_t urgency;     /* 0 (en yuksek) - 7 (en dusuk) */
    uint8_t incremental; /* 0 = false, 1 = true */
    int     valid;       /* Parse basarili mi? 1=evet, 0=hayir */
} RFC9213Priority;

/**
 * @brief RFC 9213 Priority header string'ini parse eder
 *
 * Ornek gecerli degerler:
 *   "u=1"         -> urgency=1, incremental=0
 *   "u=3, i"      -> urgency=3, incremental=1
 *   "u=0, i=?1"   -> urgency=0, incremental=1
 *   "u=7, i=?0"   -> urgency=7, incremental=0
 *   ""             -> default: urgency=3, incremental=0
 *
 * @param header_value RFC 9213 Priority header string
 * @return RFC9213Priority parse sonucu
 */
RFC9213Priority rfc9213_parse(const char *header_value);

/**
 * @brief RFC 9213 oncelik degerini Min-Heap icin sayisal oncelik skoruna donusturur
 *
 * Dusuk skor = daha yuksek oncelik (Min-Heap icin)
 * Incremental istekler ayni urgency seviyesinde daha dusuk oncelik alir.
 *
 * @param priority Parse edilmis RFC9213Priority
 * @return int Hesaplanan oncelik skoru (heap key olarak kullanilir)
 */
int rfc9213_to_heap_key(RFC9213Priority priority);

/**
 * @brief RFC9213Priority yapisini string olarak yazdirir (debug amacli)
 *
 * @param priority RFC9213Priority yapisi
 * @param buf Yazilacak buffer
 * @param buf_size Buffer boyutu
 */
void rfc9213_to_string(RFC9213Priority priority, char *buf, int buf_size);

#endif /* RFC9213_PARSER_H */
