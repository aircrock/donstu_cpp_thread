#include <iostream>
#include <vector>
#include <thread>
#include <sstream>
#include <future>
#include <string>
#include <unistd.h>

#include "threadfuncs.h"


void resultThread(std::promise<std::string> prom) {
  int completed = 0;

  for (int i = 0; i < COUNT_ITERATIONS; ++i) {
    ++completed;
  }

  prom.set_value(
      "result thread: completed iterations = " +
      std::to_string(completed)
  );
}


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
    threads.emplace_back(
        funcThread,
        std::cref(args[i]),
        std::ref(logger)
    );
  }

  // compare std::thread::id
  {
    std::ostringstream oss;

    oss << "thread[0].id = " << threads[0].get_id()
        << ", thread[1].id = " << threads[1].get_id()
        << ", equal = "
        << (threads[0].get_id() == threads[1].get_id())
        << "\n";

    logger.writeLine(oss.str());
  }

  // wait for all threads
  for (auto& t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  // atomic counter result
  {
    std::ostringstream oss;
    oss << "counter = " << counter << "\n";
    logger.writeLine(oss.str());
  }

  // promise / future
  std::promise<std::string> prom;
  std::future<std::string> fut = prom.get_future();

  std::thread resultWorker(
      resultThread,
      std::move(prom)
  );

  std::string result = fut.get();

  if (resultWorker.joinable()) {
    resultWorker.join();
  }

  logger.writeLine(result + "\n");

  logger.writeLine("main: all threads finished\n");

  return 0;
}
