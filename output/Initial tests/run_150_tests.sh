#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
OUT_DIR="$ROOT_DIR/output"
RAW_DIR="$OUT_DIR/raw"
IMG_DIR="$OUT_DIR/screenshots"
BIN="$ROOT_DIR/project_test_runner"
CSV_FILE="$OUT_DIR/results.csv"
MD_FILE="$OUT_DIR/detailed_test_log.md"

mkdir -p "$RAW_DIR" "$IMG_DIR"

cd "$ROOT_DIR"
g++ -std=c++17 -O2 project.cpp -o "$BIN"

normalize_symbolic() {
  local s="$1"
  s="${s// /}"
  s="${s//\t/}"
  s="${s//\*/}"
  printf '%s' "$s"
}

is_numeric_equal() {
  local a="$1"
  local b="$2"
  awk -v x="$a" -v y="$b" 'BEGIN {dx=x-y; if (dx<0) dx=-dx; if (dx<=1e-9) print "1"; else print "0"}'
}

value_for_var() {
  local var="$1"
  local mode="$2"
  if [[ "$mode" == "positive" ]]; then
    case "$var" in
      x) echo "2" ;;
      y) echo "3" ;;
      z) echo "4" ;;
      a) echo "12" ;;
      b) echo "6" ;;
      c) echo "3" ;;
      *) echo "2" ;;
    esac
  else
    case "$var" in
      x) echo "-2" ;;
      y) echo "-3" ;;
      z) echo "-4" ;;
      a) echo "-12" ;;
      b) echo "6" ;;
      c) echo "-3" ;;
      *) echo "-2" ;;
    esac
  fi
}

# id|group|expression|expected
TESTS=(
"1|Group 1|1 + 2 + 3|6"
"2|Group 1|10 - 5 - 2|3"
"3|Group 1|2 + 3 * 4|14"
"4|Group 1|(2 + 3) * 4|20"
"5|Group 1|2 ^ 3 ^ 2|512"
"6|Group 1|10 / 2 * 5|25"
"7|Group 1|100 / (10 / 2)|20"
"8|Group 1|2 * 3 + 4 * 5|26"
"9|Group 1|(1 + 2) * (3 + 4)|21"
"10|Group 1|2^3 + 4^2|24"
"11|Group 2|-5 + 3|-2"
"12|Group 2|5 + -3|2"
"13|Group 2|-(-x)|x"
"14|Group 2|x - -y|x + y"
"15|Group 2|-(x + y)|-x - y"
"16|Group 2|3 * -4|-12"
"17|Group 2|-2 ^ 2|-4 or 4"
"18|Group 2|+5 + x|5 + x"
"19|Group 2|x * ---y|x * -y"
"20|Group 2|5 * (-x)|-5x"
"21|Group 3|x + 0|x"
"22|Group 3|0 + x|x"
"23|Group 3|x * 1|x"
"24|Group 3|x * 0|0"
"25|Group 3|0 / x|0"
"26|Group 3|x / 1|x"
"27|Group 3|x ^ 1|x"
"28|Group 3|x ^ 0|1"
"29|Group 3|1 ^ x|1"
"30|Group 3|(x + y) * 0|0"
"31|Group 4|x + x|2x"
"32|Group 4|2*x + 3*x|5x"
"33|Group 4|5*x - x|4x"
"34|Group 4|x + y + x|2x + y"
"35|Group 4|x * x|x^2"
"36|Group 4|x^2 * x^3|x^5"
"37|Group 4|(x^2)^3|x^6"
"38|Group 4|2 * (x + 3)|2x + 6"
"39|Group 4|x * (x + 2)|x^2 + 2x"
"40|Group 4|(x + 1) * (x + 1)|x^2 + 2x + 1"
"41|Group 5|x / x|1"
"42|Group 5|(x * y) / x|y"
"43|Group 5|1/x + 2/x|3/x"
"44|Group 5|(x^2 - 1) / (x - 1)|x + 1"
"45|Group 5|(a/b) * (b/c)|a/c"
"46|Group 6|x / 0|Error/Undefined"
"47|Group 6|0 ^ 0|1 or Error"
"48|Group 6|((((x))))|x"
"49|Group 6|x + y - x|y"
"50|Group 6|(x-x) * (y^5 + 2z)|0"
)

printf 'run_id,test_id,group_name,mode,expression,expected,vars,assignments,observed_symbolic,observed_numeric,observed_error,status,notes,raw_file,screenshot_file\n' > "$CSV_FILE"

