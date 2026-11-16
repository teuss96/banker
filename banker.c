#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int n_res;      /* tipos de recurso = quantidade de argumentos */
static int n_cust;     /* clientes = linhas de customer.txt */
static int *available; /* [n_res] */
static int *maximum;   /* matrizes [n_cust][n_res] guardadas em linha */
static int *allocation;
static int *need;

#define ROW(m, c) (&(m)[(size_t)(c) * n_res])

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
        if (sep == ' ' ? !isblank((unsigned char)*s) : *s != sep)
            return -1;
        s++;
        while (sep == ' ' && isblank((unsigned char)*s))
            s++;
    }
}

/*
 * Proxima linha nao vazia de `f`, sem o fim de linha (aceita \n e \r\n) e sem
 * espacos no final. Devolve 0 no fim do arquivo. `*no` conta as linhas lidas.
 */
static int next_line(FILE *f, char **line, size_t *cap, int *no)
{
    ssize_t len;

    while ((len = getline(line, cap, f)) != -1) {
        (*no)++;
        while (len > 0 && isspace((unsigned char)(*line)[len - 1]))
            (*line)[--len] = '\0';
        if (len > 0)
            return 1;
    }
    return 0;
}

/* customer.txt: uma linha por cliente, maximo de cada recurso separado por virgula */
static void load_customers(void)
{
    FILE *f = fopen("customer.txt", "r");
    char *line = NULL;
    size_t cap = 0;
    int no = 0;

    if (f == NULL)
        die("Fail to read customer.txt");
    while (next_line(f, &line, &cap, &no)) {
        int count;

        maximum = realloc(maximum, (size_t)(n_cust + 1) * n_res * sizeof *maximum);
        if (maximum == NULL)
            die("Fail to allocate memory");
        count = parse_list(line, ',', ROW(maximum, n_cust), n_res);
        if (count < 0)
            die("Fail to read customer.txt: invalid format at line %d", no);
        if (count != n_res)
            die("Incompatibility between customer.txt and command line: "
                "line %d has %d resources, command line has %d", no, count, n_res);
        n_cust++;
    }
    if (ferror(f))
        die("Fail to read customer.txt");
    if (n_cust == 0)
        die("Fail to read customer.txt: no customers");
    free(line);
    fclose(f);

    allocation = xmalloc((size_t)n_cust * n_res * sizeof *allocation);
    need = xmalloc((size_t)n_cust * n_res * sizeof *need);
    memset(allocation, 0, (size_t)n_cust * n_res * sizeof *allocation);
    memcpy(need, maximum, (size_t)n_cust * n_res * sizeof *need);
}

/*
 * Interpreta uma linha de commands.txt: "RQ c r1..rn", "RL c r1..rn" ou "*".
 * Devolve 'Q', 'L' ou '*'; em `cmd` ficam o cliente (cmd[0]) e os recursos.
 * Qualquer problema encerra o programa com mensagem.
 */
static char parse_command(const char *line, int no, int *cmd)
{
    int count;

    if (strcmp(line, "*") == 0)
        return '*';
    if (line[0] != 'R' || (line[1] != 'Q' && line[1] != 'L') ||
        !isblank((unsigned char)line[2]))
        die("Fail to read commands.txt: unknown command at line %d", no);
    count = parse_list(line + 3, ' ', cmd, n_res + 1);
    if (count < 0)
        die("Fail to read commands.txt: invalid format at line %d", no);
    if (count != n_res + 1)
        die("Incompatibility between commands.txt and command line: "
            "line %d has %d resources, command line has %d", no, count - 1, n_res);
    if (cmd[0] >= n_cust)
        die("Fail to read commands.txt: line %d uses customer %d, "
            "but customer.txt has only %d customers", no, cmd[0], n_cust);
    return line[1];
}

/*
 * Percorre commands.txt. Com out == NULL so valida; o arquivo inteiro e
 * validado antes de qualquer comando ser executado.
 */
static void run_commands(FILE *f, FILE *out)
{
    int *cmd = xmalloc((size_t)(n_res + 1) * sizeof *cmd);
    char *line = NULL;
    size_t cap = 0;
    int no = 0;

    rewind(f);
    while (next_line(f, &line, &cap, &no)) {
        parse_command(line, no, cmd);
        if (out == NULL)
            continue;
    }
    if (ferror(f))
        die("Fail to read commands.txt");
    free(line);
    free(cmd);
}

int main(int argc, char **argv)
{
    FILE *commands;
    int i;

    if (argc < 2)
        die("Fail: inform the instances of each resource type, e.g. ./banker 10 5 7");
    n_res = argc - 1;
    available = xmalloc(n_res * sizeof *available);
    for (i = 0; i < n_res; i++)
        if (parse_list(argv[i + 1], ' ', &available[i], 1) != 1)
            die("Fail: invalid number of instances '%s' in command line", argv[i + 1]);

    load_customers();
    commands = fopen("commands.txt", "r");
    if (commands == NULL)
        die("Fail to read commands.txt");
    run_commands(commands, NULL);

    fclose(commands);
    free(available);
    free(maximum);
    free(allocation);
    free(need);
    return 0;
}
