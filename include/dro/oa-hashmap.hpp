// Andrew Drogalis Copyright (c) 2024, GNU 3.0 Licence
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#ifndef DRO_OA_HASHMAP
#define DRO_OA_HASHMAP

#include <algorithm>  // for max
#include <concepts>   // for requires
#include <cstddef>    // for size_t, ptrdiff_t
#include <functional> // for equal_to, hash
#include <iterator>   // for pair, forward_iterator_tag
#include <stdexcept>  // for out_of_range, invalid_argument
#include <string_view>// for hash
#include <type_traits>// for std::is_default_constructible
#include <utility>    // for pair, forward, make_pair
#include <vector>     // for allocator, vector

namespace dro
{

namespace details
{

template <typename T>
concept OAHashmap_Type =
    std::is_default_constructible<T>::value &&
    (std::is_move_assignable_v<T> || std::is_copy_assignable_v<T>);

template <typename T, typename... Args>
concept OAHashmap_NoThrow =
    std::is_nothrow_constructible_v<T, Args&&...> &&
    ((std::is_nothrow_copy_assignable_v<T> && std::is_copy_assignable_v<T>) ||
     (std::is_nothrow_move_assignable_v<T> && std::is_move_assignable_v<T>));

struct HashSetEmptyType
{
};

template <OAHashmap_Type Key> struct PairHashSet
{
  Key first;
  HashSetEmptyType second [[no_unique_address]];
  PairHashSet() = default;
  explicit PairHashSet(Key key, HashSetEmptyType) : first(key) {}
  bool operator==(const PairHashSet& other) { return first == first; }
  bool operator!=(const PairHashSet& other) { return ! (*this == other); }
};

template <typename Container> struct HashIterator
{
  using key_type          = typename Container::key_type;
  using mapped_type       = typename Container::mapped_type;
  using value_type        = typename Container::value_type;
  using size_type         = typename Container::size_type;
  using key_equal         = typename Container::key_equal;
  using difference_type   = typename Container::difference_type;
  using pointer           = key_type*;
  using reference         = key_type&;
  using const_pointer     = const key_type*;
  using const_reference   = const key_type&;
  using iterator_category = std::forward_iterator_tag;

  explicit HashIterator(Container* hashmap, size_type index)
      : hashmap_(hashmap), index_(index)
  {
    _nextValidIndex();
  }

  bool operator==(const HashIterator& other) const
  {
    return other.hashmap_ == hashmap_ && other.index_ == index_;
  }

  bool operator!=(const HashIterator& other) const
  {
    return ! (*this == other);
  }

  HashIterator& operator++()
  {
    if (index_ < hashmap_->buckets_.size())
    {
      ++index_;
      _nextValidIndex();
    }
    return *this;
  }

  reference operator*() const
    requires(std::is_same_v<mapped_type, HashSetEmptyType> &&
             ! std::is_const_v<Container>)
  {
    return hashmap_->buckets_[index_].first;
  }

  const_reference operator*() const
    requires(std::is_same_v<mapped_type, HashSetEmptyType> &&
             std::is_const_v<Container>)
  {
    return hashmap_->buckets_[index_].first;
  }

  pointer operator->() const
    requires(std::is_same_v<mapped_type, HashSetEmptyType> &&
             ! std::is_const_v<Container>)
  {
    return &hashmap_->buckets_[index_].first;
  }

  const_pointer operator->() const
    requires(std::is_same_v<mapped_type, HashSetEmptyType> &&
             std::is_const_v<Container>)
  {
    return &hashmap_->buckets_[index_].first;
  }

  value_type& operator*() const
    requires(! std::is_same_v<mapped_type, HashSetEmptyType> &&
             ! std::is_const_v<Container>)
  {
    return hashmap_->buckets_[index_];
  }

  const value_type& operator*() const
    requires(! std::is_same_v<mapped_type, HashSetEmptyType> &&
             std::is_const_v<Container>)
  {
    return hashmap_->buckets_[index_];
  }

  const value_type* operator->() const
    requires(! std::is_same_v<mapped_type, HashSetEmptyType> &&
             std::is_const_v<Container>)
  {
    return &hashmap_->buckets_[index_];
  }

  value_type* operator->() const
    requires(! std::is_same_v<mapped_type, HashSetEmptyType> &&
             ! std::is_const_v<Container>)
  {
    return &hashmap_->buckets_[index_];
  }

  void _nextValidIndex()
  {
    auto& buckets = hashmap_->buckets_;
    while (index_ < buckets.size() &&
           key_equal()(buckets[index_].first, hashmap_->empty_key_))
    {
      ++index_;
    }
  }

private:
  Container* hashmap_ = nullptr;
  size_type index_ {};
  friend Container;
};

template <OAHashmap_Type Key, OAHashmap_Type Value, typename Pair,
          typename Hash      = std::hash<Key>,
          typename KeyEqual  = std::equal_to<Key>,
          typename Allocator = std::allocator<Pair>>
class OAHashmap
{
public:
  using key_type        = Key;
  using mapped_type     = Value;
  using value_type      = Pair;
  using size_type       = std::size_t;
  using hasher          = Hash;
  using key_equal       = KeyEqual;
  using allocator_type  = Allocator;
  using difference_type = std::ptrdiff_t;
  using buckets         = std::vector<value_type, Allocator>;
  using iterator        = HashIterator<OAHashmap>;
  using const_iterator  = HashIterator<const OAHashmap>;

private:
  key_type empty_key_;
  buckets buckets_;
  size_type size_ {0};
  float load_factor_ = 0.4;
  friend iterator;
  friend const_iterator;

public:
  OAHashmap(key_type empty_key, size_type count,
            const Allocator& allocator = Allocator())
      : empty_key_(empty_key), buckets_(allocator)
  {
    buckets_.resize(count, value_type(empty_key_, mapped_type()));
  }

