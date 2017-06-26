#include "server/dialog_server.h"
#include "error_handling.h"
#include "cmd_parse.h"

using namespace ::dialog;

int main(int argc, char **argv) {

  utils::error_handling::install_signal_handler(SIGSEGV, SIGKILL, SIGSTOP);

  cmd_options opts;
  opts.add(
      cmd_option("port", 'p', false).set_default("9090").set_description(
          "Port that server listens on"));

  opts.add(
      cmd_option("storage", 's', false).set_default("in-memory").set_description(
          "Storage scheme (in-memory, durable_relaxed, durable)"));

  opts.add(
      cmd_option("data-path", 'd', false).set_default(".").set_description(
          "Data path (used if storage mode is set to durable or durable_relaxed)"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  int port;
  std::string storage_scheme;
  std::string data_path;
  try {
    port = parser.get_int("port");
    storage_scheme = parser.get("storage");
    data_path = parser.get("data-path");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  LOG_INFO<< parser.parsed_values();

  if (storage_scheme == "in-memory") {
    dialog_store<storage::in_memory> store(data_path);
    log_store_server::start(store, port);
  } else if (storage_scheme == "durable") {
    dialog_store<storage::durable> store(data_path);
    log_store_server::start(store, port);
  } else if (storage_scheme == "durable-relaxed") {
    dialog_store<storage::durable_relaxed> store(data_path);
    log_store_server::start(store, port);
  } else {
    fprintf(stderr, "Unknown storage scheme: %s\n", storage_scheme.c_str());
  }

  return 0;
}
