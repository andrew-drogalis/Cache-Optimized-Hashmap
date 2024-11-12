# Dense HashMap

A STL compliant open addressing hashmap and hashset that uses linear probing to find the next available slots in the underlying vector. Faster
than [std::unordered_map](https://en.cppreference.com/w/cpp/container/unordered_map) for large sized workloads and comparable for small size workloads.

## Table of Contents

- [Usage](#Usage)
- [Benchmarks](#Benchmarks)
- [Installing](#Installing)
- [Sources](#Sources)

## Implementation

_Detailed Discussion_

ToDo:

- [ ] 100% code coverage - currently ~80%.

## Usage

Main points:

- The key and value must be default constructible.
- The key and value must be copy or move assignable.
- Memory isn't deallocated on erase. Must wait on destructor.
- _Weakness:_ All the iterators are invalidated on all modifying operations.

#### Constructor

The full list of template arguments are as follows:

- Key: Must be default constructible and a copyable or moveable type.
- Value: Must be default constructible and a copyable or moveable type.
- Hash: Function used to hash the keys.
- KeyEqual: Function used to compare keys for equality, default std::equal_to<Key>.
- Allocator: Allocator passed to the vector, takes a std::pair<Key, Value> as the template parameters.

The default count is (1) and the std::allocator is the default memory allocator.

- `HashMap<Key, Value> hashMap(size_type count = 1, const Allocator& allocator = Allocator());`

- `HashSet<Key> hashSet(size_type count = 1, const Allocator& allocator = Allocator());`

#### Member Functions

- `allocator_type get_allocator() const noexcept`

  Returns the associated allocator.

#### Iterators

- `iterator begin() noexcept;`

  Returns iterator to the first element.

- `const_iterator cbegin() const noexcept;`

  Returns constant iterator to the first element.

- `iterator end() noexcept;`

  Returns iterator to one past the last element.

- `const_iterator cend() const noexcept;`

  Returns constant iterator to one past the last element.

#### Capacity

- `[[nodiscard]] bool empty() const noexcept;`

  Checks whether the container is empty.

- `[[nodiscard]] size_type size() const noexcept;`

  Returns the number of elements in the container.

- `[[nodiscard]] size_type max_size() const noexcept;`

  Returns the maximum possible number of elements.

#### Modifiers

- `void clear() noexcept;`

  Sets the size to zero and all the keys to empty.

- `std::pair<iterator, bool> insert(const value_type& pair);`

  **HashMap Only**: Inserts key value pair into map.

- `std::pair<iterator, bool> insert(const key_type& key);`

  **HashSet Only**: Inserts key into set.

- `std::pair<iterator, bool> emplace(Args&&... args);`

  Constructs value in place for map and key in place for set.

- `size_type erase(const key_type& key);`

  Erases element from container, and does NOT deallocate memory.

- `void swap(self_type& other) noexcept;`

  Swaps the contents of two containers.

- `void merge(self_type& source);`

  Merges the contents of one container into another.

#### Lookup

- `mapped_type& at(const key_type& key);`

  **HashMap Only**: Throws std::out_of_range if element doesn't exist.

- `mapped_type& operator[](const key_type& key);`

  **HashMap Only**: Inserts new element if element doesn't exist.

- `[[nodiscard]] size_type count(const key_type& key) const;`

  Returns the number of elements matching a specific key.

- `[[nodiscard]] iterator find(const key_type& key);`

  Returns an iterator to the element that matches the specific key. Returns end() if doesn't exist.

- `[[nodiscard]] bool contains(const key_type& key) const;`

  Checks if the specific key exists in the container.

- `[[nodiscard]] pair_iterator equal_range(const key_type& key);`

  Returns a range of elements matching a specific key.

#### Bucket Interface

- `[[nodiscard]] size_type bucket_count();`

  Returns the number of buckets.

- `[[nodiscard]] size_type max_bucket_count() const;`

  Returns the maximum number of buckets.

#### Hash Policy

- `[[nodiscard]] float load_factor() const;`

  Returns the current load factor.

- `void max_load_factor(float load_factor);`

  Sets the new max load factor, and rehashes the hashmap if the load factor is exceeded.

- `[[nodiscard]] float max_load_factor() const;`

  Returns the max load factor.

- `void rehash(size_type count);`

  Reserves at least the specified number of buckets and regenerates the hash table.

- `void reserve(std::size_t count);`

  Reserves space for at least the specified number of elements and regenerates the hash table.

#### Observers

- `[[nodiscard]] hasher hash_function() const;`

  Returns the function used to hash the keys.

- `[[nodiscard]] key_equal key_eq() const;`

  Returns the function used to compare keys for equality.

## Benchmarks

These benchmarks were taken on a (4) core Intel(R) Core(TM) i5-9300H CPU @ 2.40GHz with isolcpus on cores 2 and 3.
The linux kernel is v6.10.11-200.fc40.x86_64 and compiled with gcc version 14.2.1.

Most important aspects of benchmarking:

- Have at least one core isolated with isolcpus enabled in Grub.
- Compile with -DCMAKE_BUILD_TYPE=Release

<img src="https://raw.githubusercontent.com/drogalis/Open-Addressing-Hashmap/refs/heads/main/assets/Average%20Random%20Insertion%20%26%20Deletion%20Time.png" alt="Average Random Insertion & Deletion Time" style="padding-top: 10px;">

## Installing

**Required C++20 or higher.**

To build and install the shared library, run the commands below.

```
    $ mkdir build && cd build
    $ cmake ..
    $ make
    $ sudo make install
```

## Sources

For help with building the open addressing hashmap, I want to cite the following sources:

- [Tessil Sparse Map](https://github.com/Tessil/sparse-map)
- [Rigtorp Hashmap](https://github.com/rigtorp/HashMap)

## License

This software is distributed under the MIT license. Please read [LICENSE](https://github.com/drogalis/Open-Addressing-Hashmap/blob/main/LICENSE) for information on the software availability and distribution.

## Contribution

Please open an issue of if you have any questions, suggestions, or feedback. Please submit bug reports, suggestions, and pull requests to the [GitHub issue tracker](https://github.com/drogalis/Open-Addressing-Hashmap/issues).
