// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <dro/oa-hashmap.hpp>
#include <iostream>
#include <limits>

int main(int argc, char* argv[])
{
  // 16 Initial Capacity and empty_key = 0
  dro::HashMap<int, int> hashmap(std::numeric_limits<int>::max(), 16);
  dro::HashSet<int> hashset(std::numeric_limits<int>::max(), 16);

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
