#include "server/log_store_server.h"

#include "error_handling.h"
#include "cmd_parse.h"

using namespace ::datastore;

template<typename A, typename B>
void start(std::string& data_path, int port, int aux) {
  if (aux == 0) {
    datastore::append_only::log_store<A, B> store(data_path);
    log_store_server::start(store, port);
  } else if (aux == 1) {
    datastore::append_only::log_store<A, B, dummy_aux<128, 1>> store(data_path);
    log_store_server::start(store, port);
  } else if (aux == 2) {
    datastore::append_only::log_store<A, B, dummy_aux<128, 2>> store(data_path);
    log_store_server::start(store, port);
  } else if (aux == 4) {
    datastore::append_only::log_store<A, B, dummy_aux<128, 4>> store(data_path);
    log_store_server::start(store, port);
  } else if (aux == 8) {
    datastore::append_only::log_store<A, B, dummy_aux<128, 8>> store(data_path);
    log_store_server::start(store, port);
  } else if (aux == 16) {
    datastore::append_only::log_store<A, B, dummy_aux<128, 16>> store(
        data_path);
    log_store_server::start(store, port);
  } else {
    fprintf(stderr, "Invalid dummy-aux value %d\n", aux);
  }
}

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
      cmd_option("append-only", 'a', true).set_description(
          "Enable append only log store (no updates, deletions)"));

  opts.add(
      cmd_option("concurrency-control", 'c', false).set_default("read-stalled")
          .set_description("Concurrency-control scheme"));

  opts.add(
      cmd_option("dummy-aux", '0', false).set_default("0").set_description(
          "Add m dummy auxiliary reference log (for benchmark purposes)"));

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
  bool append_only;
  int aux;
  try {
    port = parser.get_int("port");
    concurrency_control = parser.get("concurrency-control");
    storage_scheme = parser.get("storage");
    data_path = parser.get("data-path");
    append_only = parser.get_flag("append-only");
    aux = parser.get_int("dummy-aux");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  LOG_INFO<< parser.parsed_values();

  if (!append_only) {
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
        log_store<durable, write_stalled> store(data_path);
        log_store_server::start(store, port);
      } else if (concurrency_control == "read-stalled") {
        log_store<durable, read_stalled> store(data_path);
        log_store_server::start(store, port);
      } else {
        fprintf(stderr, "Unknown concurrency control: %s\n",
                concurrency_control.c_str());
      }
    } else if (storage_scheme == "persistent-relaxed") {
      if (concurrency_control == "write-stalled") {
        log_store<durable_relaxed, write_stalled> store(data_path);
        log_store_server::start(store, port);
      } else if (concurrency_control == "read-stalled") {
        log_store<durable_relaxed, read_stalled> store(data_path);
        log_store_server::start(store, port);
      } else {
        fprintf(stderr, "Unknown concurrency control: %s\n",
                concurrency_control.c_str());
      }
    } else {
      fprintf(stderr, "Unknown storage scheme: %s\n", storage_scheme.c_str());
    }
  } else {
    if (storage_scheme == "in-memory") {
      if (concurrency_control == "write-stalled") {
        start<datastore::append_only::in_memory,
            datastore::append_only::write_stalled>(data_path, port, aux);
      } else if (concurrency_control == "read-stalled") {
        start<datastore::append_only::in_memory,
            datastore::append_only::read_stalled>(data_path, port, aux);
      } else {
        fprintf(stderr, "Unknown concurrency control: %s\n",
                concurrency_control.c_str());
      }
    } else if (storage_scheme == "persistent") {
      if (concurrency_control == "write-stalled") {
        start<datastore::append_only::durable,
            datastore::append_only::write_stalled>(data_path, port, aux);
      } else if (concurrency_control == "read-stalled") {
        start<datastore::append_only::durable,
            datastore::append_only::read_stalled>(data_path, port, aux);
      } else {
        fprintf(stderr, "Unknown concurrency control: %s\n",
                concurrency_control.c_str());
      }
    } else if (storage_scheme == "persistent-relaxed") {
      if (concurrency_control == "write-stalled") {
        start<datastore::append_only::durable_relaxed,
            datastore::append_only::write_stalled>(data_path, port, aux);
      } else if (concurrency_control == "read-stalled") {
        start<datastore::append_only::durable_relaxed,
            datastore::append_only::read_stalled>(data_path, port, aux);
      } else {
        fprintf(stderr, "Unknown concurrency control: %s\n",
                concurrency_control.c_str());
      }
    } else {
      fprintf(stderr, "Unknown storage scheme: %s\n", storage_scheme.c_str());
    }
  }

  return 0;
}