  // No memory allocated, this is redundant
  ~OAHashmap()                           = default;
  OAHashmap(const OAHashmap& other)      = default;
  OAHashmap& operator=(const OAHashmap&) = default;
  OAHashmap(OAHashmap&& other)           = default;
  OAHashmap& operator=(OAHashmap&&)      = default;

  // Member Functions
  allocator_type get_allocator() const noexcept
  {
    return buckets_.get_allocator();
  }

  // Iterators
  iterator begin() noexcept { return iterator(this, 0); }

  const_iterator begin() const noexcept { return const_iterator(this, 0); }

  const_iterator cbegin() const noexcept { return const_iterator(this, 0); }

  iterator end() noexcept { return iterator(this, buckets_.size()); }

  const_iterator end() const noexcept
  {
    return const_iterator(this, buckets_.size());
  }

  const_iterator cend() const noexcept
  {
    return const_iterator(this, buckets_.size());
  }

  // Capacity
  [[nodiscard]] bool empty() const noexcept { return ! size(); }

  [[nodiscard]] size_type size() const noexcept { return size_; }

  [[nodiscard]] size_type max_size() const noexcept
  {
    return buckets_.max_size() / 2;
  }

  // Modifiers
  void clear() noexcept
  {
    for (auto& b : buckets_) { b.first = empty_key_; }
    size_ = 0;
  }

  std::pair<iterator, bool> insert(const value_type& pair)
  {
    return _emplace(pair.first, pair.second);
  }

  std::pair<iterator, bool> insert(value_type&& pair)
  {
    return _emplace(pair.first, std::move(pair.second));
  }

  std::pair<iterator, bool> insert(const key_type& key)
    requires(std::is_same_v<mapped_type, HashSetEmptyType>)
  {
    return _emplace(key);
  }

  std::pair<iterator, bool> insert(key_type&& key)
    requires(std::is_same_v<mapped_type, HashSetEmptyType>)
  {
    return _emplace(std::move(key));
  }

  template <typename P>
    requires std::is_convertible_v<P, value_type>
  std::pair<iterator, bool> insert(P&& value)
  {
    return _emplace(value.first, std::move(value.second));
  }

  template <typename... Args> std::pair<iterator, bool> emplace(Args&&... args)
  {
    return _emplace(std::forward<Args>(args)...);
  }

  iterator erase(iterator it)
  {
    _erase(it.index_);
    return iterator(this, it.index_);
  }

  iterator erase(const_iterator it)
  {
    _erase(it.index_);
    return iterator(this, it.index_);
  }

  iterator erase(const_iterator itStart, const_iterator itEnd)
  {
    for (; itStart != itEnd; ++itStart) { _erase(itStart.index_); }
    return iterator(this, itStart.index_);
  }

  size_type erase(const key_type& key)
  {
    size_type index = _find(key);
    if (index != buckets_.size())
    {
      _erase(index);
      return 1U;
    }
    return 0U;
  }

  template <typename K>
    requires std::is_convertible_v<K, key_type>
  size_type erase(K&& x)
  {
    size_type index = _find(x);
    if (index != buckets_.size())
    {
      _erase(index);
      return 1U;
    }
    return 0U;
  }

  void swap(OAHashmap& other) noexcept
  {
    std::swap(buckets_, other.buckets_);
    std::swap(size_, other.size_);
    std::swap(empty_key_, other.empty_key_);
    std::swap(load_factor_, other.load_factor_);
  }

  void merge(const OAHashmap& other)
  {
    for (auto& elem : other) { _emplace(elem); }
  }

