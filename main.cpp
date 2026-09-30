#include <iostream>
#include <vector>
#include <thread>
#include <sstream>
#include <unistd.h>

#include "threadfuncs.h"

int main() {
  about();

  // Open log file
  Logger logger("output.log");

  {
    std::ostringstream oss;
    oss << "main: pid = " << ::getpid()
        << ", opened file: 'output.log'\n";
    logger.writeLine(oss.str());
  }

  // args for threads
  std::vector<ThreadArgs> args(COUNT_THREADS);

  for (int i = 0; i < COUNT_THREADS; ++i) {
    std::ostringstream oss;
    oss << "T" << i;

    args[i].id = i;
    args[i].tag = oss.str();
  }

  // threads are starting
  std::vector<std::thread> threads;
  threads.reserve(COUNT_THREADS);

  for (int i = 0; i < COUNT_THREADS; ++i) {
    threads.emplace_back(funcThread, std::cref(args[i]), std::ref(logger));
  }

  // wait for stop all threads
  for (auto& t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  logger.writeLine("main: all threads finished\n");

  return 0;
}
