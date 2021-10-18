#!/bin/bash

. scripts/helpers.sh

RESULT=$(./bin/ag <tests/case_1_input_agregador.txt)
TEST_OUTPUT=tests/case_1_output_agregador.txt

if test -z "$(echo "$RESULT" | diff -q $TEST_OUTPUT -)"; then
  echo_done "Agregador ✓"
else
  echo_info "EXPECTED vs RESULT:"
  echo "$RESULT" | diff --color=always -u $TEST_OUTPUT -
  echo_error "Agregador ✘"
  exit 1
fi

VALUES=(
  1000
  10000
  100000
)

for i in "${VALUES[@]}"; do
  FILE=tests/linhas_venda_"$i".txt
  TBL_RESULTS=tables/agregar_"$i".md
  ./scripts/gera_linhas_venda "$i"
  if [ -x "$(command -v hyperfine)" ]; then
    hyperfine -w 5 -r 20 -u second --export-markdown "$TBL_RESULTS" "./bin/ag <$FILE"
    echo_info "Resultados guardados em $TBL_RESULTS!"
    echo ""
  else
    time ./bin/ag <"$FILE"
    echo_warning 'hyperfine is not installed, consider doing it for better time measurements results.' >&2
  fi
done
