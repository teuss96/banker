# banker — Algoritmo do Banqueiro

Implementação 5 de Infraestrutura de Software (CESAR School).
Aluno: Mateus Reinaux Batista Meira (`mrbm@cesar.school`).
Repositório: https://github.com/teuss96/banker

## Objetivo

Simular o algoritmo do Banqueiro (Silberschatz, 10ª ed., p. P-45) com número de
clientes e de recursos definido pela entrada. O programa lê `customer.txt` e
`commands.txt` do diretório atual e grava `result.txt`.

## Arquivos

| Arquivo | Responsabilidade |
|---|---|
| `banker.c` | Único fonte: valida argumentos, lê `customer.txt` e `commands.txt`, executa `RQ`/`RL`/`*` com o teste de estado seguro e grava `result.txt` |
| `Makefile` | `make` gera `banker`; `make test`, `make asan`, `make clean` |
| `testes/run.sh` | Suíte: exemplo do enunciado, casos limite e casos de erro (32 casos) |
| `testes/exemplo/` | `customer.txt`, `commands.txt` do enunciado e `esperado.txt` (o `result.txt` fornecido) |
| `evidencias.log` | Sessões de terminal gravadas com `script` durante o desenvolvimento |

## Compilar

```
make clean
make
```

## Executar

Um argumento por tipo de recurso (número de instâncias). Exemplo do enunciado:

```
cp testes/exemplo/customer.txt testes/exemplo/commands.txt .
./banker 10 5 7
cat result.txt
```

## Testar

```
make test    # 32 casos; compara result.txt byte a byte com o esperado
make asan    # mesma suíte com AddressSanitizer + UBSan
```

## Formato das entradas

- `customer.txt`: uma linha por cliente, máximos separados por vírgula (`7,5,3`).
- `commands.txt`: `RQ <cliente> <r1> ... <rn>`, `RL <cliente> <r1> ... <rn>` ou `*`.

## Tratamento de erro

O programa valida os argumentos e os dois arquivos inteiros antes de executar
qualquer comando. Em caso de erro imprime uma mensagem, termina com status 1 e
não cria `result.txt`:

- sem argumentos, argumento não numérico ou negativo;
- `customer.txt` ou `commands.txt` ausente ou ilegível (`Fail to read ...`);
- formato inválido (letra, número negativo, vírgula sobrando, comando desconhecido, cliente inexistente);
- quantidade de recursos diferente da linha de comando (`Incompatibility between ... and command line`).

## Decisões principais

- Ordem das checagens de `RQ`: excede a necessidade → não há recursos livres → estado inseguro (aloca provisoriamente, roda o algoritmo de segurança e desfaz se inseguro).
- `RL` maior que o alocado é negado com mensagem própria (o enunciado não define o caso).
- Tabela do `*`: colunas com a largura do exemplo (10 e 12) que crescem se uma linha for mais larga.
- Linhas em branco, fim de linha CRLF e espaços repetidos em `commands.txt` são aceitos.

## Ambiente de teste

Windows 11 + WSL2 (Ubuntu, kernel 6.6.87.2-microsoft-standard-WSL2), gcc 15.2.0,
GNU Make 4.4.1. Não testado em macOS.
