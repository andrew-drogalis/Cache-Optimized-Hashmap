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

#ifndef DRO_OA_HASHMAP
#define DRO_OA_HASHMAP

#include <concepts>   // for requires
#include <cstddef>    // for size_t, ptrdiff_t
#include <cstdint>    // for uint64_t
#include <functional> // for equal_to, hash
#include <iterator>   // for pair, forward_iterator_tag
#include <stdexcept>  // for out_of_range, invalid_argument
#include <string_view>// for hash
#include <type_traits>// for std::is_default_constructible
#include <utility>    // for pair, forward, make_pair
#include <vector>     // for allocator, vector

namespace dro {
namespace detail {
namespace hashmix {

// Improves the hash function for identity hash. i.e. std::hash, boost::hash
// Template parameter bool HashMix = true (default) sets this On or Off
// Only turn off if a high quality hash function is provided

inline void mum(uint64_t* a, uint64_t* b) {
#if defined(__SIZEOF_INT128__)
  __uint128_t r = *a;
  r *= *b;
  *a = static_cast<uint64_t>(r);
  *b = static_cast<uint64_t>(r >> 64U);
#elif defined(_MSC_VER) && defined(_M_X64)
  *a = _umul128(*a, *b, b);
#else
  uint64_t ha = *a >> 32U;
  uint64_t hb = *b >> 32U;
  uint64_t la = static_cast<uint32_t>(*a);
  uint64_t lb = static_cast<uint32_t>(*b);
  uint64_t hi {};
  uint64_t lo {};
  uint64_t rh  = ha * hb;
  uint64_t rm0 = ha * lb;
  uint64_t rm1 = hb * la;
  uint64_t rl  = la * lb;
  uint64_t t   = rl + (rm0 << 32U);
  auto c       = static_cast<uint64_t>(t < rl);
  lo           = t + (rm1 << 32U);
  c += static_cast<uint64_t>(lo < t);
  hi = rh + (rm0 >> 32U) + (rm1 >> 32U) + c;
  *a = lo;
  *b = hi;
#endif
}

[[nodiscard]] inline auto mix(uint64_t a, uint64_t b) -> uint64_t {
  mum(&a, &b);
  return a ^ b;
}

[[nodiscard]] inline auto hash(uint64_t x) -> uint64_t {
  return mix(x, UINT64_C(0x9E3779B97F4A7C15));
}
}// namespace hashmix

template <typename T>
concept Hashmap_Type =
    std::is_default_constructible_v<T> &&
    (std::is_move_assignable_v<T> || std::is_copy_assignable_v<T>);

template <typename T, typename... Args>
concept Hashmap_NoThrow =
    std::is_nothrow_constructible_v<T, Args&&...> &&
    ((std::is_nothrow_copy_assignable_v<T> && std::is_copy_assignable_v<T>) ||
     (std::is_nothrow_move_assignable_v<T> && std::is_move_assignable_v<T>));

struct IsAHashSet {};

template <Hashmap_Type Key> struct PairHashSet {
  Key first;
  IsAHashSet second [[no_unique_address]];
  PairHashSet() = default;
  explicit PairHashSet(Key key, IsAHashSet) : first(key) {}
};

template <typename Pair> struct HashNode {
  Pair pair_;
  // Least significant bit: 1 - Full, 0 - Empty
  uint64_t fingerprint_full_ {};
  uint64_t next_ {};// Index of next element in collision chain
  HashNode() = default;
  explicit HashNode(Pair pair) : pair_(pair) {}
};

template <typename Container> struct HashIterator {
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

  explicit HashIterator(Container* hashmap, size_type index)
      : hashmap_(hashmap), index_(index) {
    _nextValidIndex();
  }

  bool operator==(const HashIterator& other) const {
    return other.hashmap_ == hashmap_ && other.index_ == index_;
  }

  bool operator!=(const HashIterator& other) const {
    return ! (*this == other);
  }

  HashIterator& operator++() {
    if (index_ < hashmap_->buckets_.size()) {
      ++index_;
      _nextValidIndex();
    }
    return *this;
  }

  reference operator*()
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             ! std::is_const_v<Container>)
  {
    return hashmap_->buckets_[index_].pair_.first;
  }

  const_reference operator*() const
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_const_v<Container>)
  {
    return hashmap_->buckets_[index_].pair_.first;
  }