run_id=0
pass_count=0
fail_count=0
na_count=0

for row in "${TESTS[@]}"; do
  IFS='|' read -r test_id group_name expr expected <<< "$row"

  vars="$(printf '%s' "$expr" | grep -oE '[A-Za-z_][A-Za-z0-9_]*' || true)"
  vars="$(printf '%s' "$vars" | sort -u | tr '\n' ' ' | sed 's/ $//')"

  for mode in blank positive negative; do
    run_id=$((run_id + 1))

    input_payload="$expr\n"
    assignments=""

    if [[ -n "$vars" ]]; then
      if [[ "$mode" == "blank" ]]; then
        input_payload+="\n"
        assignments="(blank first variable to force simplified-expression output)"
      else
        for v in $vars; do
          val="$(value_for_var "$v" "$mode")"
          input_payload+="$val\n"
          if [[ -z "$assignments" ]]; then
            assignments="$v=$val"
          else
            assignments+="; $v=$val"
          fi
        done
      fi
    else
      assignments="(no variables)"
    fi

    output_text="$(printf "%b" "$input_payload" | "$BIN" 2>&1 || true)"

    raw_file="$RAW_DIR/test_${test_id}_${mode}.txt"
    img_file="$IMG_DIR/test_${test_id}_${mode}.png"
    printf '%s\n' "$output_text" > "$raw_file"

    # Render text output proof image in high resolution for readability.
    if ! convert -background '#0d1117' -fill '#e6edf3' -pointsize 30 -interline-spacing 8 \
      -size 3200x4200 caption:@"$raw_file" "$img_file" 2>/dev/null; then
      convert -background white -fill black -pointsize 26 -interline-spacing 6 \
        -size 3000x4000 caption:@"$raw_file" "$img_file" 2>/dev/null || true
    fi

    observed_symbolic="$(printf '%s\n' "$output_text" | sed -n 's/^Final Simplified Expression: //p' | tail -n1)"
    if [[ -z "$observed_symbolic" ]]; then
      observed_symbolic="$(printf '%s\n' "$output_text" | sed -n 's/^Stringified Result: //p' | tail -n1)"
    fi

    observed_numeric="$(printf '%s\n' "$output_text" | sed -n 's/^Numeric Result   : //p' | tail -n1)"
    observed_error="$(printf '%s\n' "$output_text" | grep -E '^\[Phase 1 - |^Evaluation Error : |^Input stream closed\.' | tail -n1 || true)"

    status="FAIL"
    notes=""

    if [[ "$mode" == "blank" && -n "$vars" ]]; then
      status="INFO"
      notes="Blank-input mode is for simplified-expression behavior check, not strict numeric comparison."
      na_count=$((na_count + 1))
    else
      if [[ "$expected" == "Error/Undefined" ]]; then
        if [[ -n "$observed_error" ]]; then
          status="PASS"
          notes="Error surfaced as expected."
          pass_count=$((pass_count + 1))
        else
          status="FAIL"
          notes="Expected an error/undefined outcome."
          fail_count=$((fail_count + 1))
        fi
      elif [[ "$expected" == "-4 or 4" ]]; then
        if [[ -n "$observed_numeric" ]] && ([[ "$(is_numeric_equal "$observed_numeric" "-4")" == "1" ]] || [[ "$(is_numeric_equal "$observed_numeric" "4")" == "1" ]]); then
          status="PASS"
          notes="Accepted unary-minus precedence variant."
          pass_count=$((pass_count + 1))
        else
          status="FAIL"
          notes="Did not match either accepted value (-4 or 4)."
          fail_count=$((fail_count + 1))
        fi
      elif [[ "$expected" == "1 or Error" ]]; then
        if [[ -n "$observed_error" ]] || ([[ -n "$observed_numeric" ]] && [[ "$(is_numeric_equal "$observed_numeric" "1")" == "1" ]]); then
          status="PASS"
          notes="Accepted indeterminate-form policy variant."
          pass_count=$((pass_count + 1))
        else
          status="FAIL"
          notes="Expected 1 or explicit error."
          fail_count=$((fail_count + 1))
        fi
      elif [[ "$expected" =~ ^-?[0-9]+(\.[0-9]+)?$ ]]; then
        if [[ -n "$observed_numeric" ]] && [[ "$(is_numeric_equal "$observed_numeric" "$expected")" == "1" ]]; then
          status="PASS"
          notes="Numeric match."
          pass_count=$((pass_count + 1))
        else
          status="FAIL"
          notes="Numeric mismatch against expected baseline."
          fail_count=$((fail_count + 1))
        fi
      else
        exp_norm="$(normalize_symbolic "$expected")"
        obs_norm="$(normalize_symbolic "$observed_symbolic")"
        if [[ "$exp_norm" == "$obs_norm" ]]; then
          status="PASS"
          notes="Symbolic form match (normalized spaces and multiplication signs)."
          pass_count=$((pass_count + 1))
        else
          status="FAIL"
          notes="Symbolic form differs from expected baseline."
          fail_count=$((fail_count + 1))
        fi
      fi
    fi

    esc_expr="${expr//\"/\"\"}"
    esc_expected="${expected//\"/\"\"}"
    esc_vars="${vars//\"/\"\"}"
    esc_assignments="${assignments//\"/\"\"}"
    esc_observed_symbolic="${observed_symbolic//\"/\"\"}"
    esc_observed_numeric="${observed_numeric//\"/\"\"}"
    esc_observed_error="${observed_error//\"/\"\"}"
    esc_notes="${notes//\"/\"\"}"

    printf '%s,%s,"%s",%s,"%s","%s","%s","%s","%s","%s","%s",%s,"%s",%s,%s\n' \
      "$run_id" "$test_id" "$group_name" "$mode" "$esc_expr" "$esc_expected" "$esc_vars" "$esc_assignments" \
      "$esc_observed_symbolic" "$esc_observed_numeric" "$esc_observed_error" "$status" "$esc_notes" \
      "$raw_file" "$img_file" >> "$CSV_FILE"
  done
