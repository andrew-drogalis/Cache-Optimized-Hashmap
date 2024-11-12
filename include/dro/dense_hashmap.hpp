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

#ifndef DRO_DENSE_HASHMAP_HPP
#define DRO_DENSE_HASHMAP_HPP

#include <concepts>   // for requires
#include <cstddef>    // for size_t, ptrdiff_t
#include <cstdint>    // for uint64_t
#include <functional> // for equal_to, hash
#include <iterator>   // for pair, forward_iterator_tag
#include <stdexcept>  // for out_of_range, invalid_argument
#include <type_traits>// for std::is_default_constructible
#include <utility>    // for pair, forward, make_pair
#include <vector>     // for allocator, vector

namespace dro {
namespace detail {

template <typename T>
concept densehash_t =
    std::is_default_constructible_v<T> &&
    (std::is_move_assignable_v<T> || std::is_copy_assignable_v<T>);

template <typename T, typename... Args>
concept densehash_nothrow =
    std::is_nothrow_constructible_v<T, Args&&...> &&
    ((std::is_nothrow_copy_assignable_v<T> && std::is_copy_assignable_v<T>) ||
     (std::is_nothrow_move_assignable_v<T> && std::is_move_assignable_v<T>));

struct IsAHashSet {};

template <densehash_t Key> struct hashset_pair {
  Key first;
  IsAHashSet second [[no_unique_address]];
};

template <typename Pair> struct hashnode {
  Pair pair_;
  // Least significant bit: 1 - Full, 0 - Empty
  uint64_t fingerprint_full_ {};
  uint64_t next_ {};// Index of next element in collision chain
};

template <typename Container> struct dense_iterator {
  using key_type        = typename Container::key_type;
  using mapped_type     = typename Container::mapped_type;
  using value_type      = typename Container::value_type;
  using size_type       = typename Container::size_type;
  using key_equal       = typename Container::key_equal;
  using difference_type = typename Container::difference_type;
  using reference = std::conditional_t<std::is_same_v<mapped_type, IsAHashSet>,
                                       key_type&, value_type&>;
  using pointer   = std::conditional_t<std::is_same_v<mapped_type, IsAHashSet>,
                                       key_type*, value_type*>;
  using const_reference =
      std::conditional_t<std::is_same_v<mapped_type, IsAHashSet>,
                         const key_type&, const value_type&>;
  using const_pointer =
      std::conditional_t<std::is_same_v<mapped_type, IsAHashSet>,
                         const key_type*, const value_type*>;
  using iterator_category = std::forward_iterator_tag;

  explicit dense_iterator(Container* hashbase, size_type index)
      : hashbase_(hashbase), index_(index) {
    _nextValidIndex();
  }

  bool operator==(const dense_iterator& other) const {
    return other.hashbase_ == hashbase_ && other.index_ == index_;
  }

  bool operator!=(const dense_iterator& other) const {
    return ! (*this == other);
  }

  dense_iterator& operator++() {
    if (index_ < hashbase_->capacity_) {
      ++index_;
      _nextValidIndex();
    }
    return *this;
  }

  reference operator*()
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             ! std::is_const_v<Container>)
  {
    return hashbase_->buckets_[index_].pair_.first;
  }

  const_reference operator*() const
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_const_v<Container>)
  {
    return hashbase_->buckets_[index_].pair_.first;
  }

  pointer operator->()
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             ! std::is_const_v<Container>)
  {
    return &hashbase_->buckets_[index_].pair_.first;
  }

  const_pointer operator->() const
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_const_v<Container>)
  {
    return &hashbase_->buckets_[index_].pair_.first;
  }

  reference operator*()
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             ! std::is_const_v<Container>)
  {
    return hashbase_->buckets_[index_].pair_;
  }

  const_reference operator*() const
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_const_v<Container>)
  {
    return hashbase_->buckets_[index_].pair_;
  }

  pointer operator->()
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             ! std::is_const_v<Container>)
  {
    return &hashbase_->buckets_[index_].pair_;
  }

  const_pointer operator->() const
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_const_v<Container>)
  {
    return &hashbase_->buckets_[index_].pair_;
  }

  void _nextValidIndex() {
    const auto& buckets = hashbase_->buckets_;
    while (index_ < hashbase_->capacity_ &&
           ! hashbase_->_getFull(buckets[index_].fingerprint_full_)) {
      ++index_;
    }
  }

