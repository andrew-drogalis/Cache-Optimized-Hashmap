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
#include <limits>     // for numeric_limits
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
    _next_valid_index();
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
      _next_valid_index();
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

  void _next_valid_index() {
    const auto& buckets = hashbase_->buckets_;
    while (index_ < hashbase_->capacity_ &&
           ! hashbase_->_get_full(buckets[index_].fingerprint_full_)) {
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

  using node           = hashnode<Pair>;
  using buckets        = std::vector<node, Allocator>;
  using iterator       = dense_iterator<dense_hashbase>;
  using const_iterator = dense_iterator<const dense_hashbase>;

private:
  static constexpr bool densehash_nothrow_v = densehash_nothrow<key_type> &&
                                              densehash_nothrow<mapped_type> &&
                                              densehash_nothrow<value_type>;
  static constexpr bool nothrow_swapable_v =
      std::is_nothrow_swappable_v<buckets> &&
      std::is_nothrow_swappable_v<Hash> &&
      std::is_nothrow_swappable_v<KeyEqual>;

  static constexpr float hashable_ratio_ = 0.82F;
  float load_factor_                     = 1.0F;
  float growth_multiple_                 = 2.0F;
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
      : capacity_(capacity), buckets_(allocator) {
    if (capacity < 1) {
      throw std::invalid_argument("Capacity must be positive number.");
    }
    if (capacity == std::numeric_limits<size_type>::max()) {
      throw std::overflow_error(
          "Capacity must be less than numeric_limits<size_type>::max().");
    }
    // Additional slot for last collision_head_
    buckets_.resize(capacity_ + 1);
    hashable_capacity_ = static_cast<float>(capacity_) * hashable_ratio_;
    collision_head_    = hashable_capacity_;
    collision_tail_    = hashable_capacity_;
  }

  // Member Functions
  [[nodiscard]] allocator_type get_allocator() const noexcept {
    return buckets_.get_allocator();
  }

  // Iterators //////////////

  constexpr iterator begin() noexcept { return iterator(this, 0); }

  constexpr const_iterator begin() const noexcept {
    return const_iterator(this, 0);
  }

  constexpr const_iterator cbegin() const noexcept {
    return const_iterator(this, 0);
  }

  constexpr iterator end() noexcept { return iterator(this, capacity_); }

  constexpr const_iterator end() const noexcept {
    return const_iterator(this, capacity_);
  }

  constexpr const_iterator cend() const noexcept {
    return const_iterator(this, capacity_);
  }

  // Capacity //////////////

  [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

  [[nodiscard]] constexpr size_type size() const noexcept { return size_; }

  [[nodiscard]] constexpr size_type max_size() const noexcept {
    return buckets_.max_size();
  }

  // Modifiers //////////////

  void clear() noexcept {
    for (auto& bucket : buckets_) {
      _set_empty(bucket.fingerprint_full_);
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

  template <typename P>
    requires std::is_convertible_v<P&&, key_type>
  std::pair<iterator, bool> insert(P&& value) {
    return _emplace(std::forward<P>(value));
  }

  constexpr iterator insert(const_iterator /*hint*/, const value_type& value) {
    return insert(value).first;
  }

  constexpr iterator insert(const_iterator /*hint*/, value_type&& value) {
    return insert(std::move(value)).first;
  }

  constexpr iterator insert(const_iterator /*hint*/, const key_type& value)
    requires(std::is_same_v<mapped_type, IsAHashSet>)
  {
    return insert(value).first;
  }

  constexpr iterator insert(const_iterator /*hint*/, key_type&& value)
    requires(std::is_same_v<mapped_type, IsAHashSet>)
  {
    return insert(std::move(value)).first;
  }

  template <typename P>
    requires std::is_convertible_v<P&&, key_type>
  constexpr auto insert(const_iterator /*hint*/, P&& value) -> iterator {
    return insert(std::forward<P>(value)).first;
  }

  template <class InputIt> void insert(InputIt first, InputIt last) {
    while (first != last) {
      insert(*first);
      ++first;
    }
  }

  void insert(std::initializer_list<value_type> ilist) {
    insert(ilist.begin(), ilist.end());
  }

  template <class M>
  constexpr std::pair<iterator, bool> insert_or_assign(const key_type& k,
                                                       M&& obj) {
    auto result = try_emplace(k, std::forward<M>(obj));
    if (! result.second) {
      result.first->second = std::forward<M>(obj);
    }
    return result;
  }

  template <class M>
  constexpr std::pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj) {
    auto result = try_emplace(std::move(k), std::forward<M>(obj));
    if (! result.second) {
      result.first->second = std::forward<M>(obj);
    }
    return result;
  }

  template <class M>
  constexpr iterator insert_or_assign(const_iterator /*hint*/,
                                      const key_type& k, M&& obj) {
    return insert_or_assign(k, std::forward<M>(obj)).first;
  }

  template <class M>
  constexpr iterator insert_or_assign(const_iterator /*hint*/, key_type&& k,
                                      M&& obj) {
    return insert_or_assign(std::move(k), std::forward<M>(obj)).first;
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return _emplace(std::forward<Args>(args)...);
  }

  template <class... Args>
  iterator emplace_hint(const_iterator /*hint*/, Args&&... args) {
    return emplace(std::forward<Args>(args)...).first;
  }

  iterator erase(iterator it) {
    _erase(buckets_[it.index_].pair_.first);
    return iterator(this, it.index_);
  }

  iterator erase(const_iterator it) {
    _erase(buckets_[it.index_].pair_.first);
    return iterator(this, it.index_);
  }

  iterator erase(const_iterator it_start, const_iterator it_end) {
    for (; it_start != it_end && it_start != end(); ++it_start) {
      _erase(buckets_[it_start.index_].pair_.first);
    }
    return iterator(this, it_start.index_);
  }

  size_type erase(const key_type& key) { return _erase(key); }

  template <typename K>
    requires std::is_convertible_v<K, key_type>
  size_type erase(K&& x) {
    return _erase(x);
  }

  void swap(dense_hashbase& other) noexcept(nothrow_swapable_v) {
    std::swap(*this, other);
  }

  void merge(const dense_hashbase& other) {
    for (const auto& elem : other) { _emplace(elem); }
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
    auto first = find(key).first;
    return {iterator(this, first), iterator(this, first + 1)};
  }

  std::pair<const_iterator, const_iterator>
  equal_range(const key_type& key) const {
    auto first = find(key).first;
    return {const_iterator(this, first), const_iterator(this, first + 1)};
  }

  template <typename K>
    requires std::is_convertible_v<K, key_type>
  std::pair<iterator, iterator> equal_range(const K& x) {
    auto first = find(x).first;
    return {iterator(this, first), iterator(this, first + 1)};
  }

  template <typename K>
    requires std::is_convertible_v<K, key_type>
  std::pair<const_iterator, const_iterator> equal_range(const K& x) const {
    auto first = find(x).first;
    return {const_iterator(this, first), const_iterator(this, first + 1)};
  }

  // Bucket Interface ///////

  [[nodiscard]] size_type bucket_count() const noexcept { return capacity_; }

  [[nodiscard]] size_type max_bucket_count() const {
    return buckets_.max_size();
  }

  // Hash Policy ///////

  [[nodiscard]] float load_factor() const {
    return static_cast<float>(size()) / static_cast<float>(bucket_count());
  }

  void max_load_factor(float load_factor) {
    if (load_factor <= 0 || load_factor > 1) {
      throw std::invalid_argument("dro::load_factor: Argument must be "
                                  "greater than zero and not greater than 1.");
    }
    load_factor_ = load_factor;
    reserve(size_);
  }

  [[nodiscard]] float max_load_factor() const { return load_factor_; }

  // Non-standard API
  void growth_multiple(float growth_multiple) {
    if (growth_multiple <= 1) {
      throw std::invalid_argument("dro::growth_multiple: Argument must be "
                                  "greater than 1.");
    }
    growth_multiple_ = growth_multiple;
  }

  [[nodiscard]] float growth_multiple() const { return growth_multiple_; }

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
  // template <typename... Args>
  // std::pair<iterator, bool> _emplace(Args&&... args) {
  //   key_type key {};
  //   _build_key(key, std::forward<Args>(args)...);
  //   return _emplace(key, std::forward<Args>(args)...);
  // }

  template <typename... Args>
  std::pair<iterator, bool> _emplace(Args&&... args) {
    key_type key {};
    _build_key(key, std::forward<Args>(args)...);
    uint64_t key_hash      = _hash(key);
    auto find_locations    = _find_hash(key, key_hash);
    size_type insert_index = find_locations.first;
    if (insert_index != capacity_) {
      return {iterator(this, insert_index), false};
    }
    // Not in the hashmap, will 100% insert.
    if (! _validate_load_factor_bounds()) {
      return _emplace(std::forward<Args>(args)...);
    }
    size_type prev_index = find_locations.second;
    insert_index         = prev_index;
    if (_get_full(buckets_[prev_index].fingerprint_full_) &&
        ! _emplace_collisions(insert_index, prev_index)) {
      return _emplace(std::forward<Args>(args)...);
    }
    // Insert into hash base
    _emplace_key(insert_index, key);
    _emplace_value(insert_index, std::forward<Args>(args)...);
    _set_fingerprint(buckets_[insert_index].fingerprint_full_, key_hash);
    buckets_[insert_index].next_ = 0;
    ++size_;
    return {iterator(this, insert_index), true};
  }

  bool _emplace_collisions(size_type& insert_index,
                           const size_type& prev_index) {
    // Insert at the end of the collisions vector
    if (collision_tail_ == collision_head_) {
      if (! _validate_collision_space_bounds()) {
        return false;
      }
      insert_index = collision_head_;
      ++collision_head_;
      ++collision_tail_;
    } else {
      // Fill in the empty slots
      insert_index = buckets_[collision_head_].next_;
      if (insert_index == collision_tail_) {
        collision_tail_ = collision_head_;
      } else {
        buckets_[collision_head_].next_ = buckets_[insert_index].next_;
      }
    }
    // Update collision chain
    buckets_[prev_index].next_ = insert_index;
    return true;
  }

  template <typename First, typename... Args>
  void _build_key(key_type& key, First&& first, Args&&... args)
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_move_assignable_v<key_type>)
  {
    key = first;
  }

  template <typename First, typename... Args>
  void _build_key(key_type& key, First&& first, Args&&... args)
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             ! std::is_move_assignable_v<key_type> &&
             std::is_copy_assignable_v<key_type>)
  {
    key_type non_movable(first);
    key = non_movable;
  }

  template <typename... Args>
  void _build_key(key_type& key, Args&&... args)
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_constructible_v<key_type, Args && ...> &&
             std::is_move_assignable_v<key_type>)
  {
    key = key_type(std::forward<Args>(args)...);
  }

  template <typename... Args>
  void _build_key(key_type& key, Args&&... args)
    requires(std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_constructible_v<key_type, Args && ...> &&
             std::is_copy_assignable_v<key_type> &&
             ! std::is_move_assignable_v<key_type>)
  {
    key_type non_movable(std::forward<Args>(args)...);
    key = non_movable;
  }

  void _emplace_key(const size_type& insert_index,
                    key_type& key) noexcept(densehash_nothrow_v)
    requires(std::is_move_assignable_v<key_type>)
  {
    buckets_[insert_index].pair_.first = std::move(key);
  }

  void _emplace_key(const size_type& insert_index,
                    key_type& key) noexcept(densehash_nothrow_v)

    requires(std::is_copy_assignable_v<key_type> &&
             ! std::is_move_assignable_v<key_type>)
  {
    buckets_[insert_index].pair_.first = key;
  }

  template <typename... Args>
  void _emplace_value(const size_type& insert_index, Args&&... args) noexcept

    requires(std::is_same_v<mapped_type, IsAHashSet>)
  {}// Intentionally blank for hashset

  template <typename First, typename... Args>
  void _emplace_value(const size_type& insert_index, First&&,
                      Args&&... args) noexcept(densehash_nothrow_v)
    requires(! std::is_same_v<mapped_type, IsAHashSet> &&
             std::is_move_assignable_v<mapped_type> &&
             std::is_constructible_v<mapped_type, Args && ...>)
  {
    buckets_[insert_index].pair_.second =
        mapped_type(std::forward<Args>(args)...);
  }

  template <typename First, typename... Args>
  void _emplace_value(const size_type& insert_index, First&&,
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
    auto find_locations    = _find(key);
    size_type& erase_index = find_locations.first;
    if (erase_index == capacity_) {
      return 0U;
    }
    // Definitely in the hash base
    auto& bucket    = buckets_[erase_index];
    size_type& next = bucket.next_;
    if (erase_index < hashable_capacity_) {
      if (next == 0) {
        _set_empty(bucket.fingerprint_full_);
        --size_;
        return 1U;
      }
      std::swap(bucket, buckets_[next]);
      erase_index = next;
    } else {
      size_type& prev_index      = find_locations.second;
      buckets_[prev_index].next_ = next;
    }
    _set_empty(buckets_[erase_index].fingerprint_full_);
    buckets_[erase_index].next_     = 0;
    buckets_[collision_tail_].next_ = erase_index;
    collision_tail_                 = erase_index;
    --size_;
    return 1U;
  }

  template <typename K>
  std::pair<size_type, size_type> _find(const K& key) const
    requires std::is_convertible_v<K, key_type>
  {
    uint64_t key_hash = _hash(key);
    return _find_hash(key, key_hash);
  }

  template <typename K>
  std::pair<size_type, size_type> _find_hash(const K& key,
                                             const uint64_t& key_hash) const
    requires std::is_convertible_v<K, key_type>
  {
    size_type prev_index {};// This is to avoid needing a doubly linked list
    uint64_t fingerprint = _get_fingerprint(key_hash);
    for (size_type index = _index_from_hash(key_hash);;) {
      auto& bucket                   = buckets_[index];
      const auto& bucket_fingerprint = bucket.fingerprint_full_;
      if (_get_full(bucket_fingerprint) &&
          fingerprint == _get_fingerprint(bucket_fingerprint) &&
          equal_(bucket.pair_.first, key)) {
        return {index, prev_index};
      }
      prev_index = index;
      index      = bucket.next_;
      if (index == 0) {
        break;
      }
    }
    return {capacity_, prev_index};
  }

  [[nodiscard]] bool _validate_load_factor_bounds() {
    auto capacity_float    = static_cast<float>(capacity_);
    size_type max_capacity = capacity_float * load_factor_;
    if (size_ + 1 > max_capacity) {
      size_type new_capacity = capacity_float * growth_multiple_;
      _rehash(new_capacity);
      return false;
    }
    return true;
  }

  [[nodiscard]] bool _validate_collision_space_bounds() {
    if (collision_head_ >= capacity_) {
      size_type new_capacity = static_cast<float>(capacity_) * growth_multiple_;
      _rehash(new_capacity);
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

  void _set_fingerprint(uint64_t& fingerprint_full,
                        const uint64_t& hash) noexcept {
    fingerprint_full = hash;
    _set_full(fingerprint_full);
  }

  [[nodiscard]] uint64_t
  _get_fingerprint(const uint64_t& fingerprint_full) const noexcept {
    return fingerprint_full >> 1;
  }

  void _set_empty(uint64_t& fingerprint_full) noexcept {
    uint64_t mask = ~1;
    fingerprint_full &= mask;
  }

  void _set_full(uint64_t& fingerprint_full) noexcept { fingerprint_full |= 1; }

  [[nodiscard]] bool
  _get_full(const uint64_t& fingerprint_full) const noexcept {
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

namespace pmr {

template <detail::densehash_t Key, detail::densehash_t Value,
          typename Hash     = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
using dense_hashmap =
    dense_hashmap<Key, Value, Hash, KeyEqual,
                  std::pmr::polymorphic_allocator<std::pair<Key, Value>>>;

template <detail::densehash_t Key, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
using dense_hashset =
    dense_hashset<Key, Hash, KeyEqual,
                  std::pmr::polymorphic_allocator<
                      detail::hashnode<detail::hashset_pair<Key>>>>;
}// namespace pmr
}// namespace dro

namespace std {

template <typename Key, typename Value, typename Hash, typename KeyEqual,
          typename Allocator>
constexpr void
swap(dro::dense_hashmap<Key, Value, Hash, KeyEqual, Allocator>& lhs,
     dro::dense_hashmap<Key, Value, Hash, KeyEqual, Allocator>&
         rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}
}// namespace std
#endif
