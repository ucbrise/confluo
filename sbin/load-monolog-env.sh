#!/usr/bin/env bash

# This script loads monolog-env.sh if it exists, and ensures it is only loaded once.
# monolog-env.sh is loaded from MONOLOG_CONF_DIR if set, or within the current directory's
# conf/ subdirectory.

if [ -z "$MONOLOG_ENV_LOADED" ]; then
  export MONOLOG_ENV_LOADED=1

  # Returns the parent of the directory this script lives in.
  parent_dir="$(cd "`dirname "$0"`"/..; pwd)"

  user_conf_dir="${MONOLOG_CONF_DIR:-"$parent_dir"/conf}"

  if [ -f "${user_conf_dir}/monolog-env.sh" ]; then
    # Promote all variable declarations to environment (exported) variables
    set -a
    . "${user_conf_dir}/monolog-env.sh"
    set +a
  fi
fi