private:
  Container* hashbase_ = nullptr;
  size_type index_ {};
  friend Container;
};

template <densehash_t Key, densehash_t Value, typename Pair,
          typename Hash      = std::hash<Key>,
          typename KeyEqual  = std::equal_to<Key>,
          typename Allocator = std::allocator<hashnode<Pair>>>
class dense_hashbase {
public:
  using key_type        = Key;
  using mapped_type     = Value;
  using value_type      = Pair;
  using size_type       = std::size_t;
  using hasher          = Hash;
  using key_equal       = KeyEqual;
  using allocator_type  = Allocator;
  using difference_type = std::ptrdiff_t;
  using node            = hashnode<Pair>;
  using buckets         = std::vector<node, Allocator>;
  using iterator        = dense_iterator<dense_hashbase>;
  using const_iterator  = dense_iterator<const dense_hashbase>;

private:
  static constexpr bool densehash_nothrow_v = densehash_nothrow<key_type> &&
                                              densehash_nothrow<mapped_type> &&
                                              densehash_nothrow<value_type>;
  float load_factor_                     = 1.0F;
  static constexpr float hashable_ratio_ = 0.7F;
  buckets buckets_;
  size_type size_ {};
  size_type collision_head_ {};
  size_type collision_tail_ {};
  hasher hash_ {};
  key_equal equal_ {};
  size_type capacity_ {};
  size_type hashable_capacity_ {};

  friend iterator;
  friend const_iterator;

public:
  explicit dense_hashbase(size_type capacity,
                          const Allocator& allocator = Allocator())
      : capacity_(capacity), buckets_(capacity, allocator) {
    if (capacity < 1) {
      throw std::invalid_argument("Capacity must be positive number.");
    }
    hashable_capacity_ = static_cast<float>(capacity_) * hashable_ratio_;
    collision_head_    = hashable_capacity_;
    collision_tail_    = hashable_capacity_;
  }

  // Member Functions
  [[nodiscard]] allocator_type get_allocator() const noexcept {
    return buckets_.get_allocator();
  }

  // Iterators //////////////

  iterator begin() noexcept { return iterator(this, 0); }

  const_iterator begin() const noexcept { return const_iterator(this, 0); }

  const_iterator cbegin() const noexcept { return const_iterator(this, 0); }

  iterator end() noexcept { return iterator(this, capacity_); }

  const_iterator end() const noexcept {
    return const_iterator(this, capacity_);
  }

  const_iterator cend() const noexcept {
    return const_iterator(this, capacity_);
  }

  // Capacity //////////////

  [[nodiscard]] bool empty() const noexcept { return ! size(); }

  [[nodiscard]] size_type size() const noexcept { return size_; }

  [[nodiscard]] size_type max_size() const noexcept {
    return buckets_.max_size();
  }

  // Modifiers //////////////

  void clear() noexcept {
    for (auto& bucket : buckets_) {
      _setEmpty(bucket.fingerprint_full_);
      bucket.next_ = 0;
    }
    size_           = 0;
    collision_head_ = hashable_capacity_;
    collision_tail_ = hashable_capacity_;
  }

  std::pair<iterator, bool>
  insert(const value_type& pair) noexcept(densehash_nothrow_v) {
    return _emplace(pair.first, pair.second);
  }

  std::pair<iterator, bool>
  insert(value_type&& pair) noexcept(densehash_nothrow_v) {
    return _emplace(std::move(pair.first), std::move(pair.second));
  }

  std::pair<iterator, bool>
  insert(const key_type& key) noexcept(densehash_nothrow_v)
    requires(std::is_same_v<mapped_type, IsAHashSet>)
  {
    return _emplace(key);
  }

  std::pair<iterator, bool> insert(key_type&& key) noexcept(densehash_nothrow_v)
    requires(std::is_same_v<mapped_type, IsAHashSet>)
  {
    return _emplace(std::move(key));
  }

