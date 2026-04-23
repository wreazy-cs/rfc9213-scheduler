/**
 * @file rfc9213_parser.c
 * @brief RFC 9213 Extensible HTTP Priorities - Parser Implementasyonu
 *
 * RFC 9213'e gore "Priority" header degerleri Structured Fields (RFC 8941)
 * formatinda ifade edilir. Bu parser asagidaki formati destekler:
 *
 *   Priority: u=<0-7>[, i[=?0|?1]]
 *
 * Ornekler:
 *   "u=0"        -> en yuksek oncelik, non-incremental
 *   "u=3, i"     -> orta oncelik, incremental
 *   "u=5, i=?1"  -> dusuk oncelik, incremental
 *   ""            -> varsayilan: u=3, i=false
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "rfc9213_parser.h"

/* ---------- Yardimci: bosluk atla ---------- */
static const char *skip_whitespace(const char *p) {
    while (*p && isspace((unsigned char)*p)) {
        p++;
    }
    return p;
}

/* ---------- Ana Parser ---------- */
RFC9213Priority rfc9213_parse(const char *header_value) {
    RFC9213Priority result;
    result.urgency     = RFC9213_DEFAULT_URGENCY;
    result.incremental = RFC9213_DEFAULT_INC;
    result.valid       = 1;

    /* NULL veya bos header -> RFC 9213 Bolum 4: varsayilan degerler */
    if (header_value == NULL || *header_value == '\0') {
        return result;
    }

    const char *p = skip_whitespace(header_value);

    /*
     * Structured Fields token listesi parse et.
     * Format: token=value, token, token=value ...
     */
    while (*p) {
        p = skip_whitespace(p);
        if (*p == '\0') break;

        /* Token ismini oku */
        char token[32] = {0};
        int  ti = 0;
        while (*p && *p != '=' && *p != ',' && !isspace((unsigned char)*p)) {
            if (ti < (int)sizeof(token) - 1) {
                token[ti++] = *p;
            }
            p++;
        }
        token[ti] = '\0';

        p = skip_whitespace(p);

        /* Token: "u" -> urgency */
        if (strcmp(token, "u") == 0) {
            if (*p == '=') {
                p++; /* '=' atla */
                p = skip_whitespace(p);
                if (*p >= '0' && *p <= '7') {
                    result.urgency = (uint8_t)(*p - '0');
                    p++;
                } else {
                    /* Gecersiz urgency degeri */
                    result.valid = 0;
                }
            } else {
                /* '=' olmadan 'u' gecersiz */
                result.valid = 0;
            }
        }
        /* Token: "i" -> incremental flag */
        else if (strcmp(token, "i") == 0) {
            if (*p == '=') {
                p++; /* '=' atla */
                p = skip_whitespace(p);
                /* Structured Fields boolean: ?1 = true, ?0 = false */
                if (*p == '?' && *(p+1) == '1') {
                    result.incremental = 1;
                    p += 2;
                } else if (*p == '?' && *(p+1) == '0') {
                    result.incremental = 0;
                    p += 2;
                } else {
                    result.valid = 0;
                }
            } else {
                /*
                 * RFC 9213: "i" tek basina "i=?1" ile esanlamlidir
                 * Structured Fields'da bare boolean = true
                 */
                result.incremental = 1;
            }
        }
        /* Bilinmeyen token -> RFC 9213: yoksay (forward compatibility) */
        else {
            /* Eger deger varsa atla */
            if (*p == '=') {
                p++;
                p = skip_whitespace(p);
                /* Deger sonuna kadar atla */
                while (*p && *p != ',') p++;
            }
        }

        p = skip_whitespace(p);
        if (*p == ',') {
            p++; /* ayirici virgul */
        }
    }

    return result;
}

/* ---------- Heap Key Hesaplama ---------- */
int rfc9213_to_heap_key(RFC9213Priority priority) {
    /*
     * Min-Heap icin: kucuk key = yuksek oncelik
     *
     * RFC 9213: urgency 0 en yuksek oncelik, 7 en dusuk
     * -> heap_key dogrudan urgency (0 kucuk = heap'te uste cikacak)
     *
     * Incremental istekler (i=?1): ayni urgency grubunda DAHA SONRA islenecek
     * -> heap_key'e kucuk bir ek puan ekliyoruz
     *
     * heap_key = urgency * 2 + incremental
     *   u=0, i=false -> key=0  (en yuksek oncelik)
     *   u=0, i=true  -> key=1
     *   u=3, i=false -> key=6
     *   u=3, i=true  -> key=7
     *   u=7, i=false -> key=14
     *   u=7, i=true  -> key=15 (en dusuk oncelik)
     */
    int key = (int)priority.urgency * 2 + (int)priority.incremental;
    return key;
}

/* ---------- Debug String ---------- */
void rfc9213_to_string(RFC9213Priority priority, char *buf, int buf_size) {
    if (!buf || buf_size <= 0) return;

    if (!priority.valid) {
        snprintf(buf, buf_size, "[GECERSIZ]");
        return;
    }

    snprintf(buf, buf_size,
             "u=%d, i=%s (heap_key=%d)",
             priority.urgency,
             priority.incremental ? "?1(true)" : "?0(false)",
             rfc9213_to_heap_key(priority));
}
