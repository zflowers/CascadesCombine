#!/usr/bin/env bash
set -euo pipefail

# run_all.sh
# Usage: ./run_all.sh [--run-name LABEL] [other run_combine.py args...]
#
# Behavior:
#  - If --run-name LABEL is given, final run dir will be: run_<LABEL>_<MonthDay_Year_HourMin>
#  - If not given, final run dir will be: run_<MonthDay_Year_HourMin>
#  - If that run dir exists, try _1 .. _999 suffixes immediately. If all used, sleep 60s and retry.
#  - Before launching, do a non-blocking check of runs/.build_and_stage.lock and exit immediately
#    with a friendly message if another run is currently in the build/stage critical section.
#  - Launch: nohup python3 python/run_combine.py --run-name "<final_run_name>" [other args...] >> runs/<final_run_name>/debug_run_combine.debug 2>&1 &

LOCKFILE="runs/.build_and_stage.lock"

RUN_NAME_PREFIX=""
ARGS=()

# extract a single --run-name if provided (consume it)
while (( "$#" )); do
  case "$1" in
    --run-name)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --run-name requires an argument" >&2
        exit 2
      fi
      RUN_NAME_PREFIX="$2"
      shift 2
      ;;
    *)
      ARGS+=("$1")
      shift
      ;;
  esac
done

# Ensure runs dir & lock file exist
mkdir -p runs
touch "${LOCKFILE}"

# Non-blocking advisory lock check: if another process holds the lock, inform and exit.
# We use FD 9 to avoid stomping other fds.
exec 9>"${LOCKFILE}"
if ! flock -n 9 ; then
  echo "[run_all] ERROR: Another run is currently building/staging assets. Aborting start." >&2
  echo "[run_all] Lock file: ${LOCKFILE}" >&2
  if [[ -s "${LOCKFILE}" ]]; then
    echo "[run_all] Lock owner info (from lock file):" >&2
    sed -n '1,20p' "${LOCKFILE}" >&2 || true
  else
    echo "[run_all] (lock file exists but is empty)" >&2
  fi
  echo "[run_all] Wait for that run to finish or remove ${LOCKFILE} if it is stale." >&2
  # release FD 9 and exit non-zero
  exec 9>&-
  exit 1
fi
# We acquired the temporary check lock — release it immediately so the Python process can obtain it
flock -u 9
exec 9>&-

# format timestamp down to minute (e.g. August29_2025_1315)
compute_base_name() {
  local ts
  ts=$(date "+%B%d_%Y_%H%M")
  if [[ -n "$RUN_NAME_PREFIX" ]]; then
    echo "run_${RUN_NAME_PREFIX}_${ts}"
  else
    echo "run_${ts}"
  fi
}

while true; do
  base_run_name=$(compute_base_name)
  run_dir=""
  final_run_name=""
  increment=0

  # try base name and then _1 .. _999
  while [[ $increment -lt 1000 ]]; do
    if [[ $increment -eq 0 ]]; then
      candidate="${base_run_name}"
    else
      candidate="${base_run_name}_$increment"
    fi

    if [[ ! -d "runs/${candidate}" ]]; then
      run_dir="runs/${candidate}"
      final_run_name="${candidate}"
      break
    fi
    increment=$((increment + 1))
  done

  if [[ -n "$run_dir" ]]; then
    mkdir -p "${run_dir}"
    break
  fi

  # exhausted increments: sleep then recompute timestamp & retry
  echo "[run_all] All increments used for ${base_run_name}, waiting 60s and retrying..."
  sleep 60
done

DEBUG_PATH="${run_dir}/debug_run_combine.debug"
echo "[run_all] Logging to ${DEBUG_PATH}"

# launch python, passing the exact run name (and all other args)
# Use nohup so run survives logout; since we redirect stdout/stderr to DEBUG_PATH, nohup won't create nohup.out
nohup python3 python/run_combine.py --run-name "${final_run_name}" "${ARGS[@]}" >> "${DEBUG_PATH}" 2>&1 &
PID=$!
echo "[run_all] Started PID ${PID}"
echo "${PID}" > "${run_dir}/run_combine.pid"

