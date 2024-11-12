// Copyright (c) 2024 Andrew Drogalis
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

#include <chrono>
#include <dro/dense_hashmap.hpp>
#include <iostream>
#include <random>
#include <unistd.h>
#include <unordered_map>

int main(int argc, char* argv[]) {
  size_t size  = 1'000'000;
  size_t iters = 10'000'000;

  {
    dro::dense_hashmap<int, int> hashmap {size};
    std::cout << "dro::dense_hashmap: \n";

    std::minstd_rand generator(0);
    std::uniform_int_distribution<int> uniform_distribution(2, size);

    for (size_t i {}; i < size; ++i) {
      const int value = uniform_distribution(generator);
      hashmap.insert({value, {}});
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i {}; i < iters; ++i) {
      const int value = uniform_distribution(generator);
      const auto it   = hashmap.find(value);
      if (it == hashmap.end()) {
        hashmap.insert({value, {}});
      } else {
        hashmap.erase(it);
      }
    }
    auto stop = std::chrono::high_resolution_clock::now();

    std::cout << "Mean: "
              << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                      start)
                         .count() /
                     iters
              << " ns/iter\n";
  }

  {
    std::unordered_map<int, int> hashmap {};
    hashmap.reserve(size);
    std::cout << "std::unordered_map: \n";

    std::minstd_rand generator(0);
    std::uniform_int_distribution<int> uniform_distribution(2, size);

    for (size_t i {}; i < size; ++i) {
      const int value = uniform_distribution(generator);
      hashmap.insert({value, {}});
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i {}; i < iters; ++i) {
      const int value = uniform_distribution(generator);
      const auto it   = hashmap.find(value);
      if (it == hashmap.end()) {
        hashmap.insert({value, {}});
      } else {
        hashmap.erase(it);
      }
    }
    auto stop = std::chrono::high_resolution_clock::now();

    std::cout << "Mean: "
              << std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                      start)
                         .count() /
                     iters
              << " ns/iter\n";
  }

  return 0;
}