  // Get Values
  mapped_type& at(const key_type& key)
    requires(! std::is_same_v<mapped_type, HashSetEmptyType>)
  {
    size_type index = _find(key);
    if (index != buckets_.size())
    {
      return buckets_[index].second;
    }
    throw std::out_of_range("OAHashmap::at");
  }

  const mapped_type& at(const key_type& key) const
    requires(! std::is_same_v<mapped_type, HashSetEmptyType>)
  {
    size_type index = _find(key);
    if (index != buckets_.size())
    {
      return buckets_[index].second;
    }
    throw std::out_of_range("OAHashmap::at");
  }

  template <typename K>
  mapped_type& at(const K& x)
    requires(! std::is_same_v<mapped_type, HashSetEmptyType> &&
             std::is_convertible_v<K, key_type>)
  {
    size_type index = _find(x);
    if (index != buckets_.size())
    {
      return buckets_[index].second;
    }
    throw std::out_of_range("OAHashmap::at");
  }

  template <typename K>
  const mapped_type& at(const K& x) const
    requires(! std::is_same_v<mapped_type, HashSetEmptyType> &&
             std::is_convertible_v<K, key_type>)
  {
    size_type index = _find(x);
    if (index != buckets_.size())
    {
      return buckets_[index].second;
    }
    throw std::out_of_range("OAHashmap::at");
  }

  mapped_type& operator[](const key_type& key)
    requires(! std::is_same_v<mapped_type, HashSetEmptyType>)
  {
    size_type index = _find(key);
    if (index != buckets_.size())
    {
      return buckets_[index].second;
    }
    index = _emplace(key).first.index_;
    return buckets_[index].second;
  }

  mapped_type& operator[](key_type&& key)
    requires(! std::is_same_v<mapped_type, HashSetEmptyType>)
  {
    size_type index = _find(key);
    if (index != buckets_.size())
    {
      return buckets_[index].second;
    }
    index = _emplace(key).first.index_;
    return buckets_[index].second;
  }

  template <typename K>
  mapped_type& operator[](K&& x)
    requires(! std::is_same_v<mapped_type, HashSetEmptyType> &&
             std::is_convertible_v<K, key_type>)
  {
    size_type index = _find(x);
    if (index != buckets_.size())
    {
      return buckets_[index].second;
    }
    index = _emplace(x).first.index_;
    return buckets_[index].second;
  }

