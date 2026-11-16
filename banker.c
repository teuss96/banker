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

static void print_vec(FILE *out, const int *v)
{
    int i;

    for (i = 0; i < n_res; i++)
        fprintf(out, i ? " %d" : "%d", v[i]);
}

/* a <= b em todos os recursos? */
static int fits(const int *a, const int *b)
{
    int i;

    for (i = 0; i < n_res; i++)
        if (a[i] > b[i])
            return 0;
    return 1;
}

/* entrega `req` ao cliente c (sign = 1) ou toma de volta (sign = -1) */
static void move(int c, const int *req, int sign)
{
    int i;

    for (i = 0; i < n_res; i++) {
        available[i] -= sign * req[i];
        ROW(allocation, c)[i] += sign * req[i];
        ROW(need, c)[i] -= sign * req[i];
    }
}

/*
 * Algoritmo de seguranca: o estado e seguro se existe uma ordem em que todo
 * cliente consegue o que ainda precisa usando o que esta livre mais o que os
 * clientes anteriores da ordem devolvem ao terminar.
 */
static int is_safe(void)
{
    int *work = xmalloc((size_t)n_res * sizeof *work);
    char *finish = xmalloc((size_t)n_cust);
    int c, i, left = n_cust, progress = 1;

    memcpy(work, available, (size_t)n_res * sizeof *work);
    memset(finish, 0, (size_t)n_cust);
    while (left > 0 && progress) {
        progress = 0;
        for (c = 0; c < n_cust; c++) {
            if (finish[c] || !fits(ROW(need, c), work))
                continue;
            for (i = 0; i < n_res; i++)
                work[i] += ROW(allocation, c)[i];
            finish[c] = 1;
            left--;
            progress = 1;
        }
    }
    free(work);
    free(finish);
    return left == 0;
}

static void request(FILE *out, int c, const int *req)
{
    if (!fits(req, ROW(need, c))) {
        fprintf(out, "The customer %d request ", c);
        print_vec(out, req);
        fprintf(out, " was denied because exceed its maximum allocation\n");
        return;
    }
    if (!fits(req, available)) {
        fprintf(out, "The resources ");
        print_vec(out, available);
        fprintf(out, " are not enough to customer %d request ", c);
        print_vec(out, req);
        fprintf(out, " \n"); /* o result.txt de exemplo tem esse espaco no fim */
        return;
    }
    /* aloca de mentira, testa e desfaz se o estado ficar inseguro */
    move(c, req, 1);
    if (!is_safe()) {
        move(c, req, -1);
        fprintf(out, "The customer %d request ", c);
        print_vec(out, req);
        fprintf(out, " was denied because result in an unsafe state\n");
        return;
    }
    fprintf(out, "Allocate to customer %d the resources ", c);
    print_vec(out, req);
    fputc('\n', out);
}

static void release(FILE *out, int c, const int *req)
{
    /* o enunciado nao define este caso; a mensagem segue o padrao das negacoes de RQ */
    if (!fits(req, ROW(allocation, c))) {
        fprintf(out, "The customer %d released ", c);
        print_vec(out, req);
        fprintf(out, " was denied because exceed its maximum allocation\n");
        return;
    }
    move(c, req, -1);
    fprintf(out, "Release from customer %d the resources ", c);
    print_vec(out, req);
    fputc('\n', out);
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
        char op = parse_command(line, no, cmd);

        if (out == NULL)
            continue;
        if (op == 'Q')
            request(out, cmd[0], cmd + 1);
        else if (op == 'L')
            release(out, cmd[0], cmd + 1);
    }
    if (ferror(f))
        die("Fail to read commands.txt");
    free(line);
    free(cmd);
}

int main(int argc, char **argv)
{
    FILE *commands, *result;
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

    /* so chega aqui com todas as entradas validas */
    result = fopen("result.txt", "w");
    if (result == NULL)
        die("Fail to write result.txt");
    run_commands(commands, result);
    if (fclose(result) != 0)
        die("Fail to write result.txt");

    fclose(commands);
    free(available);
    free(maximum);
    free(allocation);
    free(need);
    return 0;
}
