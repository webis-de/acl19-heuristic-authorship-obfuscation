#!/usr/bin/env bash

if [ "$3" == "" ]; then
    echo "USAGE: $(basename $0) OBFUSCATOR_BIN IN_CORPUS OUT_CORPUS" >&2
    exit 1
fi

OBFUSCATOR_BIN="$(realpath "$1")"
IN_CORPUS="$(realpath "$2")"
OUT_CORPUS="$(realpath "$3")"
STATS_OUT="assets/out/$(date --iso-8601=seconds)/$(basename "$OUT_CORPUS")"

mkdir -p "$STATS_OUT"

for i in $(grep " Y" ${IN_CORPUS}/truth.txt | cut -d" " -f1); do
    CASE_DIR_IN=$(echo ${IN_CORPUS}/${i})
    CASE_DIR_OUT=$(echo ${OUT_CORPUS}/${i})

    echo "Obfuscating ${CASE_DIR_IN}..." >&2
    ${OBFUSCATOR_BIN} \
        -i "${CASE_DIR_IN}/unknown.txt" \
        -f <(cat "${CASE_DIR_IN}"/known*.txt) \
        -o "${CASE_DIR_OUT}/unknown.txt" \
        -p /dev/null \
        -n "$NETSPEAK_PATH" | tee "${STATS_OUT}/${i}.log"
done
