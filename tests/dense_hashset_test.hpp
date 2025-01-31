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

#include <algorithm>
#include <cassert>
#include <dro/dense_hashmap.hpp>

int setTest() {
  // Iterators
  {
    dro::dense_hashset<int> hashset(10);
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
    dro::dense_hashset<int> hashset(10);
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
    dro::dense_hashset<int> hashset(10);
    hashset.insert(1);
    hashset.clear();
    assert(hashset.empty());
    assert(hashset.size() == 0);
    assert(hashset.begin() == hashset.end());
    assert(hashset.cbegin() == hashset.cend());
  }

  {
    dro::dense_hashset<int> hashset(10);
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
    dro::dense_hashset<int> hashset(10);
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
    dro::dense_hashset<int> hashset(10);
    auto res = hashset.emplace(1);
    hashset.erase(res.first);
    assert(hashset.empty());
    assert(hashset.size() == 0);
    assert(hashset.begin() == hashset.end());
    assert(hashset.cbegin() == hashset.cend());
  }

  {
    dro::dense_hashset<int> hashset(10);
    assert(hashset.erase(1) == 0);
    hashset.insert(1);
    assert(hashset.erase(1) == 1);
    assert(hashset.empty());
    assert(hashset.size() == 0);
    assert(hashset.begin() == hashset.end());
    assert(hashset.cbegin() == hashset.cend());
  }

  {
    dro::dense_hashset<int> hashset1(10), hashset2(16);
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
    dro::dense_hashset<int> hashset(10);
    const auto& chashset = hashset;
    hashset.insert(1);
    assert(hashset.count(1) == 1);
    assert(hashset.count(2) == 0);
    assert(chashset.count(1) == 1);
    assert(chashset.count(2) == 0);
  }

  {
    dro::dense_hashset<int> hashset(10);
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
    const int size = 10;
    dro::dense_hashset<int> hashset(size);
    const auto& chashset = hashset;
    assert(hashset.bucket_count() == size);
    assert(chashset.bucket_count() == size);
  }

  {
    dro::dense_hashset<int> hashset(10);
    const auto& chashset = hashset;
    assert(hashset.max_bucket_count() > 0);
    assert(chashset.max_bucket_count() > 0);
  }

  // Hash policy
  {
    dro::dense_hashset<int> hashset(2);
    const auto& chashset = hashset;
    auto load_factor     = hashset.max_load_factor();
    // double mult          = 1.0 / load_factor;
    // hashset.emplace(1);
    // hashset.emplace(2);
    // int newCount = static_cast<double>(hashset.size()) * mult;
    // assert(hashset.bucket_count() == newCount);
    // assert(chashset.bucket_count() == newCount);
    // hashset.rehash(2);
    // assert(hashset.bucket_count() == newCount);
    // assert(chashset.bucket_count() == newCount);
    // hashset.rehash(10);
    // assert(hashset.bucket_count() == 10);
    // assert(chashset.bucket_count() == 10);
    // hashset.reserve(2);
    // assert(hashset.bucket_count() == 10);
    // assert(chashset.bucket_count() == 10);
    // hashset.reserve(10);
    // assert(hashset.bucket_count() == 24);
    // assert(chashset.bucket_count() == 24);
  }

  return 0;
}
