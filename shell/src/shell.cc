#include "shell.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <unistd.h>
#include <string>
#include <sstream>
#include <iostream>

void Shell::Run() {
  ShellConnection cx("localhost", 11002);

  std::string input;
  int64_t cur_key = 0;
  while (true) {
    char cmd_line[500];
    std::cout << "shell> ";
    std::cin.getline(cmd_line, sizeof(cmd_line));

    std::istringstream iss(cmd_line);
    std::string cmd, arg;

    if (!(iss >> cmd >> arg)) {
      std::cerr << "Could not parse command: " << cmd_line << "\n";
      continue;
    }

    if (cmd == "get" || cmd == "GET") {
      int64_t key = atol(arg.c_str());
      std::string result;
      cx.client->Get(result, key);
      std::cout << "KEY: " << key << "; VALUE: " << result << "\n";
    } else if (cmd == "put" || cmd == "PUT") {
      int ret = cx.client->Append(cur_key++, arg);
      if (ret == 0) {
        std::cout << "PUT successful. KEY: " << cur_key - 1 << "; VALUE: "
                  << arg << "\n";
      }
    } else if (cmd == "search" || cmd == "SEARCH") {
      std::set<int64_t> ret;
      cx.client->Search(ret, arg);
      std::cout << "Found " << ret.size() << " matches.\n";
      for (auto key : ret) {
        std::string value;
        cx.client->Get(value, key);
        std::cout << "KEY: " << key << "; VALUE: " << value << "\n";
      }
    } else {
      std::cerr << "Unknown command: " << cmd << "\n";
    }
  }
}

int main(int argc, char** argv) {
  Shell shell;
  shell.Run();
  return 0;
}
