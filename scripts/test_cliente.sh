#!/bin/bash

. scripts/helpers.sh

QTD=10000

[ -n "$1" ] && QTD=$1

./scripts/insere_artigos $(($QTD / 10))
./scripts/gera_pedidos_cliente $QTD

FILE=tests/pedidos_cliente_$QTD.txt
TBL_RESULTS=tables/pedidos_cliente_$QTD.txt

if [ -x "$(command -v hyperfine)" ]; then
  hyperfine -w 5 -r 20 "./bin/cv <$FILE"
  echo_info "Resultados guardados em $TBL_RESULTS!"
  echo ""
else
  time ./bin/cv <"$FILE"
  echo_warning 'hyperfine is not installed, consider doing it for better time measurements results.' >&2
fi