  // template <typename P>
  //   requires std::is_constructible_v<value_type, P&&>
  // std::pair<iterator, bool> insert(P&& value) {
  //   return _emplace(std::forward<P>(value));
  // }

  template <class InputIt> void insert(InputIt first, InputIt last) {
    while (first != last) {
      insert(*first);
      ++first;
    }
  }

  void insert(std::initializer_list<value_type> ilist) {
    insert(ilist.begin(), ilist.end());
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return _emplace(std::forward<Args>(args)...);
  }

  iterator erase(iterator it) {
    if (_getFull(buckets_[it.index_].fingerprint_full_)) {
      _erase(buckets_[it.index_].pair_.first);
    }
    return iterator(this, it.index_);
  }

  iterator erase(const_iterator it) {
    if (_getFull(buckets_[it.index_].fingerprint_full_)) {
      _erase(buckets_[it.index_].pair_.first);
    }
    return iterator(this, it.index_);
  }

  iterator erase(const_iterator itStart, const_iterator itEnd) {
    for (; itStart != itEnd && itStart != end(); ++itStart) {
      if (_isFull(buckets_[itStart.index_].fingerprint_full_)) {
        _erase(buckets_[itStart.index_].pair_.first);
      }
    }
    return iterator(this, itStart.index_);
  }

  size_type erase(const key_type& key) { return _erase(key); }

  template <typename K>
    requires std::is_convertible_v<K, key_type>
  size_type erase(K&& x) {
    return _erase(x);
  }

  void swap(dense_hashbase& other) noexcept {
    // std::swap(buckets_, other.buckets_);
    // std::swap(size_, other.size_);
    // std::swap(load_factor_, other.load_factor_);
    // std::swap(collision_head_, other.collision_head_);
    // std::swap(collision_tail_, other.collision_tail_);
    // std::swap(capacity_, other.capacity_);
    // std::swap(hashable_capacity_, other.hashable_capacity_);
    std::swap(*this, other);
  }

  void merge(const dense_hashbase& other) {
    for (auto& elem : other) { _emplace(elem); }
  }

  // Lookup //////////////

  mapped_type& at(const key_type& key)
    requires(! std::is_same_v<mapped_type, IsAHashSet>)
  {
    size_type index = _find(key).first;
    if (index != capacity_) {
      return buckets_[index].pair_.second;
    }
    throw std::out_of_range("dro::dense_hashbase::at");
  }

  const mapped_type& at(const key_type& key) const
    requires(! std::is_same_v<mapped_type, IsAHashSet>)
  {
    size_type index = _find(key).first;
    if (index != capacity_) {
      return buckets_[index].pair_.second;
    }
    throw std::out_of_range("dro::dense_hashbase::at");
  }