  size_type count(const key_type& key) const
  {
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

  const_iterator find(const key_type& key) const
  {
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

  std::pair<iterator, iterator> equal_range(const key_type& key)
  {
    auto first  = find(key);
    auto second = first;
    return {first, ++second};
  }

  std::pair<const_iterator, const_iterator> equal_range(
      const key_type& key) const
  {
    auto first  = find(key);
    auto second = first;
    return {first, ++second};
  }

  template <typename K>
    requires std::is_convertible_v<K, key_type>
  std::pair<iterator, iterator> equal_range(const K& x)
  {
    auto first  = find(x);
    auto second = first;
    return {first, ++second};
  }

  template <typename K>
    requires std::is_convertible_v<K, key_type>
  std::pair<const_iterator, const_iterator> equal_range(const K& x) const
  {
    auto first  = find(x);
    auto second = first;
    return {first, ++second};
  }

  // Bucket Interface
  [[nodiscard]] size_type bucket_count() const { return buckets_.size(); }

  [[nodiscard]] size_type max_bucket_count() const
  {
    return buckets_.max_size();
  }

  // Hash Policy
  [[nodiscard]] float load_factor() const
  {
    return static_cast<float>(size()) / static_cast<float>(max_bucket_count());
  }

  void max_load_factor(float load_factor)
  {
    load_factor_ = load_factor;
    reserve(size_);
  }

  [[nodiscard]] float max_load_factor() const { return load_factor_; }

  void rehash(size_type count)
  {
    double mult       = 1.0 / load_factor_;
    size_type newSize = static_cast<double>(size_) * mult;
    count             = std::max(count, newSize);
    OAHashmap other(empty_key_, count, get_allocator());
    for (auto it = begin(); it != end(); ++it) { other.insert(*it); }
    swap(other);
  }

  void reserve(std::size_t count)
  {
    double mult        = 1.0 / load_factor_;
    size_type newCount = static_cast<double>(count) * mult;
    if (newCount > buckets_.size())
    {
      rehash(newCount);
    }
  }

  // Observers
  [[nodiscard]] hasher hash_function() const { return hasher(); }

  [[nodiscard]] key_equal key_eq() const { return key_equal(); }

private:
  template <typename K, typename... Args>
  std::pair<iterator, bool> _emplace(const K& key, Args&&... args)
    requires(std::is_convertible_v<K, key_type> &&
             ! std::is_same_v<mapped_type, HashSetEmptyType> &&
             std::is_constructible_v<mapped_type, Args...>)
  {
    _validateKey(key);
    reserve(size_ + 1);
    for (size_type index = _hash(key);; index = _next(index))
    {
      if (key_equal()(buckets_[index].first, empty_key_))
      {
        buckets_[index].second = mapped_type(std::forward<Args...>(args)...);
        buckets_[index].first  = key;
        ++size_;
        return std::make_pair(iterator(this, index), true);
      }
      else if (key_equal()(buckets_[index].first, key))
      {
        return std::make_pair(iterator(this, index), false);
      }
    }
  }

  template <typename... Args>
  std::pair<iterator, bool> _emplace(Args&&... args)
    requires(std::is_constructible_v<key_type, Args...> &&
             std::is_same_v<mapped_type, HashSetEmptyType>)
  {
    key_type key = key_type(std::forward<Args...>(args)...);
    _validateKey(key);
    reserve(size_ + 1);
    for (size_type index = _hash(key);; index = _next(index))
    {
      if (key_equal()(buckets_[index].first, empty_key_))
      {
        buckets_[index].second = mapped_type();
        buckets_[index].first  = key_type(std::forward<Args...>(args)...);
        ++size_;
        return std::make_pair(iterator(this, index), true);
      }
      else if (key_equal()(buckets_[index].first, key))
      {
        return std::make_pair(iterator(this, index), false);
      }
    }
  }

  void _erase(size_type erase_index)
  {
    for (size_type index = _next(erase_index);; index = _next(index))
    {
      if (key_equal()(buckets_[index].first, empty_key_))
      {
        buckets_[erase_index].first = empty_key_;
        --size_;
        return;
      }
      size_type idealIndexBucket = _hash(buckets_[index].first);
      if (_differenceFromIdeal(erase_index, idealIndexBucket) <
          _differenceFromIdeal(index, idealIndexBucket))
      {
        buckets_[erase_index] = buckets_[index];
        erase_index           = index;
      }
    }
  }

  template <typename K>
  size_type _find(const K& key) const
    requires std::is_convertible_v<K, key_type>
  {
    _validateKey(key);
    for (size_type index = _hash(key);; index = _next(index))
    {
      if (key_equal()(buckets_[index].first, key))
      {
        return index;
      }
      if (key_equal()(buckets_[index].first, empty_key_))
      {
        return buckets_.size();
      }
    }
  }

  template <typename K>
  [[nodiscard]] size_type _hash(const K& key) const
      noexcept(noexcept(Hash()(key)))
    requires std::is_convertible_v<K, key_type>
  {
    return hasher()(key) % buckets_.size();
  }

  [[nodiscard]] size_type _next(size_type index) const noexcept
  {
    return (index + 1) % buckets_.size();
  }

  [[nodiscard]] size_type _differenceFromIdeal(size_type indexA,
                                               size_type indexB) const noexcept
  {
    return (buckets_.size() + (indexA - indexB)) % buckets_.size();
  }

  template <typename K>
  void _validateKey(const K& key) const
    requires std::is_convertible_v<K, key_type>
  {
    if (key_equal()(key, empty_key_))
    {
      throw std::invalid_argument("Key cannot equal empty_key type");
    }
  }
};
}// namespace details

template <details::OAHashmap_Type Key, details::OAHashmap_Type Value,
          typename Hash      = std::hash<Key>,
          typename KeyEqual  = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<Key, Value>>>
class HashMap : public details::OAHashmap<Key, Value, std::pair<Key, Value>,
                                          Hash, KeyEqual, Allocator>
{
  using size_type = std::size_t;
  using base_type = details::OAHashmap<Key, Value, std::pair<Key, Value>, Hash,
                                       KeyEqual, Allocator>;

public:
  explicit HashMap(Key empty_key, size_type count = 1,
                   const Allocator& allocator = Allocator())
      : base_type(empty_key, count, allocator)
  {
  }
};

template <details::OAHashmap_Type Key, typename Hash = std::hash<Key>,
          typename KeyEqual  = std::equal_to<Key>,
          typename Allocator = std::allocator<details::PairHashSet<Key>>>
class HashSet : public details::OAHashmap<Key, details::HashSetEmptyType,
                                          details::PairHashSet<Key>, Hash,
                                          KeyEqual, Allocator>
{
  using size_type = std::size_t;
  using base_type =
      details::OAHashmap<Key, details::HashSetEmptyType,
                         details::PairHashSet<Key>, Hash, KeyEqual, Allocator>;

public:
  explicit HashSet(Key empty_key, size_type count = 1,
                   const Allocator& allocator = Allocator())
      : base_type(empty_key, count, allocator)
  {
  }
};

}// namespace dro
#endif
