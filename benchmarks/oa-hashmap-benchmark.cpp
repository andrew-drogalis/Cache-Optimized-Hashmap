// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <chrono>
#include <dro/oa-hashmap.hpp>
#include <iostream>
#include <random>
#include <unistd.h>
#include <unordered_map>

int main(int argc, char* argv[])
{
  size_t size  = 1'000'000;
  size_t iters = 10'000'000;

  {
    dro::HashMap<int, int> hashmap {0, size};
    std::cout << "Dro Hashmap: \n";

    std::minstd_rand generator(0);
    std::uniform_int_distribution<int> uniform_distribution(2, size);

    for (size_t i {}; i < size; ++i)
    {
      const int value = uniform_distribution(generator);
      hashmap.insert({value, {}});
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i {}; i < iters; ++i)
    {
      const int value = uniform_distribution(generator);
      const auto it   = hashmap.find(value);
      if (it == hashmap.end())
      {
        hashmap.insert({value, {}});
      }
      else
      {
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

    for (size_t i {}; i < size; ++i)
    {
      const int value = uniform_distribution(generator);
      hashmap.insert({value, {}});
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i {}; i < iters; ++i)
    {
      const int value = uniform_distribution(generator);
      const auto it   = hashmap.find(value);
      if (it == hashmap.end())
      {
        hashmap.insert({value, {}});
      }
      else
      {
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