  template <typename K>
  mapped_type& at(const K& x)
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_convertible_v<K, key_type>)
  {
    size_type index = _find(x).first;
    if (index != capacity_) {
      return buckets_[index].pair_.second;
    }
    throw std::out_of_range("dro::dense_hashbase::at");
  }

  template <typename K>
  const mapped_type& at(const K& x) const
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_convertible_v<K, key_type>)
  {
    size_type index = _find(x).first;
    if (index != capacity_) {
      return buckets_[index].pair_.second;
    }
    throw std::out_of_range("dro::dense_hashbase::at");
  }

  mapped_type& operator[](const key_type& key)
    requires(! std::is_same_v<mapped_type, IsAHashSet>)
  {
    size_type index = _emplace(key).first.index_;
    return buckets_[index].pair_.second;
  }

  mapped_type& operator[](key_type&& key)
    requires(! std::is_same_v<mapped_type, IsAHashSet>)
  {
    size_type index = _emplace(key).first.index_;
    return buckets_[index].pair_.second;
  }

  template <typename K>
  mapped_type& operator[](K&& x)
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_convertible_v<K, key_type>)
  {
    size_type index = _emplace(x).first.index_;
    return buckets_[index].pair_.second;
  }

  size_type count(const key_type& key) const {
    return (_find(key).first != capacity_);
  }

  template <typename K>
  size_type count(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return (_find(x).first != capacity_);
  }

  iterator find(const key_type& key) {
    return iterator(this, _find(key).first);
  }

  template <typename K>
  iterator find(const K& key)
    requires std::is_convertible_v<K, key_type>
  {
    return iterator(this, _find(key).first);
  }

  const_iterator find(const key_type& key) const {
    return const_iterator(this, _find(key).first);
  }

  template <typename K>
  const_iterator find(const K& key) const
    requires std::is_convertible_v<K, key_type>
  {
    return const_iterator(this, _find(key).first);
  }

  bool contains(const key_type& key) { return count(key); }

  template <typename K>
  bool contains(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return count(x);
  }

  std::pair<iterator, iterator> equal_range(const key_type& key) {
    auto first  = find(key);
    auto second = first + 1;
    return {first, second};
  }

  std::pair<const_iterator, const_iterator>
  equal_range(const key_type& key) const {
    auto first  = find(key);
    auto second = first + 1;
    return {first, second};
  }

  template <typename K>
    requires std::is_convertible_v<K, key_type>
  std::pair<iterator, iterator> equal_range(const K& x) {
    auto first  = find(x);
    auto second = first + 1;
    return {first, second};
  }

  template <typename K>
    requires std::is_convertible_v<K, key_type>
  std::pair<const_iterator, const_iterator> equal_range(const K& x) const {
    auto first  = find(x);
    auto second = first + 1;
    return {first, second};
  }

  // Bucket Interface ///////

  [[nodiscard]] size_type bucket_count() const noexcept { return capacity_; }

  [[nodiscard]] size_type max_bucket_count() const {
    return buckets_.max_size();
  }

  // Hash Policy ///////

  [[nodiscard]] float load_factor() const {
    return static_cast<float>(size()) / static_cast<float>(max_bucket_count());
  }

  void max_load_factor(float load_factor) {
    load_factor_ = load_factor;
    reserve(size_);
  }

  [[nodiscard]] float max_load_factor() const { return load_factor_; }

  void rehash(size_type count) {
    size_type min_size = static_cast<double>(size_) / load_factor_;
    count              = (count > min_size) ? count : min_size;
    _rehash(count);
  }

  void reserve(size_type count) {
    size_type max_capacity = static_cast<float>(capacity_) * load_factor_;
    if (count > max_capacity) {
      rehash(count);
    }
  }

  // Observers ///////

  [[nodiscard]] hasher hash_function() const { return hash_; }

  [[nodiscard]] key_equal key_eq() const { return equal_; }

