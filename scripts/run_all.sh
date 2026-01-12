#!/usr/bin/env bash
set -euo pipefail

# run_all.sh
# Usage: ./run_all.sh [--run-name LABEL] [--existing-BFI-dir PATH | --existing-BF-dir PATH] [other run_combine.py args...]
#
# Behavior:
#  - If --run-name LABEL is given, final run dir will be: run_<LABEL>_<MonthDay_Year_HourMin>
#  - If not given, final run dir will be: run_<MonthDay_Year_HourMin>
#  - If that run dir exists, try _1 .. _999 suffixes immediately. If all used, sleep 60s and retry.
#  - If --existing-BFI-dir or --existing-BF-dir is provided, do NOT create a new run dir; use that existing dir.
#  - Before launching, do a non-blocking check of runs/.build_and_stage.lock and exit/wait as configured.
#  - Launch: nohup python3 python/run_combine.py [--run-name "<final_run_name>"] [other args...] >> runs/<final_run_name>/debug_run_combine.debug 2>&1 &
#
LOCKFILE="runs/.build_and_stage.lock"

RUN_NAME_PREFIX=""
ARGS=()

# Recognize these existing-run flags (both hyphen and underscore forms)
EXISTING_BFI_DIR=""
EXISTING_BF_DIR=""
LOCK_WAIT_SECS=60  # default seconds to wait if lock held

# Parse args: capture --run-name, --existing-BFI-dir / --existing_BFI_dir, --existing-BF-dir / --existing_BF_dir,
# and --lock-wait=*, and forward everything to ARGS so the python script receives them too.
while (( "$#" )); do
  case "$1" in
    --run-name)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: --run-name requires an argument" >&2
        exit 2
      fi
      RUN_NAME_PREFIX="$2"
      # don't forward run-name yet: we'll forward only if we actually create a new run name later
      shift 2
      ;;
    --existing-BFI-dir|--existing_BFI_dir)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: $1 requires an argument" >&2
        exit 2
      fi
      EXISTING_BFI_DIR="$2"
      # forward to python exactly as provided
      ARGS+=("$1" "$2")
      shift 2
      ;;
    --existing-BF-dir|--existing_BF_dir)
      if [[ $# -lt 2 ]]; then
        echo "ERROR: $1 requires an argument" >&2
        exit 2
      fi
      EXISTING_BF_DIR="$2"
      ARGS+=("$1" "$2")
      shift 2
      ;;
    --lock-wait=*)
      LOCK_WAIT_SECS="${1#*=}"
      # forward too
      ARGS+=("$1")
      shift
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

# -------------------------
# Lock handling (non-blocking check + optional short wait)
# -------------------------
exec 9>"${LOCKFILE}"
if ! flock -n 9 ; then
  echo "[run_all] Another run is currently building/staging assets."
  echo "[run_all] Lock file: ${LOCKFILE}"
  echo "[run_all] Waiting up to ${LOCK_WAIT_SECS}s for the lock to be released..."

  sleep "${LOCK_WAIT_SECS}"

  if ! flock -n 9 ; then
    echo "[run_all] ERROR: Lock still held after ${LOCK_WAIT_SECS}s. Aborting start." >&2
    if [[ -s "${LOCKFILE}" ]]; then
      echo "[run_all] Lock owner info (from lock file):" >&2
      sed -n '1,20p' "${LOCKFILE}" >&2 || true
    else
      echo "[run_all] (lock file exists but is empty)" >&2
    fi
    exec 9>&-
    exit 1
  else
    echo "[run_all] Lock released, proceeding..."
  fi
fi
# We acquired the temporary check lock — release it immediately so the Python process can obtain it
flock -u 9
exec 9>&-

# Helper to compute timestamp-based run name
compute_base_name() {
  local ts
  ts=$(date "+%B%d_%Y_%H%M")
  if [[ -n "$RUN_NAME_PREFIX" ]]; then
    echo "run_${RUN_NAME_PREFIX}_${ts}"
  else
    echo "run_${ts}"
  fi
}

# If either existing-run flag is present, we will NOT create a new run dir here.
# Validate that existing dir actually exists.
if [[ -n "${EXISTING_BFI_DIR}" || -n "${EXISTING_BF_DIR}" ]]; then
  # Prefer BFI if both somehow provided (should be mutually exclusive in usage)
  if [[ -n "${EXISTING_BFI_DIR}" && -n "${EXISTING_BF_DIR}" ]]; then
    echo "[run_all] ERROR: cannot pass both --existing-BFI-dir and --existing-BF-dir." >&2
    exit 2
  fi

  # Choose the provided existing dir
  if [[ -n "${EXISTING_BFI_DIR}" ]]; then
    run_dir="${EXISTING_BFI_DIR}"
  else
    run_dir="${EXISTING_BF_DIR}"
  fi

  # Ensure the path is the run directory (must exist)
  if [[ ! -d "${run_dir}" ]]; then
    echo "[run_all] ERROR: specified existing run directory does not exist: ${run_dir}" >&2
    exit 2
  fi

  # We will NOT pass --run-name to python when resuming an existing run
  final_run_name=""
else
  # Fresh run: pick a unique timestamped run name, with _1 .. _999 attempts
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
fi

if [[ -n "${final_run_name}" ]]; then
  DEBUG_PATH="${run_dir}/debug_run_combine.debug"
  echo "[run_all] Logging to ${DEBUG_PATH}"
else
  echo "[run_all] Logging location will be determined by run_combine.py"
fi

# Compose command and pass --run-name only for fresh runs
CMD=(python3 python/run_combine.py)

if [[ -n "${final_run_name}" ]]; then
  CMD+=(--run-name "${final_run_name}")
fi

# Append the rest of the user-provided args (including any existing-BFI/BF flags we've already captured)
if [[ "${#ARGS[@]}" -gt 0 ]]; then
  CMD+=("${ARGS[@]}")
fi

# Ensure run_dir exists (for logging) before redirect
mkdir -p "${run_dir}"

# launch python, using nohup so it survives logout; redirect stdout/stderr to debug path
if [[ -n "${final_run_name}" ]]; then
  nohup "${CMD[@]}" >> "${DEBUG_PATH}" 2>&1 &
else
  nohup "${CMD[@]}" >/dev/null 2>&1 &
fi
PID=$!
echo "[run_all] Started PID ${PID}"
if [[ -n "${final_run_name}" ]]; then
  echo "${PID}" > "${run_dir}/run_combine.pid"
fi

