#include <iostream>
#include <vector>
#include <thread>
#include <sstream>
#include <future>
#include <string>
#include <mutex>
#include <condition_variable>
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

  // =========================================================
  // Main worker threads
  // =========================================================

  std::vector<ThreadArgs> args(COUNT_THREADS);

  for (int i = 0; i < COUNT_THREADS; ++i) {
    std::ostringstream oss;
    oss << "T" << i;

    args[i].id = i;
    args[i].tag = oss.str();
  }

  std::vector<std::thread> threads;
  threads.reserve(COUNT_THREADS);

  for (int i = 0; i < COUNT_THREADS; ++i) {
    threads.emplace_back(
        funcThread,
        std::cref(args[i]),
        std::ref(logger)
    );
  }

  // Compare std::thread::id
  {
    std::ostringstream oss;

    oss << "thread[0].id = " << threads[0].get_id()
        << ", thread[1].id = " << threads[1].get_id()
        << ", equal = "
        << (threads[0].get_id() == threads[1].get_id())
        << "\n";

    logger.writeLine(oss.str());
  }

  // Wait for worker threads
  for (auto& t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  // Atomic counter result
  {
    std::ostringstream oss;
    oss << "counter = " << counter << "\n";
    logger.writeLine(oss.str());
  }


  // =========================================================
  // Promise / future
  // =========================================================

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


  // =========================================================
  // Producer - Consumer
  // Buffer size = 1
  // =========================================================

  std::mutex pcMutex;
  std::condition_variable cv;

  int buffer = 0;
  bool ready = false;
  bool done = false;

  // Consumer
  std::thread consumer([&]() {
    while (true) {
      std::unique_lock<std::mutex> lock(pcMutex);

      cv.wait(lock, [&]() {
        return ready || done;
      });

      // Producer finished and there is no value left
      if (!ready && done) {
        break;
      }

      int value = buffer;
      ready = false;

      // Release mutex before writing to log
      lock.unlock();

      // Wake producer waiting for empty buffer
      cv.notify_one();

      std::ostringstream oss;
      oss << "consumer: value = " << value << "\n";
      logger.writeLine(oss.str());
    }
  });


  // Producer
  std::thread producer([&]() {
    for (int value = 1; value <= 10; ++value) {
      std::unique_lock<std::mutex> lock(pcMutex);

      // Wait until previous value has been consumed
      cv.wait(lock, [&]() {
        return !ready;
      });

      buffer = value;
      ready = true;

      lock.unlock();

      // Tell consumer that a value is ready
      cv.notify_one();
    }

    // Wait until consumer takes the last value
    {
      std::unique_lock<std::mutex> lock(pcMutex);

      cv.wait(lock, [&]() {
        return !ready;
      });

      done = true;
    }

    // Wake consumer so it can terminate
    cv.notify_one();
  });


  if (producer.joinable()) {
    producer.join();
  }

  if (consumer.joinable()) {
    consumer.join();
  }


  logger.writeLine("main: all threads finished\n");

  return 0;
}