private:
  template <typename... Args>
  std::pair<iterator, bool> _emplace(Args&&... args) {
    key_type key {};
    _buildKey(key, std::forward<Args>(args)...);
    uint64_t keyHash       = _hash(key);
    uint64_t fingerprint   = _getFingerprint(keyHash);
    size_type insert_index = _index_from_hash(keyHash);

    if (_getFull(buckets_[insert_index].fingerprint_full_)) {
      // Check the collision chain
      size_type chainNode {insert_index};
      do {
        auto& bucket = buckets_[chainNode];
        if (fingerprint == _getFingerprint(bucket.fingerprint_full_) &&
            equal_(bucket.pair_.first, key)) {
          return std::make_pair(iterator(this, insert_index), false);
        }
        chainNode = bucket.next_;
      } while (chainNode);
      // Not in the hashmap, will 100% insert.
      if (! _validateLoadFactorNotExceeded()) {
        return _emplace(std::forward<Args>(args)...);
      }
      // Insert at the end of the collisions vector
      if (collision_tail_ == collision_head_) {
        if (! _validateCollisionHasSpace()) {
          return _emplace(std::forward<Args>(args)...);
        }
        insert_index = collision_head_;
        ++collision_head_;
        ++collision_tail_;
      }
      // Fill in the empty slots
      else {
        insert_index = buckets_[collision_head_].next_;
        if (insert_index == collision_tail_) {
          collision_tail_ = collision_head_;
        } else {
          buckets_[collision_head_].next_ = buckets_[insert_index].next_;
        }
      }
      // Update chain
      buckets_[chainNode].next_ = insert_index;
    } else if (! _validateLoadFactorNotExceeded()) {
      return _emplace(std::forward<Args>(args)...);
    }
    // Insert
    _emplaceKey(insert_index, key);
    _emplaceValue(insert_index, std::forward<Args>(args)...);
    _setFingerprint(buckets_[insert_index].fingerprint_full_, keyHash);
    buckets_[insert_index].next_ = 0;
    ++size_;
    return std::make_pair(iterator(this, insert_index), true);
  }

  template <typename First, typename... Args>
  void _buildKey(key_type& key, First&& first, Args&&... args)
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_move_assignable_v<key_type>)
  {
    key = first;
  }

  template <typename First, typename... Args>
  void _buildKey(key_type& key, First&& first, Args&&... args)
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             ! std::is_move_assignable_v<key_type> &&
             std::is_copy_assignable_v<key_type>)
  {
    key_type non_movable(first);
    key = non_movable;
  }

  template <typename... Args>
  void _buildKey(key_type& key, Args&&... args)
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_constructible_v<key_type, Args && ...> &&
             std::is_move_assignable_v<key_type>)
  {
    key = key_type(std::forward<Args>(args)...);
  }

  template <typename... Args>
  void _buildKey(key_type& key, Args&&... args)
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_constructible_v<key_type, Args && ...> &&
             std::is_copy_assignable_v<key_type> &&
             ! std::is_move_assignable_v<key_type>)
  {
    key_type non_movable(std::forward<Args>(args)...);
    key = non_movable;
  }

  void _emplaceKey(const size_type& insert_index,
                   key_type& key) noexcept(densehash_nothrow_v)
    requires(std::is_move_assignable_v<key_type>)
  {
    buckets_[insert_index].pair_.first = std::move(key);
  }

  void _emplaceKey(const size_type& insert_index,
                   key_type& key) noexcept(densehash_nothrow_v)

    requires(std::is_copy_assignable_v<key_type> &&
             ! std::is_move_assignable_v<key_type>)
  {
    buckets_[insert_index].pair_.first = key;
  }

  template <typename... Args>
  void _emplaceValue(const size_type& insert_index, Args&&... args) noexcept

    requires(std::is_same_v<mapped_type, IsAHashSet>)
  {}// Intentionally blank for hashset

  template <typename First, typename... Args>
  void _emplaceValue(const size_type& insert_index, First&&,
                     Args&&... args) noexcept(densehash_nothrow_v)
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_move_assignable_v<mapped_type> &&
             std::is_constructible_v<mapped_type, Args && ...>)
  {
    buckets_[insert_index].pair_.second =
        mapped_type(std::forward<Args>(args)...);
  }

  template <typename First, typename... Args>
  void _emplaceValue(const size_type& insert_index, First&&,
                     Args&&... args) noexcept(densehash_nothrow_v)
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_copy_assignable_v<mapped_type> &&
             ! std::is_move_assignable_v<mapped_type> &&
             std::is_constructible_v<mapped_type, Args && ...>)
  {
    mapped_type non_movable(std::forward<Args>(args)...);
    buckets_[insert_index].pair_.second = non_movable;
  }

  size_type _erase(const key_type& key) {
    auto findLocation     = _find(key);
    size_type erase_index = findLocation.first;
    if (erase_index == capacity_) {
      return 0U;
    }
    auto& bucket   = buckets_[erase_index];
    size_type next = bucket.next_;
    if (erase_index < hashable_capacity_) {
      if (next == 0) {
        _setEmpty(bucket.fingerprint_full_);
        --size_;
        return 1U;
      }
      std::swap(bucket, buckets_[next]);
      erase_index = next;
    } else {
      size_type prev_index       = findLocation.second;
      buckets_[prev_index].next_ = next;
    }
    _setEmpty(buckets_[erase_index].fingerprint_full_);
    buckets_[erase_index].next_ = 0;
    --size_;
    // Add to collision linked list
    buckets_[collision_tail_].next_ = erase_index;
    collision_tail_                 = erase_index;
    return 1U;
  }

  template <typename K>
  std::pair<size_type, size_type> _find(const K& key) const
    requires std::is_convertible_v<K, key_type>
  {
    uint64_t keyHash     = _hash(key);
    uint64_t fingerprint = _getFingerprint(keyHash);
    // This is to avoid needing a doubly linked list
    size_type prev_index {capacity_};

    for (size_type index = _index_from_hash(keyHash);;) {
      auto& bucket                   = buckets_[index];
      const auto& bucket_fingerprint = bucket.fingerprint_full_;
      if (_getFull(bucket_fingerprint) &&
          fingerprint == _getFingerprint(bucket_fingerprint) &&
          equal_(bucket.pair_.first, key)) {
        return {index, prev_index};
      }
      if (index == 0) {
        break;
      }
      prev_index = index;
      index      = bucket.next_;
    }
    return {capacity_, prev_index};
  }

  [[nodiscard]] bool _validateLoadFactorNotExceeded() {
    size_type max_capacity = static_cast<float>(capacity_) * load_factor_;
    if (size_ + 1 > max_capacity) {
      _rehash(capacity_ * 2);
      return false;
    }
    return true;
  }

  [[nodiscard]] bool _validateCollisionHasSpace() {
    if (collision_head_ == capacity_) {
      _rehash(capacity_ * 2);
      return false;
    }
    return true;
  }

  void _rehash(const size_type& count) {
    dense_hashbase other(count, get_allocator());
    for (auto it = begin(); it != end(); ++it) { other.insert(*it); }
    swap(other);
  }

  [[nodiscard]] size_type _index_from_hash(const uint64_t& hash) const {
    uint64_t mask   = ~0UL >> __builtin_clzl(hashable_capacity_);
    size_type index = hash & mask;
    return (index >= hashable_capacity_) ? index - hashable_capacity_ : index;
  }

  template <typename K>
  [[nodiscard]] uint64_t _hash(const K& key) const
      noexcept(noexcept(Hash()(key)))
    requires std::is_convertible_v<K, key_type>
  {
    return hash_(key);
  }

  void _setFingerprint(uint64_t& fingerprint_full,
                       const uint64_t& hash) noexcept {
    fingerprint_full = hash;
    _setFull(fingerprint_full);
  }

  [[nodiscard]] uint64_t
  _getFingerprint(const uint64_t& fingerprint_full) const noexcept {
    return fingerprint_full >> 1;
  }

  void _setEmpty(uint64_t& fingerprint_full) noexcept {
    uint64_t mask = ~1;
    fingerprint_full &= mask;
  }

  void _setFull(uint64_t& fingerprint_full) noexcept { fingerprint_full |= 1; }

  [[nodiscard]] bool _getFull(const uint64_t& fingerprint_full) const noexcept {
    return fingerprint_full & 1;
  }
};
}// namespace detail

