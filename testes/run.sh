#!/bin/sh
# uso: sh testes/run.sh [executavel]
# Cada caso roda em um diretorio temporario com seus proprios customer.txt e
# commands.txt. Sai com status 1 se algum caso falhar.

BANKER=$(realpath "${1:-./banker}")
DIR=$(cd "$(dirname "$0")" && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
pass=0
fail=0

ok()  { pass=$((pass + 1)); echo "ok   - $1"; }
bad() { fail=$((fail + 1)); echo "FAIL - $1"; }

# novo "<conteudo de customer.txt>" "<conteudo de commands.txt>"
# (argumento "-" = nao cria o arquivo)
novo() {
    cd "$TMP" && rm -rf caso && mkdir caso && cd caso || exit 1
    [ "$1" = "-" ] || printf "$1" > customer.txt
    [ "$2" = "-" ] || printf "$2" > commands.txt
}

# confere "nome" args... : espera sucesso e result.txt identico a esperado.txt
confere() {
    nome=$1; shift
    if "$BANKER" "$@" > saida.txt 2>&1 && cmp -s result.txt esperado.txt; then
        ok "$nome"
    else
        bad "$nome"
        cat saida.txt
        [ -f result.txt ] && diff result.txt esperado.txt
    fi
}

# erro "nome" "trecho da mensagem" args... : espera mensagem, status de erro
# (sem crash) e nenhum result.txt criado
erro() {
    nome=$1; trecho=$2; shift 2
    "$BANKER" "$@" > saida.txt 2>&1
    st=$?
    if [ $st -ne 0 ] && [ $st -lt 126 ] && grep -q "$trecho" saida.txt && [ ! -e result.txt ]; then
        ok "$nome -> $(cat saida.txt)"
    else
        bad "$nome (status $st)"
        cat saida.txt
    fi
}

CUST='7,5,3\n3,2,2\n9,0,2\n2,2,2\n4,3,3\n'

echo "== saida correta"

novo - -
cp "$DIR/exemplo/customer.txt" "$DIR/exemplo/commands.txt" "$DIR/exemplo/esperado.txt" .
confere "exemplo do enunciado (./banker 10 5 7)" 10 5 7

novo - -
sed 's/$/\r/' "$DIR/exemplo/customer.txt" > customer.txt
sed 's/$/\r/' "$DIR/exemplo/commands.txt" > commands.txt
cp "$DIR/exemplo/esperado.txt" .
confere "exemplo com fim de linha CRLF" 10 5 7

novo '5\n' 'RQ 0 3\nRL 0 1\n*\nRQ 0 3\nRQ 0 4\nRL 0 9\n'
printf '%s\n' \
    'Allocate to customer 0 the resources 3' \
    'Release from customer 0 the resources 1' \
    'MAXIMUM   |   ALLOCATION  |   NEED' \
    '5         |   2           |   3 ' \
    'AVAILABLE 3 ' \
    'Allocate to customer 0 the resources 3' \
    'The customer 0 request 4 was denied because exceed its maximum allocation' \
    'The customer 0 released 9 was denied because exceed its maximum allocation' \
    > esperado.txt
confere "limite: 1 cliente e 1 recurso, liberacao maior que a alocacao" 5

novo '100,20\n15,7\n' 'RQ 0 60 10\nRQ 1 15 0\n*\nRQ 1 0 7\nRQ 0 40 10\nRQ 0 0 0\n'
printf '%s\n' \
    'Allocate to customer 0 the resources 60 10' \
    'Allocate to customer 1 the resources 15 0' \
    'MAXIMUM   |   ALLOCATION  |   NEED' \
    '100 20    |   60 10       |   40 10 ' \
    '15 7      |   15 0        |   0 7 ' \
    'AVAILABLE 25 10 ' \
    'Allocate to customer 1 the resources 0 7' \
    'The resources 25 3 are not enough to customer 0 request 40 10 ' \
    'Allocate to customer 0 the resources 0 0' \
    > esperado.txt
confere "numeros com mais de um digito e pedido de zero recursos" 100 20

novo '1,1,1,1,1,1\n' '*\n'
printf '%s\n' \
    'MAXIMUM     |   ALLOCATION  |   NEED' \
    '1 1 1 1 1 1 |   0 0 0 0 0 0 |   1 1 1 1 1 1 ' \
    'AVAILABLE 1 1 1 1 1 1 ' \
    > esperado.txt
confere "6 recursos: coluna mais larga que o cabecalho" 1 1 1 1 1 1

novo "$CUST" ''
: > esperado.txt
confere "commands.txt vazio gera result.txt vazio" 10 5 7

novo "$CUST" '\nRQ 0 0 2 1\n\n\n'
echo 'Allocate to customer 0 the resources 0 2 1' > esperado.txt
confere "linhas em branco sao ignoradas" 10 5 7

echo "== tratamento de erro"

novo "$CUST" '*\n'
erro "sem argumentos" "Fail"
erro "argumento nao numerico" "Fail" 10 x 7
erro "argumento negativo" "Fail" 10 -5 7
erro "argumento maior que INT_MAX" "Fail" 10 99999999999 7
erro "menos argumentos que recursos em customer.txt" "Incompatibility between customer.txt" 10 5
erro "mais argumentos que recursos em customer.txt" "Incompatibility between customer.txt" 10 5 7 1

novo - '*\n'
erro "customer.txt ausente" "Fail to read customer.txt" 10 5 7
novo "$CUST" -
erro "commands.txt ausente" "Fail to read commands.txt" 10 5 7

novo '' '*\n'
erro "customer.txt vazio" "Fail to read customer.txt" 10 5 7
novo '7,a,3\n' '*\n'
erro "customer.txt com letra" "Fail to read customer.txt" 10 5 7
novo '7,-5,3\n' '*\n'
erro "customer.txt com numero negativo" "Fail to read customer.txt" 10 5 7
novo '7,5,3,\n' '*\n'
erro "customer.txt com virgula sobrando" "Fail to read customer.txt" 10 5 7
novo '7 5 3\n' '*\n'
erro "customer.txt separado por espaco" "Fail to read customer.txt" 10 5 7
novo '7,5,3\n3,2\n' '*\n'
erro "customer.txt com linha de tamanho diferente" "Incompatibility between customer.txt" 10 5 7

novo "$CUST" 'XX 0 1 1 1\n'
erro "commands.txt com comando desconhecido" "Fail to read commands.txt" 10 5 7
novo "$CUST" 'RQ\n'
erro "commands.txt com RQ sem valores" "Fail to read commands.txt" 10 5 7
novo "$CUST" '* 1\n'
erro "commands.txt com lixo depois do *" "Fail to read commands.txt" 10 5 7
novo "$CUST" 'RQ 0 1 b 1\n'
erro "commands.txt com letra" "Fail to read commands.txt" 10 5 7
novo "$CUST" 'RL 0 -1 0 0\n'
erro "commands.txt com numero negativo" "Fail to read commands.txt" 10 5 7
novo "$CUST" 'RQ 5 0 0 0\n'
erro "commands.txt com cliente inexistente" "Fail to read commands.txt" 10 5 7
novo "$CUST" 'RQ 0 1 1\n'
erro "commands.txt com menos recursos que argumentos" "Incompatibility between commands.txt" 10 5 7
novo "$CUST" 'RL 0 1 1 1 1\n'
erro "commands.txt com mais recursos que argumentos" "Incompatibility between commands.txt" 10 5 7
novo "$CUST" 'RQ 0 0 2 1\n*\nRQ 1 2 0\n'
erro "erro so na ultima linha: nada e executado" "Incompatibility between commands.txt" 10 5 7

echo "== $pass ok, $fail falharam"
[ "$fail" -eq 0 ]
