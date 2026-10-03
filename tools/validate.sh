#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
mode="${1:---all}"
app="${2:-all}"
case "${app}" in all|tuner|personal-card) ;; *) echo "Unknown application: ${app}" >&2; exit 2 ;; esac
cd "${repo_root}"

run_static_checks() (
    test_dir="$(mktemp -d /tmp/passport-works-host.XXXXXX)"
    trap 'rm -rf -- "${test_dir}"' EXIT
    python3 tools/check_repo.py
    actionlint_bin="${ACTIONLINT_BIN:-$(command -v actionlint || true)}"
    if [[ -z "${actionlint_bin}" || ! -x "${actionlint_bin}" ]]; then
        actionlint_bin="$(./tools/install-actionlint.sh)"
    fi
    "${actionlint_bin}" -color .github/workflows/*.yml
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Iapps/tuner tests/test_tuner_pitch.c apps/tuner/tuner_pitch.c -lm -o "${test_dir}/pitch"
    "${test_dir}/pitch"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Iapps/tuner tests/test_metronome.c apps/tuner/metronome.c -o "${test_dir}/metronome"
    "${test_dir}/metronome"
    for test in display_rounding es8311_sleep_check; do
        "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Icomponents/bsp/src "tests/test_bsp_${test}.c" "components/bsp/src/bsp_${test}.c" -o "${test_dir}/${test}"
        "${test_dir}/${test}"
    done
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Itests/bsp_stubs -Icomponents/bsp/include tests/test_bsp_button.c -o "${test_dir}/button"
    "${test_dir}/button"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Itests/bsp_stubs -Icomponents/bsp/include tests/test_bsp_lvgl_init.c components/bsp/src/bsp_display_rounding.c -o "${test_dir}/lvgl"
    "${test_dir}/lvgl"
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror -Itests/audio_stubs -Icomponents/bsp/include -Icomponents/bsp/src tests/test_bsp_audio_recovery.c components/bsp/src/bsp_es8311_sleep_check.c -o "${test_dir}/audio"
    "${test_dir}/audio"
    PYTHONDONTWRITEBYTECODE=1 python3 tests/test_verify_firmware.py
    PYTHONDONTWRITEBYTECODE=1 python3 tests/test_archive_firmware.py
    echo "Host tests: PASS"
)

build_one() (
    selected_app="${1}"
    if ! command -v idf.py >/dev/null 2>&1 || [[ "$(idf.py --version)" != "ESP-IDF v5.5.3" ]]; then
        echo "Activate ESP-IDF v5.5.3 before building." >&2
        exit 1
    fi
    validation_build_dir="$(mktemp -d /tmp/passport-works-firmware.XXXXXX)"
    trap 'rm -rf -- "${validation_build_dir}"' EXIT
    idf.py -B "${validation_build_dir}" -D "SDKCONFIG=${validation_build_dir}/sdkconfig" -D "PASSPORT_APP=${selected_app}" build
    idf.py -B "${validation_build_dir}" merge-bin -o "${validation_build_dir}/Passport-Works-full.bin"
    python3 tools/verify_firmware.py "${validation_build_dir}"
    PYTHONDONTWRITEBYTECODE=1 python3 tools/archive_firmware.py create "${validation_build_dir}" --archive-root "${repo_root}/build/${selected_app}/firmware"
    mkdir -p "${repo_root}/build/${selected_app}"
    install -m 0644 "${validation_build_dir}/Passport-Works-full.bin" "${repo_root}/build/${selected_app}/Passport-Works-full.bin"
    echo "Firmware build (${selected_app}): PASS"
)

run_firmware_checks() {
    if [[ "${app}" == all ]]; then
        build_one tuner
        build_one personal-card
    else
        build_one "${app}"
    fi
}
case "${mode}" in
    --static) run_static_checks ;;
    --firmware) run_firmware_checks ;;
    --all) run_static_checks; run_firmware_checks ;;
    *) echo "Usage: ${0} [--all|--static|--firmware] [all|tuner|personal-card]" >&2; exit 2 ;;
esac
