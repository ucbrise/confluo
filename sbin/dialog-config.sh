# resolve links - $0 may be a softlink
this="${BASH_SOURCE:-$0}"
common_bin="$(cd -P -- "$(dirname -- "$this")" && pwd -P)"
script="$(basename -- "$this")"
this="$common_bin/$script"

# convert relative path to absolute path
config_bin="`dirname "$this"`"
script="`basename "$this"`"
config_bin="`cd "$config_bin"; pwd`"
this="$config_bin/$script"

export DIALOG_PREFIX="`dirname "$this"`"/..
export DIALOG_HOME="${DIALOG_PREFIX}"
export DIALOG_CONF_DIR="${DIALOG_CONF_DIR:-"$DIALOG_HOME/conf"}"
