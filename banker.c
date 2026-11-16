#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int n_res;      /* tipos de recurso = quantidade de argumentos */
static int *available; /* [n_res] */

/* mensagem de erro e fim da execucao; nada e gravado em result.txt antes disso */
static void die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    putchar('\n');
    exit(EXIT_FAILURE);
}

static void *xmalloc(size_t size)
{
    void *p = malloc(size);

    if (p == NULL)
        die("Fail to allocate memory");
    return p;
}

/*
 * Le inteiros nao negativos separados por `sep` ate o fim da string.
 * Guarda os primeiros `max` em `out` e devolve quantos existem na linha,
 * ou -1 se houver qualquer coisa fora do formato (sinal, letra, vazio...).
 */
static int parse_list(const char *s, char sep, int *out, int max)
{
    int n = 0;

    for (;;) {
        char *end;
        long v;

        if (!isdigit((unsigned char)*s))
            return -1;
        errno = 0;
        v = strtol(s, &end, 10);
        if (errno != 0 || v > INT_MAX)
            return -1;
        if (n < max)
            out[n] = (int)v;
        n++;
        s = end;
        if (*s == '\0')
            return n;
        if (*s != sep)
            return -1;
        s++;
    }
}

int main(int argc, char **argv)
{
    int i;

    if (argc < 2)
        die("Fail: inform the instances of each resource type, e.g. ./banker 10 5 7");
    n_res = argc - 1;
    available = xmalloc(n_res * sizeof *available);
    for (i = 0; i < n_res; i++)
        if (parse_list(argv[i + 1], ' ', &available[i], 1) != 1)
            die("Fail: invalid number of instances '%s' in command line", argv[i + 1]);

    free(available);
    return 0;
}
