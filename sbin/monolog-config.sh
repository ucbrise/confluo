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

export MONOLOG_PREFIX="`dirname "$this"`"/..
export MONOLOG_HOME="${MONOLOG_PREFIX}"
export MONOLOG_CONF_DIR="${MONOLOG_CONF_DIR:-"$MONOLOG_HOME/conf"}"
export DATASTORE_BIN_DIR="${DATASTORE_BIN_DIR:-"$MONOLOG_HOME/build/libds/bin"}"
export GRAPHSTORE_BIN_DIR="${GRAPHSTORE_BIN_DIR:-"$MONOLOG_HOME/build/app/graohstore/libgs/bin"}"
