#!/usr/bin/env bash
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TEST_DIR="$SCRIPT_DIR/tests"
EXPECTED_DIR="$SCRIPT_DIR/expected"
OUTPUT_DIR="$SCRIPT_DIR/output"
UDF_BIN="$SCRIPT_DIR/../udf"

if [[ ! -x "$UDF_BIN" ]]; then
  echo "ERRO: não encontrei o binário 'udf' em:"
  echo "       $UDF_BIN"
  exit 1
fi

mkdir -p "$OUTPUT_DIR"

declare -A TOTAL_POR_LETRA
declare -A PASSOU_POR_LETRA
TOTAL_GERAL=0
PASSOU_GERAL=0

cd "$TEST_DIR"

echo " A executar os testes..."
for testfile in *.udf; do
  [[ -e "$testfile" ]] || break
  name="${testfile%.udf}"
  letra="${name:0:1}"

  TOTAL_POR_LETRA[$letra]=$(( TOTAL_POR_LETRA[$letra] + 1 ))
  TOTAL_GERAL=$(( TOTAL_GERAL + 1 ))
  FAILED=0
  echo "A executar - $name"

  # 1. Compilar - com supressão total de output
  { "$UDF_BIN" "$testfile" >/dev/null 2>&1; } 2>/dev/null || FAILED=1

  # 2. Assembler
  [[ $FAILED -eq 0 ]] && yasm -felf32 "${name}.asm" &>/dev/null || FAILED=1

  # 3. Link
  [[ $FAILED -eq 0 ]] && ld -m elf_i386 -o "$name" "${name}.o" -L"$HOME/compiladores/root/usr/lib" -lrts &>/dev/null || FAILED=1

  # 4. Executar
  if [[ $FAILED -eq 0 ]]; then
    ./"$name" &> "$OUTPUT_DIR/${name}.out" || true
  else
    : > "$OUTPUT_DIR/${name}.out"
  fi

  # 5. Comparar (sem print)
  if [[ $FAILED -eq 0 ]]; then
    if diff -iwub <(tr -d '\n' < "$EXPECTED_DIR/${name}.out") <(tr -d '\n' < "$OUTPUT_DIR/${name}.out") > /dev/null 2>&1; then
      PASSOU_POR_LETRA[$letra]=$(( PASSOU_POR_LETRA[$letra] + 1 ))
      PASSOU_GERAL=$(( PASSOU_GERAL + 1 ))
    else
      FALHOU+=("$name")
    fi
  else
    FALHOU+=("$name")
  fi
done

# Limpar
for testfile in *.udf; do
  [[ -e "$testfile" ]] || break
  nome="${testfile%.udf}"
  rm -f "$TEST_DIR/${nome}.asm" "$TEST_DIR/${nome}.o" "$TEST_DIR/${nome}"
done

# Nota
NUM_CATEGORIAS=${#TOTAL_POR_LETRA[@]}
NOTA_FINAL=0

if [[ $NUM_CATEGORIAS -gt 0 ]]; then
  PESO_POR_CATEGORIA=$(bc <<< "scale=10; 100 / $NUM_CATEGORIAS")
  for letra in "${!TOTAL_POR_LETRA[@]}"; do
    total_letra=${TOTAL_POR_LETRA[$letra]}
    passou_letra=${PASSOU_POR_LETRA[$letra]:-0}

    if [[ "$total_letra" -gt 0 ]]; then
      frac=$(bc <<< "scale=10; $passou_letra / $total_letra")
      peso_contrib=$(bc <<< "scale=10; $PESO_POR_CATEGORIA * $frac")
      NOTA_FINAL=$(bc <<< "scale=10; $NOTA_FINAL + $peso_contrib")
    fi
  done
fi

if [[ ${#FALHOU[@]} -gt 0 ]]; then
  echo
  echo "Testes com falha ou output diferente:"
  for t in "${FALHOU[@]}"; do
    echo "- ❌ $t"
  done
fi


# Resumo por categoria
for letra in $(printf "%s\n" "${!TOTAL_POR_LETRA[@]}" | sort); do
  total=${TOTAL_POR_LETRA[$letra]}
  passou=${PASSOU_POR_LETRA[$letra]:-0}
  echo "$letra: $passou / $total"
done

# Resumo geral
echo "Geral: $PASSOU_GERAL / $TOTAL_GERAL"

printf "Nota final: %.2f / 100\n" "$NOTA_FINAL"
