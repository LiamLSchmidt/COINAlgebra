#!/bin/bash

export COINALGEBRA_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export COINALGEBRA_CONFIG_DIR="$COINALGEBRA_HOME/config"
if [ -d "$COINALGEBRA_HOME/share/COINAlgebra" ]; then
    export COINALGEBRA_DATA_DIR="$COINALGEBRA_HOME/share/COINAlgebra"
else
    export COINALGEBRA_DATA_DIR="$COINALGEBRA_HOME"
fi

# Single place to edit for a user install of ROOT.
# Example:
#   export ROOT_PREFIX=/usr/local
#   export ROOT_PREFIX=/opt/root
#   export ROOT_PREFIX=/root/miniconda3/envs/root626
# If left unset, the script will try the current ROOT environment first and then
# common install locations.
: "${ROOT_PREFIX:=${ROOTSYS:-}}"

# Make the project-local executable resolve before any stale system wrapper.
case ":$PATH:" in
    *":$COINALGEBRA_HOME/bin:"*) ;;
    *) export PATH="$COINALGEBRA_HOME/bin:$PATH" ;;
esac
unalias coinalgebra 2>/dev/null || true
unset -f coinalgebra 2>/dev/null || true
hash -r

# Initialize ROOT if it is installed but not yet loaded in this shell.
if [ -z "${ROOTSYS:-}" ]; then
    if [ -n "${ROOT_PREFIX:-}" ] && [ -f "${ROOT_PREFIX}/bin/thisroot.sh" ]; then
        source "${ROOT_PREFIX}/bin/thisroot.sh"
    elif command -v root >/dev/null 2>&1; then
        ROOTSYS="$(root-config --prefix 2>/dev/null || true)"
        if [ -n "${ROOTSYS:-}" ] && [ -f "${ROOTSYS}/bin/thisroot.sh" ]; then
            source "${ROOTSYS}/bin/thisroot.sh"
        fi
    fi

    if [ -z "${ROOTSYS:-}" ]; then
        for root_path in \
            /usr/local/bin/thisroot.sh \
            /usr/local/root/bin/thisroot.sh \
            /opt/root/bin/thisroot.sh \
            /usr/lib/root/bin/thisroot.sh \
            /usr/bin/thisroot.sh; do
            if [ -f "$root_path" ]; then
                source "$root_path"
                break
            fi
        done
    fi
fi

# Conda-activated ROOT environments may inject compiler flags that conflict with
# ROOT's own Cling headers. Clear those to keep the project build compatible.
unset CPPFLAGS CFLAGS CXXFLAGS LDFLAGS

export LD_LIBRARY_PATH="$COINALGEBRA_HOME/lib:${LD_LIBRARY_PATH:-}"
export ROOT_INCLUDE_PATH="$COINALGEBRA_HOME/include:${ROOT_INCLUDE_PATH:-}"
export ROOT_PREFIX="${ROOTSYS:-${ROOT_PREFIX:-}}"
