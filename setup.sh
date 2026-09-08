#!/bin/bash

export COINALGEBRA_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export PATH="$COINALGEBRA_HOME/bin:$COINALGEBRA_HOME/gui/bin:$PATH"

export ROOT_INCLUDE_PATH="$COINALGEBRA_HOME/include:$ROOT_INCLUDE_PATH"
