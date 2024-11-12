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

#include <dro/oa-hashmap.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
  // 16 Initial Capacity
  dro::HashMap<int, int> hashmap(16);
  dro::HashSet<int> hashset(16);

  std::array<int, 10> arr = {0, 0, 0, 3, 3, 4, 4, 5, 9, 9};
  // Count num of elements in array
  for (auto& elem : arr) { hashmap[elem]++; }

  // Can also build in place
  hashmap.emplace(30, 2);
  hashmap.insert(std::make_pair(50, 3));

  for (const auto& elem : hashmap)
  {
    std::cout << "Key: " << elem.first << " Value: " << elem.second << "\n";
  }

  int key = 30;
  std::cout << "Lookup Value at " << key << ": " << hashmap.at(key) << "\n";

  // Adjust Load Factor
  hashmap.max_load_factor(0.4);

  // Erase element
  hashmap.erase(9);
  bool success = hashmap.contains(9);

  return 0;
}
