#include "server/log_store_server.h"

#include "error_handling.h"
#include "cmd_parse.h"

using namespace ::datastore;

int main(int argc, char **argv) {

  utils::error_handling::install_signal_handler(SIGSEGV, SIGKILL, SIGSTOP);

  cmd_options opts;
  opts.add(
      cmd_option("port", 'p', false).set_default("9090").set_description(
          "Port that server listens on"));
  opts.add(
      cmd_option("storage", 's', false).set_default("in-memory").set_description(
          "Storage scheme (in-memory, persistent)"));

  opts.add(
      cmd_option("concurrency-control", 'c', false).set_default("read-stalled")
          .set_description("Concurrency-control scheme"));

  opts.add(
      cmd_option("data-path", 'd', false).set_default(".").set_description(
          "Data path"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  int port;
  std::string concurrency_control;
  std::string storage_scheme;
  std::string data_path;
  try {
    port = parser.get_int("port");
    concurrency_control = parser.get("concurrency-control");
    storage_scheme = parser.get("storage");
    data_path = parser.get("data-path");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  if (storage_scheme == "in-memory") {
    if (concurrency_control == "write-stalled") {
      log_store<in_memory, write_stalled> store(data_path);
      log_store_server::start(store, port);
    } else if (concurrency_control == "read-stalled") {
      log_store<in_memory, read_stalled> store(data_path);
      log_store_server::start(store, port);
    } else {
      fprintf(stderr, "Unknown concurrency control: %s\n",
              concurrency_control.c_str());
    }
  } else if (storage_scheme == "persistent") {
    if (concurrency_control == "write-stalled") {
      log_store<persistent, write_stalled> store(data_path);
      log_store_server::start(store, port);
    } else if (concurrency_control == "read-stalled") {
      log_store<persistent, read_stalled> store(data_path);
      log_store_server::start(store, port);
    } else {
      fprintf(stderr, "Unknown concurrency control: %s\n",
              concurrency_control.c_str());
    }
  } else {
    fprintf(stderr, "Unknown storage scheme: %s\n", storage_scheme.c_str());
  }

  return 0;
}