template <detail::densehash_t Key, detail::densehash_t Value,
          typename Hash     = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator =
              std::allocator<detail::hashnode<std::pair<Key, Value>>>>
class dense_hashmap
    : public detail::dense_hashbase<Key, Value, std::pair<Key, Value>, Hash,
                                    KeyEqual, Allocator> {
  using size_type = std::size_t;
  using base_type = detail::dense_hashbase<Key, Value, std::pair<Key, Value>,
                                           Hash, KeyEqual, Allocator>;

public:
  explicit dense_hashmap(size_type capacity         = 2,
                         const Allocator& allocator = Allocator())
      : base_type(capacity, allocator) {}
};

template <detail::densehash_t Key, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator =
              std::allocator<detail::hashnode<detail::hashset_pair<Key>>>>
class dense_hashset : public detail::dense_hashbase<Key, detail::IsAHashSet,
                                                    detail::hashset_pair<Key>,
                                                    Hash, KeyEqual, Allocator> {
  using size_type = std::size_t;
  using base_type =
      detail::dense_hashbase<Key, detail::IsAHashSet, detail::hashset_pair<Key>,
                             Hash, KeyEqual, Allocator>;

public:
  explicit dense_hashset(size_type capacity         = 2,
                         const Allocator& allocator = Allocator())
      : base_type(capacity, allocator) {}
};

}// namespace dro
#endif