done

{
  echo "# Algebraic Solver Stress-Test Log (150 Runs)"
  echo
  echo "Generated: $(date -Is)"
  echo
  echo "## Scope"
  echo "- Base list: 50 expressions provided by user"
  echo "- Per-expression run modes: blank-input symbolic check, positive assignments, negative assignments"
  echo "- Total runs: 150"
  echo "- Imaginary/complex-number note: not possible with current solver because it uses real-valued doubles and the grammar has no complex/imaginary literals."
  echo
  echo "## Assignment Sets"
  echo "- Positive set: x=2, y=3, z=4, a=12, b=6, c=3"
  echo "- Negative set: x=-2, y=-3, z=-4, a=-12, b=6, c=-3"
  echo "- Blank mode: first variable left empty to verify simplified-expression fallback"
  echo
  echo "## Summary"
  printf -- '- PASS: %s\n' "$pass_count"
  printf -- '- FAIL: %s\n' "$fail_count"
  printf -- '- INFO/N-A: %s\n' "$na_count"
  echo
  echo "## Per-Run Results"
  echo
  echo "| Run | Test | Group | Mode | Expression | Expected | Observed Symbolic | Observed Numeric | Error | Status | Notes | Raw Log | Screenshot |"
  echo "|---:|---:|---|---|---|---|---|---|---|---|---|---|---|"

  awk -F',' 'NR>1 {
    run=$1; test=$2; group=$3; mode=$4; expr=$5; expected=$6; osym=$9; onum=$10; err=$11; status=$12; notes=$13; raw=$14; shot=$15;
    gsub(/^"|"$/, "", group); gsub(/^"|"$/, "", expr); gsub(/^"|"$/, "", expected);
    gsub(/^"|"$/, "", osym); gsub(/^"|"$/, "", onum); gsub(/^"|"$/, "", err);
    gsub(/^"|"$/, "", notes); gsub(/^"|"$/, "", raw); gsub(/^"|"$/, "", shot);
    gsub(/\|/, "\\|", expr); gsub(/\|/, "\\|", expected); gsub(/\|/, "\\|", osym); gsub(/\|/, "\\|", onum); gsub(/\|/, "\\|", err); gsub(/\|/, "\\|", notes);
    sub(/^.*\/output\//, "output/", raw);
    sub(/^.*\/output\//, "output/", shot);
    printf("| %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | [%s](%s) | [%s](%s) |\n", run, test, group, mode, expr, expected, osym, onum, err, status, notes, raw, raw, shot, shot);
  }' "$CSV_FILE"

  echo
  echo "## Notes"
  echo "- Screenshot proofs are rendered from each captured terminal run output as PNG files in output/screenshots/."
  echo "- Raw text transcripts are in output/raw/."
} > "$MD_FILE"

echo "Completed 150 runs."
echo "CSV: $CSV_FILE"
echo "Report: $MD_FILE"
echo "Screenshots: $IMG_DIR"