  pointer operator->()
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             ! std::is_const_v<Container>)
  {
    return &hashmap_->buckets_[index_].pair_.first;
  }

  const_pointer operator->() const
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_const_v<Container>)
  {
    return &hashmap_->buckets_[index_].pair_.first;
  }

  reference operator*()
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             ! std::is_const_v<Container>)
  {
    return hashmap_->buckets_[index_].pair_;
  }

  const_reference operator*() const
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_const_v<Container>)
  {
    return hashmap_->buckets_[index_].pair_;
  }

  pointer operator->()
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             ! std::is_const_v<Container>)
  {
    return &hashmap_->buckets_[index_].pair_;
  }

  const_pointer operator->() const
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_const_v<Container>)
  {
    return &hashmap_->buckets_[index_].pair_;
  }

  void _nextValidIndex() {
    const auto& buckets = hashmap_->buckets_;
    while (index_ < buckets.size() &&
           ! hashmap_->_getFull(buckets[index_].fingerprint_full_)) {
      ++index_;
    }
  }

private:
  Container* hashmap_ = nullptr;
  size_type index_ {};
  friend Container;
};

template <Hashmap_Type Key, Hashmap_Type Value, typename Pair,
          typename Hash = std::hash<Key>, bool HashMix = true,
          typename KeyEqual  = std::equal_to<Key>,
          typename Allocator = std::allocator<HashNode<Pair>>>
class OAHashmap {
public:
  using key_type        = Key;
  using mapped_type     = Value;
  using value_type      = Pair;
  using size_type       = std::size_t;
  using hasher          = Hash;
  using key_equal       = KeyEqual;
  using allocator_type  = Allocator;
  using difference_type = std::ptrdiff_t;
  using node            = HashNode<Pair>;
  using buckets         = std::vector<node, Allocator>;
  using iterator        = HashIterator<OAHashmap>;
  using const_iterator  = HashIterator<const OAHashmap>;

private:
  float load_factor_                     = 0.95F;
  static constexpr float hashable_ratio_ = 0.8F;
  buckets buckets_;
  size_type size_ {};
  size_type collision_head_ {};
  size_type collision_tail_ {};
  Hash hash_ {};
  size_type hashable_capacity_ {};
  friend iterator;
  friend const_iterator;

public:
  explicit OAHashmap(size_type count, const Allocator& allocator = Allocator())
      : buckets_(allocator) {
    if (count < 1) {
      throw std::invalid_argument("Count must be positive number.");
    }
    buckets_.resize(count);
    hashable_capacity_ = static_cast<float>(count) * hashable_ratio_;
    collision_head_    = hashable_capacity_;
    collision_tail_    = hashable_capacity_;
  }

  // No memory allocated, this is redundant
  ~OAHashmap()                           = default;
  OAHashmap(const OAHashmap& other)      = default;
  OAHashmap& operator=(const OAHashmap&) = default;
  OAHashmap(OAHashmap&& other)           = default;
  OAHashmap& operator=(OAHashmap&&)      = default;

  // Member Functions
  allocator_type get_allocator() const noexcept {
    return buckets_.get_allocator();
  }

  // Iterators
  iterator begin() noexcept { return iterator(this, 0); }

  const_iterator begin() const noexcept { return const_iterator(this, 0); }

  const_iterator cbegin() const noexcept { return const_iterator(this, 0); }

  iterator end() noexcept { return iterator(this, buckets_.size()); }

  const_iterator end() const noexcept {
    return const_iterator(this, buckets_.size());
  }

  const_iterator cend() const noexcept {
    return const_iterator(this, buckets_.size());
  }

  // Capacity
  [[nodiscard]] bool empty() const noexcept { return ! size(); }

  [[nodiscard]] size_type size() const noexcept { return size_; }

  [[nodiscard]] size_type max_size() const noexcept {
    return buckets_.max_size() / 2;
  }

  // Modifiers
  void clear() noexcept {
    for (auto& bucket : buckets_) {
      _setEmpty(bucket.fingerprint_full_);
      bucket.next_ = 0;
    }
    size_           = 0;
    collision_head_ = hashable_capacity_;
    collision_tail_ = hashable_capacity_;
  }

  std::pair<iterator, bool> insert(const value_type& pair) {
    return _emplace(pair.first, pair.second);
  }

