// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <algorithm>
#include <cassert>
#include <dro/oa-hashmap.hpp>
#include <iostream>

int setTest()
{

  // Iterators
  {
    dro::HashSet<int> hashset(10, 0);
    const auto& chashset = hashset;

    assert(hashset.begin() == hashset.end());
    assert(chashset.begin() == chashset.end());
    assert(hashset.cbegin() == hashset.cend());
    assert(hashset.cbegin() == chashset.begin());
    assert(hashset.cend() == chashset.end());

    assert(! (hashset.begin() != hashset.end()));
    assert(! (chashset.begin() != chashset.end()));
    assert(! (hashset.cbegin() != hashset.cend()));
    assert(! (hashset.cbegin() != chashset.begin()));
    assert(! (hashset.cend() != chashset.end()));

    for (int i = 1; i < 100; ++i) { hashset.insert(i); }

    int sum {};
    for (auto it : hashset) { sum += it; }
    assert(sum == 4950);
    assert(std::all_of(hashset.begin(), hashset.end(),
                       [](const auto& item) { return item > 0; }));
  }

  // Capacity
  {
    dro::HashSet<int> hashset(10, 0);
    const auto& chashset = hashset;
    assert(chashset.empty());
    assert(chashset.size() == 0);
    assert(chashset.max_size() > 0);
    hashset.insert(1);
    assert(! chashset.empty());
    assert(chashset.size() == 1);
  }

  // Modifiers
  {
    dro::HashSet<int> hashset(10, 0);
    hashset.insert(1);
    hashset.clear();
    assert(hashset.empty());
    assert(hashset.size() == 0);
    assert(hashset.begin() == hashset.end());
    assert(hashset.cbegin() == hashset.cend());
  }

  {
    dro::HashSet<int> hashset(10, 0);
    auto res = hashset.insert(1);
    assert(! hashset.empty());
    assert(hashset.size() == 1);
    assert(hashset.begin() != hashset.end());
    assert(hashset.cbegin() != hashset.cend());
    assert(res.first != hashset.end());
    assert(*(res.first) == 1);
    assert(res.second);
    auto res2 = hashset.insert(1);
    assert(hashset.size() == 1);
    assert(res2.first == res.first);
    assert(*(res2.first) == 1);
    assert(! res2.second);
  }

  {
    dro::HashSet<int> hashset(10, 0);
    auto res = hashset.emplace(1);
    assert(! hashset.empty());
    assert(hashset.size() == 1);
    assert(hashset.begin() != hashset.end());
    assert(hashset.cbegin() != hashset.cend());
    assert(res.first != hashset.end());
    assert(*(res.first) == 1);
    assert(res.second);
    auto res2 = hashset.emplace(1);
    assert(hashset.size() == 1);
    assert(res2.first == res.first);
    assert(*(res2.first) == 1);
    assert(! res2.second);
  }

  {
    dro::HashSet<int> hashset(10, 0);
    auto res = hashset.emplace(1);
    hashset.erase(res.first);
    assert(hashset.empty());
    assert(hashset.size() == 0);
    assert(hashset.begin() == hashset.end());
    assert(hashset.cbegin() == hashset.cend());
  }

  {
    dro::HashSet<int> hashset(10, 0);
    assert(hashset.erase(1) == 0);
    hashset.insert(1);
    assert(hashset.erase(1) == 1);
    assert(hashset.empty());
    assert(hashset.size() == 0);
    assert(hashset.begin() == hashset.end());
    assert(hashset.cbegin() == hashset.cend());
  }

  {
    dro::HashSet<int> hashset1(10, 0), hashset2(16, 0);
    hashset1.insert(1);
    hashset2.swap(hashset1);
    assert(hashset1.empty());
    assert(hashset1.size() == 0);
    assert(hashset2.size() == 1);
    assert(*(hashset2.find(1)) == 1);
    std::swap(hashset1, hashset2);
    assert(hashset1.size() == 1);
    assert(*(hashset1.find(1)) == 1);
    assert(hashset2.empty());
    assert(hashset2.size() == 0);
  }

  {
    dro::HashSet<int> hashset(10, 0);
    const auto& chashset = hashset;
    hashset.insert(1);
    assert(hashset.count(1) == 1);
    assert(hashset.count(2) == 0);
    assert(chashset.count(1) == 1);
    assert(chashset.count(2) == 0);
  }

  {
    dro::HashSet<int> hashset(10, 0);
    const auto& chashset = hashset;
    hashset.insert(1);
    {
      auto it = hashset.find(1);
      assert(it != hashset.end());
      assert(*it == 1);
      it = hashset.find(2);
      assert(it == hashset.end());
    }
    {
      auto it = chashset.find(1);
      assert(it != chashset.end());
      assert(*it == 1);
      it = chashset.find(2);
      assert(it == chashset.end());
    }
  }

  // Bucket interface
  {
    dro::HashSet<int> hashset(10, 0);
    const auto& chashset = hashset;
    assert(hashset.bucket_count() == 10);
    assert(chashset.bucket_count() == 10);
  }

  {
    dro::HashSet<int> hashset(10, 0);
    const auto& chashset = hashset;
    assert(hashset.max_bucket_count() > 0);
    assert(chashset.max_bucket_count() > 0);
  }

  // Hash policy
  {
    dro::HashSet<int> hashset(2, 0);
    const auto& chashset = hashset;
    hashset.emplace(1);
    hashset.emplace(2);
    assert(hashset.bucket_count() == 4);
    assert(chashset.bucket_count() == 4);
    hashset.rehash(2);
    assert(hashset.bucket_count() == 4);
    assert(chashset.bucket_count() == 4);
    hashset.rehash(10);
    assert(hashset.bucket_count() == 10);
    assert(chashset.bucket_count() == 10);
    hashset.reserve(2);
    assert(hashset.bucket_count() == 10);
    assert(chashset.bucket_count() == 10);
    hashset.reserve(10);
    assert(hashset.bucket_count() == 32);
    assert(chashset.bucket_count() == 32);
  }

  return 0;
}
