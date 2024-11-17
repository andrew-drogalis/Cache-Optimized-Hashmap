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

#include "catch2/catch_all.hpp"
#include "dro/dense_hashmap.hpp"

#include <string>
#include <utility>

TEST_CASE("collision functional test") {
  const int size = 20;
  dro::dense_hashmap<int, int> int_map {size};
  dro::dense_hashmap<std::string, int> str_map {size};
  dro::dense_hashset<int, int> int_set {size};
  dro::dense_hashset<std::string, int> str_set {size};

  REQUIRE(*(int_map.emplace(5, 0).first) == std::make_pair(5, 0));
  REQUIRE(*(int_map.emplace(21, 0).first) == std::make_pair(21, 0));
  REQUIRE(*(int_map.emplace(37, 0).first) == std::make_pair(37, 0));
  REQUIRE(*(int_map.emplace(53, 0).first) == std::make_pair(53, 0));
  REQUIRE(*(int_map.emplace(69, 0).first) == std::make_pair(69, 0));

  REQUIRE(int_map.find(5) != int_map.end());
  REQUIRE(int_map.find(21) != int_map.end());
  REQUIRE(int_map.find(37) != int_map.end());
  REQUIRE(int_map.find(53) != int_map.end());
  REQUIRE(int_map.find(69) != int_map.end());

  REQUIRE(*(int_map.find(5)) == std::make_pair(5, 0));
  REQUIRE(*(int_map.find(21)) == std::make_pair(21, 0));
  REQUIRE(*(int_map.find(37)) == std::make_pair(37, 0));
  REQUIRE(*(int_map.find(53)) == std::make_pair(53, 0));
  REQUIRE(*(int_map.find(69)) == std::make_pair(69, 0));

  REQUIRE(int_map.emplace(5, 0).second == false);
  REQUIRE(int_map.erase(21) == 1);

  REQUIRE(int_map.find(5) != int_map.end());
  REQUIRE(int_map.find(21) == int_map.end());
  REQUIRE(int_map.find(37) != int_map.end());
  REQUIRE(int_map.find(53) != int_map.end());
  REQUIRE(int_map.find(69) != int_map.end());

  REQUIRE(*(int_map.find(5)) == std::make_pair(5, 0));
  REQUIRE(*(int_map.find(37)) == std::make_pair(37, 0));
  REQUIRE(*(int_map.find(53)) == std::make_pair(53, 0));
  REQUIRE(*(int_map.find(69)) == std::make_pair(69, 0));

  REQUIRE(int_map.erase(37) == 1);

  REQUIRE(int_map.find(5) != int_map.end());
  REQUIRE(int_map.find(21) == int_map.end());
  REQUIRE(int_map.find(37) == int_map.end());
  REQUIRE(int_map.find(53) != int_map.end());
  REQUIRE(int_map.find(69) != int_map.end());

  REQUIRE(*(int_map.find(5)) == std::make_pair(5, 0));
  REQUIRE(*(int_map.find(53)) == std::make_pair(53, 0));
  REQUIRE(*(int_map.find(69)) == std::make_pair(69, 0));

  REQUIRE(int_map.erase(53) == 1);

  REQUIRE(int_map.find(5) != int_map.end());
  REQUIRE(int_map.find(21) == int_map.end());
  REQUIRE(int_map.find(37) == int_map.end());
  REQUIRE(int_map.find(53) == int_map.end());
  REQUIRE(int_map.find(69) != int_map.end());

  REQUIRE(*(int_map.find(5)) == std::make_pair(5, 0));
  REQUIRE(*(int_map.find(69)) == std::make_pair(69, 0));

  int_map.emplace(53, 0);
  int_map.emplace(21, 0);
  int_map.emplace(37, 0);

  REQUIRE(int_map.find(5) != int_map.end());
  REQUIRE(int_map.find(21) != int_map.end());
  REQUIRE(int_map.find(37) != int_map.end());
  REQUIRE(int_map.find(53) != int_map.end());
  REQUIRE(int_map.find(69) != int_map.end());

  REQUIRE(*(int_map.find(5)) == std::make_pair(5, 0));
  REQUIRE(*(int_map.find(21)) == std::make_pair(21, 0));
  REQUIRE(*(int_map.find(37)) == std::make_pair(37, 0));
  REQUIRE(*(int_map.find(53)) == std::make_pair(53, 0));
  REQUIRE(*(int_map.find(69)) == std::make_pair(69, 0));
}

TEST_CASE("rehash functional test") {
  dro::dense_hashmap<int, int> int_map;
  dro::dense_hashmap<std::string, int> str_map;
  dro::dense_hashset<int, int> int_set;
  dro::dense_hashset<std::string, int> str_set;
}

TEST_CASE("find functional test") {
  dro::dense_hashmap<int, int> int_map;
  dro::dense_hashmap<std::string, int> str_map;
  dro::dense_hashset<int, int> int_set;
  dro::dense_hashset<std::string, int> str_set;
}