  std::pair<iterator, bool> insert(value_type&& pair) {
    return _emplace(std::move(pair.first), std::move(pair.second));
  }

  std::pair<iterator, bool> insert(const key_type& key)
    requires(std::is_same_v<mapped_type, IsAHashSet>)
  {
    return _emplace(key);
  }

  std::pair<iterator, bool> insert(key_type&& key)
    requires(std::is_same_v<mapped_type, IsAHashSet>)
  {
    return _emplace(std::move(key));
  }

  template <typename P>
    requires std::is_convertible_v<P, value_type>
  std::pair<iterator, bool> insert(P&& value) {
    return _emplace(value.first, std::move(value.second));
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return _emplace(std::forward<Args>(args)...);
  }

  iterator erase(iterator it) {
    _erase(it.index_);
    return iterator(this, it.index_);
  }

  iterator erase(const_iterator it) {
    _erase(it.index_);
    return iterator(this, it.index_);
  }

  iterator erase(const_iterator itStart, const_iterator itEnd) {
    for (; itStart != itEnd; ++itStart) { _erase(itStart.index_); }
    return iterator(this, itStart.index_);
  }

  size_type erase(const key_type& key) {
    size_type index = _find(key);
    if (index != buckets_.size()) {
      _erase(index);
      return 1U;
    }
    return 0U;
  }

  template <typename K>
    requires std::is_convertible_v<K, key_type>
  size_type erase(K&& x) {
    size_type index = _find(x);
    if (index != buckets_.size()) {
      _erase(index);
      return 1U;
    }
    return 0U;
  }

  void swap(OAHashmap& other) noexcept {
    std::swap(buckets_, other.buckets_);
    std::swap(size_, other.size_);
    std::swap(load_factor_, other.load_factor_);
    std::swap(collision_head_, other.collision_head_);
    std::swap(collision_tail_, other.collision_tail_);
  }

  void merge(const OAHashmap& other) {
    for (auto& elem : other) { _emplace(elem); }
  }

  // Get Values
  mapped_type& at(const key_type& key)
    requires(! std::is_same_v<mapped_type, IsAHashSet>)
  {
    size_type index = _find(key);
    if (index != buckets_.size()) {
      return buckets_[index].pair_.second;
    }
    throw std::out_of_range("OAHashmap::at");
  }

  const mapped_type& at(const key_type& key) const
    requires(! std::is_same_v<mapped_type, IsAHashSet>)
  {
    size_type index = _find(key);
    if (index != buckets_.size()) {
      return buckets_[index].pair_.second;
    }
    throw std::out_of_range("OAHashmap::at");
  }

  template <typename K>
  mapped_type& at(const K& x)
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_convertible_v<K, key_type>)
  {
    size_type index = _find(x);
    if (index != buckets_.size()) {
      return buckets_[index].pair_.second;
    }
    throw std::out_of_range("OAHashmap::at");
  }

  template <typename K>
  const mapped_type& at(const K& x) const
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_convertible_v<K, key_type>)
  {
    size_type index = _find(x);
    if (index != buckets_.size()) {
      return buckets_[index].pair_.second;
    }
    throw std::out_of_range("OAHashmap::at");
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
    return (_find(key) != buckets_.size());
  }

  template <typename K>
  size_type count(const K& x) const
    requires std::is_convertible_v<K, key_type>
  {
    return (_find(x) != buckets_.size());
  }

  iterator find(const key_type& key) { return iterator(this, _find(key)); }

  template <typename K>
  iterator find(const K& key)
    requires std::is_convertible_v<K, key_type>
  {
    return iterator(this, _find(key));
  }

  const_iterator find(const key_type& key) const {
    return const_iterator(this, _find(key));
  }

  template <typename K>
  const_iterator find(const K& key) const
    requires std::is_convertible_v<K, key_type>
  {
    return const_iterator(this, _find(key));
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
    auto second = first;
    return {first, ++second};
  }

  std::pair<const_iterator, const_iterator>
  equal_range(const key_type& key) const {
    auto first  = find(key);
    auto second = first;
    return {first, ++second};
  }

  template <typename K>
    requires std::is_convertible_v<K, key_type>
  std::pair<iterator, iterator> equal_range(const K& x) {
    auto first  = find(x);
    auto second = first;
    return {first, ++second};
  }

  template <typename K>
    requires std::is_convertible_v<K, key_type>
  std::pair<const_iterator, const_iterator> equal_range(const K& x) const {
    auto first  = find(x);
    auto second = first;
    return {first, ++second};
  }

  // Bucket Interface
  [[nodiscard]] size_type bucket_count() const { return buckets_.size(); }

  [[nodiscard]] size_type max_bucket_count() const {
    return buckets_.max_size();
  }

  // Hash Policy
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
    size_type max_capacity = static_cast<float>(bucket_count()) * load_factor_;
    if (count > max_capacity) {
      rehash(count);
    }
  }

  // Observers
  [[nodiscard]] hasher hash_function() const { return hash_; }

  [[nodiscard]] key_equal key_eq() const { return key_equal(); }

