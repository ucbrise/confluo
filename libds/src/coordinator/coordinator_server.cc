#include "coordinator/coordinator_server.h"
#include "server/log_store_client.h"

#include "coordinator.h"
#include "string_utils.h"
#include "error_handling.h"
#include "cmd_parse.h"

int main(int argc, char **argv) {

  utils::error_handling::install_signal_handler(SIGSEGV, SIGKILL, SIGSTOP);

  cmd_options opts;
  opts.add(
      cmd_option("port", 'p', false).set_default("9091").set_description(
          "Port that server listens on"));
  opts.add(
      cmd_option("host-list", 'h', false).set_default("conf/hosts")
          .set_description("Sleep duration between snapshots"));
  opts.add(
      cmd_option("sleep-us", 's', false).set_default("0").set_description(
          "Sleep duration between snapshots"));

  cmd_parser parser(argc, argv, opts);
  if (parser.get_flag("help")) {
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  int port;
  int64_t sleep_us;
  std::string hosts_file;
  try {
    port = parser.get_int("port");
    sleep_us = parser.get_long("sleep-us");
    hosts_file = parser.get("host-list");
  } catch (std::exception& e) {
    fprintf(stderr, "could not parse cmdline args: %s\n", e.what());
    fprintf(stderr, "%s\n", parser.help_msg().c_str());
    return 0;
  }

  std::vector<std::string> hosts;
  std::vector<int> ports;
  std::ifstream in(hosts_file);
  std::string line;
  LOG_INFO << "Host list:";
  while (std::getline(in, line)) {
    std::vector<std::string> elems = utils::string_utils::split(line, ':');
    if (elems.size() == 2) {
      hosts.push_back(elems[0]);
      ports.push_back(std::stoi(elems[1]));
    } else if (elems.size() == 1) {
      hosts.push_back(elems[0]);
      ports.push_back(9090);
    } else {
      fprintf(stderr, "Could not parse hosts file\n");
      return 0;
    }
    LOG_INFO << "Host: " << hosts.back().c_str() << " Port: " << ports.back();
  }

  if (hosts.empty())
    LOG_WARN << "Not connected to any log store servers";

  std::vector<datastore::log_store_client> clients;
  for (size_t i = 0; i < hosts.size(); i++)
    clients.push_back(datastore::log_store_client(hosts[i], ports[i]));

  datastore::coordinator<datastore::log_store_client> coord(clients, sleep_us);
  coord.start();

  datastore::coordinator_server::start(coord, port);

  return 0;
}
