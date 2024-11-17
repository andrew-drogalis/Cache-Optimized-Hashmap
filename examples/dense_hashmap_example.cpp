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

#include <dro/dense_hashmap.hpp>
#include <string>

int main(int argc, char* argv[]) {
  // Initialize size in constructor
  dro::dense_hashmap<int, int> hashmap {20};
  dro::dense_hashset<std::string> hashset {16};

  // See README.md for all available methods.
  // Both Hashmap and Hashset compliant with 99% of STL methods

  // Example array
  std::array<int, 10> arr = {0, 0, 0, 3, 3, 4, 4, 5, 9, 9};

  // Count number of elements in the array
  for (const auto& elem : arr) { hashmap[elem]++; }

  // Build std::pair in place
  hashmap.emplace(30, 2);

  // Insert std::pair
  hashmap.insert(std::make_pair(50, 3));

  // Erase element
  hashmap.erase(9);

  // Check if element exists
  if (hashmap.contains(9)) {
    // true
  } else {
    // false
  }

  return 0;
}