private:
  template <typename... Args>
  std::pair<iterator, bool> _emplace(Args&&... args) {
    // Resize and Rehash
    reserve(size_ + 1);
    validateSize();

    key_type key {};
    _getKey(key, std::forward<Args>(args)...);
    uint64_t keyHash            = _hash(key);
    uint64_t fingerprint        = _getFingerprint(keyHash);
    size_type insert_index      = _index_from_hash(keyHash);
    auto& bucket                = buckets_[insert_index];
    auto& bucketFingerprintFull = bucket.fingerprint_full_;
    // Main table is empty, so insert
    if (! _getFull(bucketFingerprintFull)) {
      _buildPair(insert_index, key, std::forward<Args>(args)...);
      _setFingerprint(bucketFingerprintFull, keyHash);
      bucket.next_ = 0;
      ++size_;
      return std::make_pair(iterator(this, insert_index), true);
    }
    // Main table is full, check if match
    if (fingerprint == _getFingerprint(bucketFingerprintFull) &&
        key_equal()(bucket.pair_.first, key)) {
      return std::make_pair(iterator(this, insert_index), false);
    }
    // Check the collision chain
    size_type currNode {insert_index};
    size_type nextNode = bucket.next_;
    while (nextNode) {
      currNode     = nextNode;
      auto& bucket = buckets_[currNode];
      if (fingerprint == _getFingerprint(bucket.fingerprint_full_) &&
          key_equal()(bucket.pair_.first, key)) {
        return std::make_pair(iterator(this, insert_index), false);
      }
      nextNode = bucket.next_;
    }
    // Insert at the end of the collisions vector
    if (collision_tail_ == collision_head_) {
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
    buckets_[currNode].next_ = insert_index;
    // Insert
    _buildPair(insert_index, key, std::forward<Args>(args)...);
    _setFingerprint(buckets_[insert_index].fingerprint_full_, keyHash);
    buckets_[insert_index].next_ = 0;
    ++size_;
    return std::make_pair(iterator(this, insert_index), true);
  }

  template <std::size_t N, class... Ts> decltype(auto) _getNth(Ts&&... ts) {
    return std::get<N>(std::forward_as_tuple(ts...));
  }

  template <typename... Args>
  void _getKey(key_type& key, Args&&... args)
    requires(! std::is_same_v<mapped_type, IsAHashSet>)
  {
    key = _getNth<0>(args...);
  }

  template <typename... Args>
  void _getKey(key_type& key, Args&&... args)
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_constructible_v<key_type, Args && ...> &&
             std::is_move_assignable_v<key_type>)
  {
    key = key_type(std::forward<Args>(args)...);
  }

  template <typename... Args>
  void _getKey(key_type& key, Args&&... args)
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_constructible_v<key_type, Args && ...> &&
             std::is_copy_assignable_v<key_type> &&
             ! std::is_move_assignable_v<key_type>)
  {
    key_type non_movable = key_type(std::forward<Args>(args)...);
    key                  = non_movable;
  }

  template <typename... Args>
  void _buildKey(const size_type& insert_index, key_type& key, Args&&... args)
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_move_assignable_v<key_type>)
  {
    buckets_[insert_index].pair_.first = std::move(key);
  }

  template <typename... Args>
  void _buildKey(const size_type& insert_index, key_type& key, Args&&... args)
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_copy_assignable_v<key_type> &&
             ! std::is_move_assignable_v<key_type>)
  {
    buckets_[insert_index].pair_.first = key;
  }

  template <typename First, typename... Args>
  void _buildPair(const size_type& insert_index, key_type& key, First&&,
                  Args&&... args)
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_constructible_v<mapped_type, Args && ...>)
  {
    buckets_[insert_index].pair_.second =
        mapped_type(std::forward<Args>(args)...);
    buckets_[insert_index].pair_.first = key;
  }

  void _erase(size_type& erase_index) {
    if (erase_index < hashable_capacity_) {
      auto& bucket   = buckets_[erase_index];
      size_type next = bucket.next_;
      if (next == 0) {
        _setEmpty(bucket.fingerprint_full_);
        return;
      }
      std::swap(bucket, buckets_[next]);
      erase_index = next;
    }
    auto& bucket = buckets_[erase_index];
    _setEmpty(bucket.fingerprint_full_);
    --size_;
    // Add to collision linked list
    buckets_[collision_tail_].next_ = erase_index;
    collision_tail_                 = erase_index;
  }

  template <typename K>
  size_type _find(const K& key) const
    requires std::is_convertible_v<K, key_type>
  {
    uint64_t keyHash     = _hash(key);
    uint64_t fingerprint = _getFingerprint(keyHash);

    for (size_type index = _index_from_hash(keyHash);;) {
      auto& bucket                   = buckets_[index];
      const auto& bucket_fingerprint = bucket.fingerprint_full_;
      if (_getFull(bucket_fingerprint) &&
          fingerprint == _getFingerprint(bucket_fingerprint) &&
          key_equal()(bucket.pair_.first, key)) {
        return index;
      }
      // Set index,
      index = bucket.next_;
      if (index == 0) {
        break;
      }
    }
    return buckets_.size();
  }

  void validateSize() {
    size_type bucket_capacity = bucket_count();
    size_type max_capacity = static_cast<float>(bucket_capacity) * load_factor_;
    if (size_ > max_capacity || collision_head_ == bucket_capacity) {
      _rehash(bucket_capacity * 2);
    }
  }

  void _rehash(const size_type& count) {
    OAHashmap other(count, get_allocator());
    for (auto it = begin(); it != end(); ++it) { other.insert(*it); }
    swap(other);
  }

  [[nodiscard]] size_type _index_from_hash(const uint64_t& hash) const {
    return hash % buckets_.size();
  }

  template <typename K>
  [[nodiscard]] uint64_t _hash(const K& key) const
      noexcept(noexcept(Hash()(key)))
    requires std::is_convertible_v<K, key_type> && (! HashMix)
  {
    return hash_(key);
  }

  template <typename K>
  [[nodiscard]] uint64_t _hash(K const& key) const
      noexcept(noexcept(Hash()(key)))
    requires std::is_convertible_v<K, key_type> && HashMix
  {
    return hashmix::hash(hash_(key));
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

template <detail::Hashmap_Type Key, detail::Hashmap_Type Value,
          typename Hash = std::hash<Key>, bool HashMix = true,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator =
              std::allocator<detail::HashNode<std::pair<Key, Value>>>>
class HashMap : public detail::OAHashmap<Key, Value, std::pair<Key, Value>,
                                         Hash, HashMix, KeyEqual, Allocator> {
  using size_type = std::size_t;
  using base_type = detail::OAHashmap<Key, Value, std::pair<Key, Value>, Hash,
                                      HashMix, KeyEqual, Allocator>;

public:
  explicit HashMap(size_type count            = 1,
                   const Allocator& allocator = Allocator())
      : base_type(count, allocator) {}
};

template <detail::Hashmap_Type Key, typename Hash = std::hash<Key>,
          bool HashMix = true, typename KeyEqual = std::equal_to<Key>,
          typename Allocator =
              std::allocator<detail::HashNode<detail::PairHashSet<Key>>>>
class HashSet : public detail::OAHashmap<Key, detail::IsAHashSet,
                                         detail::PairHashSet<Key>, Hash,
                                         HashMix, KeyEqual, Allocator> {
  using size_type = std::size_t;
  using base_type =
      detail::OAHashmap<Key, detail::IsAHashSet, detail::PairHashSet<Key>, Hash,
                        HashMix, KeyEqual, Allocator>;

public:
  explicit HashSet(size_type count            = 1,
                   const Allocator& allocator = Allocator())
      : base_type(count, allocator) {}
};

}// namespace dro
#endif
