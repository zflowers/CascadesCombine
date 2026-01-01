#!/usr/bin/env bash
[[ -n "${UTILS_SH_LOADED:-}" ]] && return
UTILS_SH_LOADED=1

extract_mass() {
    local sig="$1"
    local mass=""

    # ----------------------------------------
    # SMS logic
    # ----------------------------------------
    if [[ "$sig" == *SMS* ]]; then
        # extract substring after LAST "SMS"
        local after="${sig##*SMS}"

        # strip leading underscore
        after="${after#_}"

        # replace last underscore with 0 (300_270 -> 3000270)
        if [[ "$after" == *_* ]]; then
            mass="${after%_*}0${after##*_}"
        else
            mass="$after"
        fi

        # strip anything non-numeric just in case
        mass="$(echo "$mass" | tr -cd '0-9')"
        echo "$mass"
        return
    fi

    # ----------------------------------------
    # Cascades logic: first and last number only
    # ----------------------------------------
    if [[ "$sig" == Cascades_* ]]; then
        # remove "Cascades_" prefix
        local rest="${sig#Cascades_}"
        IFS='_' read -ra TOK <<< "$rest"

        if (( ${#TOK[@]} >= 2 )); then
            mass="${TOK[0]}${TOK[-1]}"   # first + last
        else
            mass="${TOK[0]}"             # fallback if only one
        fi

        # keep only digits
        mass="$(echo "$mass" | tr -cd '0-9')"
        echo "$mass"
        return
    fi

    # ----------------------------------------
    # Generic fallback: join tokens after first
    # ----------------------------------------
    IFS='_' read -ra TOK <<< "$sig"
    for ((i=1; i<${#TOK[@]}; i++)); do
        mass+="${TOK[i]}"
    done
    mass="$(echo "$mass" | tr -cd '0-9')"
    echo "$mass"
}

