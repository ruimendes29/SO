#!/bin/bash

VALUES=(
  1000
  10000
  100000
)

for i in "${VALUES[@]}"; do
  ./scripts/insere_artigos "$i"
  ./scripts/troca_precos_artigos "$i"
  ./scripts/troca_nomes_artigos "$i"
done
