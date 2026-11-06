#!/usr/bin/env bash
# Run the Qt Test suites by hand (tests/README.md).
#
#   scripts/run_tests.sh              # everything: tracker, tst_core, tst_gui, tst_local
#   scripts/run_tests.sh ci           # the CI subset only
#   scripts/run_tests.sh local -R tst_local
#
# The build directory is configured on first use (CS3_BUILD_DIR overrides it);
# any argument that is not a label is passed on to ctest.

set -euo pipefail

label=all
case ${1:-} in
    ci|local|all) label=$1; shift ;;
esac

cd "$(dirname "$0")/.."
build=${CS3_BUILD_DIR:-$PWD/build}

[ -f "$build/CMakeCache.txt" ] || cmake -S . -B "$build" -DCS3_BUILD_TESTS=ON
cmake --build "$build" --parallel

# tst_gui/tst_local need a QPA platform, tst_local also a hunspell dictionary
# (without one the spell test skips itself).
export QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-offscreen}
if [ -z "${CS3_SPELL_AFF:-}" ] || [ -z "${CS3_SPELL_DIC:-}" ]; then
    for dir in "$HOME/spell" /usr/share/hunspell /usr/share/myspell; do
        [ -f "$dir/en_US.aff" ] && [ -f "$dir/en_US.dic" ] || continue
        export CS3_SPELL_AFF=$dir/en_US.aff CS3_SPELL_DIC=$dir/en_US.dic
        break
    done
fi
# The bundled zlib is shared on Windows; keep its DLL reachable (no-op on Linux).
PATH=$build/zlib/Release:$build/zlib:$PATH
export PATH

ctest_args=(--test-dir "$build" --output-on-failure)
[ "$label" = all ] || ctest_args+=(-L "$label")
ctest "${ctest_args[@]}" "$@"
