#include "dialog_server.h"
#include "cmd_parse.h"
#include "error_handling.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace ::dialog;
using namespace ::dialog::rpc;
using namespace ::utils;

int main(int argc, char **argv) {
  utils::error_handling::install_signal_handler(SIGSEGV, SIGKILL, SIGSTOP);

  cmd_options opts;
  opts.add(
      cmd_option("endpoint", 'e', false).set_default("127.0.0.1:9090")
          .set_description("Endpoint (address:port) that server listens on"));

  opts.add(
      cmd_option("successor", 's', false).set_description(
          "Endpoint (address:port) of successor in replica chain"));

  opts.add(
      cmd_option("tail", 't', false).set_description(
          "Endpoint (address:port) of tail in replica chain"));

  opts.add(
      cmd_option("replica-id", 'r', false).set_default("0").set_description(
          "Position of server in replica chain"));

  opts.add(
      cmd_option("data-path", 'd', false).set_default(".").set_description(
          "Data path for DiaLog"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  std::string host_ep, successor_ep, tail_ep;
  int replica_id;
  std::string data_path;
  try {
    host_ep = parser.get("endpoint");
    successor_ep = parser.get("successor");
    tail_ep = parser.get("tail");
    replica_id = parser.get_int("replica-id");
    data_path = parser.get("data-path");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  rpc_endpoint host(host_ep);
  rpc_endpoint successor;
  rpc_endpoint tail;

  if (successor_ep != "") {
    successor = rpc_endpoint(successor_ep);
  }

  if (tail_ep != "") {
    tail = rpc_endpoint(tail_ep);
  }

  LOG_INFO<< parser.parsed_values();

  dialog_store* store = new dialog_store(data_path);

  try {
    auto server = dialog_server::create(store, host, successor, tail);
    server->serve();
  } catch (std::exception& e) {
    LOG_ERROR<<"Could not start server listening on " << host.addr() << ":" << host.port() << ": " << e.what();
  }

  return 0;
}

