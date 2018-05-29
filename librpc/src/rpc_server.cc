#include "rpc_server.h"
#include "cmd_parse.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using namespace ::confluo;
using namespace ::confluo::rpc;
using namespace ::utils;

int main(int argc, char **argv) {
  utils::error_handling::install_signal_handler(argv[0], SIGSEGV, SIGKILL,
                                                SIGSTOP);

  cmd_options opts;
  opts.add(
      cmd_option("port", 'p', false).set_default("9090").set_description(
          "Port that server listens on"));

  opts.add(
      cmd_option("address", 'a', false).set_default("127.0.0.1").set_description(
          "Address server binds to"));

  opts.add(
      cmd_option("data-path", 'd', false).set_default(".").set_description(
          "Data path for Confluo"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  int port;
  std::string address;
  std::string data_path;

  try {
    port = parser.get_int("port");
    address = parser.get("address");
    data_path = parser.get("data-path");
  } catch (std::exception &e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  LOG_INFO << parser.parsed_values();

  confluo_store *store = new confluo_store(data_path);

  try {
    auto server = rpc_server::create(store, address, port);
    server->serve();
  } catch (std::exception &e) {
    LOG_ERROR << "Could not start server listening on " << address << ":" << port << ": " << e.what();
  }

  return 0;
}

