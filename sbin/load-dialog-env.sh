#!/usr/bin/env bash

# This script loads dialog-env.sh if it exists, and ensures it is only loaded once.
# dialog-env.sh is loaded from DIALOG_CONF_DIR if set, or within the current directory's
# conf/ subdirectory.

if [ -z "$DIALOG_ENV_LOADED" ]; then
  export DIALOG_ENV_LOADED=1

  # Returns the parent of the directory this script lives in.
  parent_dir="$(cd "`dirname "$0"`"/..; pwd)"

  user_conf_dir="${DIALOG_CONF_DIR:-"$parent_dir"/conf}"

  if [ -f "${user_conf_dir}/dialog-env.sh" ]; then
    # Promote all variable declarations to environment (exported) variables
    set -a
    . "${user_conf_dir}/dialog-env.sh"
    set +a
  fi
fi
