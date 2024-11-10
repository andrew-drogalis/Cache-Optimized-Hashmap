// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "dro/oa-hashmap.hpp"
#include "hashset-test.hpp"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
  // HashSet
  setTest();

  // Constructors
  {
    dro::HashMap<int, int> hashmap(10);
    hashmap[1] = 1;
    dro::HashMap<int, int> hashmap2(hashmap);
    assert(! hashmap2.empty());
    assert(hashmap2.size() == 1);
    assert(hashmap2[1] == 1);
  }
  {
    dro::HashMap<int, int> hashmap(10);
    hashmap[1] = 1;
    dro::HashMap<int, int> hashmap3(std::move(hashmap));
    assert(! hashmap3.empty());
    assert(hashmap3.size() == 1);
    assert(hashmap3[1] == 1);
  }
  {
    dro::HashMap<int, int> hashmap(10);
    hashmap[1] = 1;
    dro::HashMap<int, int> hashmap4(10);
    hashmap4.operator=(hashmap);
    assert(! hashmap4.empty());
    assert(hashmap4.size() == 1);
    assert(hashmap4[1] == 1);
  }
  {
    dro::HashMap<int, int> hashmap(10);
    hashmap[1] = 1;
    dro::HashMap<int, int> hashmap5(10);
    hashmap5.operator=(std::move(hashmap));
    assert(! hashmap5.empty());
    assert(hashmap5.size() == 1);
    assert(hashmap5[1] == 1);
  }

  // Iterators
  {
    dro::HashMap<int, int> hashmap(10);
    const auto& chashmap = hashmap;

    assert(hashmap.begin() == hashmap.end());
    assert(chashmap.begin() == chashmap.end());
    assert(hashmap.cbegin() == hashmap.cend());
    assert(hashmap.cbegin() == chashmap.begin());
    assert(hashmap.cend() == chashmap.end());

    assert(! (hashmap.begin() != hashmap.end()));
    assert(! (chashmap.begin() != chashmap.end()));
    assert(! (hashmap.cbegin() != hashmap.cend()));
    assert(! (hashmap.cbegin() != chashmap.begin()));
    assert(! (hashmap.cend() != chashmap.end()));

    for (int i = 1; i < 100; ++i) { hashmap[i] = i; }

    int sum {};
    for (auto it : hashmap) { sum += it.first; }
    assert(sum == 4950);
    assert(std::all_of(hashmap.begin(), hashmap.end(),
                       [](const auto& item) { return item.second > 0; }));
  }

  // Capacity
  {
    dro::HashMap<int, int> hashmap(10);
    const auto& chashmap = hashmap;
    assert(chashmap.empty());
    assert(chashmap.size() == 0);
    assert(chashmap.max_size() > 0);
    hashmap[1] = 1;
    assert(! chashmap.empty());
    assert(chashmap.size() == 1);
  }

  // Modifiers
  {
    dro::HashMap<int, int> hashmap(10);
    hashmap[1] = 1;
    hashmap.clear();
    assert(hashmap.empty());
    assert(hashmap.size() == 0);
    assert(hashmap.begin() == hashmap.end());
    assert(hashmap.cbegin() == hashmap.cend());
  }

  {
    dro::HashMap<int, int> hashmap(10);
    auto res = hashmap.insert({1, 1});
    assert(! hashmap.empty());
    assert(hashmap.size() == 1);
    assert(hashmap.begin() != hashmap.end());
    assert(hashmap.cbegin() != hashmap.cend());
    assert(res.first != hashmap.end());
    assert(res.first->first == 1);
    assert(res.first->second == 1);
    assert(res.second);
    const auto value = std::make_pair(1, 2);
    auto res2        = hashmap.insert(value);
    assert(hashmap.size() == 1);
    assert(res2.first == res.first);
    assert(res2.first->first == 1);
    assert(res2.first->second == 1);
    assert(! res2.second);
  }

  {
    dro::HashMap<int, int> hashmap(10);
    auto res = hashmap.emplace(1, 1);
    assert(! hashmap.empty());
    assert(hashmap.size() == 1);
    assert(hashmap.begin() != hashmap.end());
    assert(hashmap.cbegin() != hashmap.cend());
    assert(res.first != hashmap.end());
    assert(res.first->first == 1);
    assert(res.first->second == 1);
    assert(res.second);
    auto res2 = hashmap.emplace(1, 2);
    assert(hashmap.size() == 1);
    assert(res2.first == res.first);
    assert(res2.first->first == 1);
    assert(res2.first->second == 1);
    assert(! res2.second);
  }

  {
    dro::HashMap<int, int> hashmap(10);
    auto res = hashmap.emplace(1, 1);
    hashmap.erase(res.first);
    assert(hashmap.empty());
    assert(hashmap.size() == 0);
    assert(hashmap.begin() == hashmap.end());
    assert(hashmap.cbegin() == hashmap.cend());
  }

  {
    dro::HashMap<int, int> hashmap(10);
    assert(hashmap.erase(1) == 0);
    hashmap[1] = 1;
    assert(hashmap.erase(1) == 1);
    assert(hashmap.empty());
    assert(hashmap.size() == 0);
    assert(hashmap.begin() == hashmap.end());
    assert(hashmap.cbegin() == hashmap.cend());
  }

  {
    // template <class K> erase(const K&)
    dro::HashMap<int, int> hashmap(10);
    assert(hashmap.erase(1) == 0);
    hashmap[1] = 1;
    assert(hashmap.erase(1) == 1);
    assert(hashmap.empty());
    assert(hashmap.size() == 0);
    assert(hashmap.begin() == hashmap.end());
    assert(hashmap.cbegin() == hashmap.cend());
  }

  {
    dro::HashMap<int, int> hashmap1(10), hashmap2(16);
    hashmap1[1] = 1;
    hashmap2.swap(hashmap1);
    assert(hashmap1.empty());
    assert(hashmap1.size() == 0);
    assert(hashmap2.size() == 1);
    assert(hashmap2[1] == 1);
    std::swap(hashmap1, hashmap2);
    assert(hashmap1.size() == 1);
    assert(hashmap1[1] == 1);
    assert(hashmap2.empty());
    assert(hashmap2.size() == 0);
  }

  // Lookup
  {
    dro::HashMap<int, int> hashmap(10);
    const auto& chashmap = hashmap;
    hashmap[1]           = 1;
    assert(hashmap.at(1) == 1);
    assert(chashmap.at(1) == 1);
    hashmap.at(1) = 2;
    assert(hashmap.at(1) == 2);
    assert(chashmap.at(1) == 2);
    try {
      hashmap.at(2);
      assert(false);// Should never reach
    } catch (std::out_of_range& e) {
      assert(true);// Should always reach;
    } catch (...) {
      assert(false);// Should never reach
    }
    try {
      chashmap.at(2);
      assert(false);// Should never reach
    } catch (std::out_of_range& e) {
      assert(true);// Should always reach;
    } catch (...) {
      assert(false);// Should never reach
    }
  }

  {
    dro::HashMap<int, int> hashmap(10);
    hashmap[1] = 1;
    assert(! hashmap.empty());
    assert(hashmap.size() == 1);
    assert(hashmap.begin() != hashmap.end());
    assert(hashmap.cbegin() != hashmap.cend());
    assert(hashmap[1] == 1);
  }

  {
    dro::HashMap<int, int> hashmap(10);
    const auto& chashmap = hashmap;
    hashmap[1]           = 1;
    assert(hashmap.count(1) == 1);
    assert(hashmap.count(2) == 0);
    assert(chashmap.count(1) == 1);
    assert(chashmap.count(2) == 0);
  }

  {
    dro::HashMap<int, int> hashmap(10);
    const auto& chashmap = hashmap;
    hashmap[1]           = 1;
    {
      auto it = hashmap.find(1);
      assert(it != hashmap.end());
      assert(it->first == 1);
      assert(it->second == 1);
      it = hashmap.find(2);
      assert(it == hashmap.end());
    }
    {
      auto it = chashmap.find(1);
      assert(it != chashmap.end());
      assert(it->first == 1);
      assert(it->second == 1);
      it = chashmap.find(2);
      assert(it == chashmap.end());
    }
  }

  // Bucket interface
  {
    dro::HashMap<int, int> hashmap(10);
    const auto& chashmap = hashmap;
    assert(hashmap.bucket_count() == 10);
    assert(chashmap.bucket_count() == 10);
  }

  {
    dro::HashMap<int, int> hashmap(10);
    const auto& chashmap = hashmap;
    assert(hashmap.max_bucket_count() > 0);
    assert(chashmap.max_bucket_count() > 0);
  }

  // Hash policy
  {
    dro::HashMap<int, int> hashmap(2);
    const auto& chashmap = hashmap;
    auto load_factor     = hashmap.max_load_factor();
    double mult          = 1.0 / load_factor;
    hashmap.emplace(1, 1);
    hashmap.emplace(2, 2);
    int newCount = static_cast<double>(hashmap.size()) * mult;
    assert(hashmap.bucket_count() == newCount);
    assert(chashmap.bucket_count() == newCount);
    hashmap.rehash(2);
    assert(hashmap.bucket_count() == newCount);
    assert(chashmap.bucket_count() == newCount);
    hashmap.rehash(10);
    assert(hashmap.bucket_count() == 10);
    assert(chashmap.bucket_count() == 10);
    hashmap.reserve(2);
    assert(hashmap.bucket_count() == 10);
    assert(chashmap.bucket_count() == 10);
    hashmap.reserve(10);
    assert(hashmap.bucket_count() == 24);
    assert(chashmap.bucket_count() == 24);
  }

  std::cout << "Test Completed!\n";
  return 0;
}
