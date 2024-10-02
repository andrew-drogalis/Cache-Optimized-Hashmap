# Open Addressing Hashmap

## Benchmark

These benchmarks were taken on a (4) core Intel(R) Core(TM) i5-9300H CPU @ 2.40GHz with isolcpus on cores 2 and 3.
The linux kernel is v6.10.11-200.fc40.x86_64 and compiled with gcc version 14.2.1.

<img src="https://raw.githubusercontent.com/drogalis/Open-Addressing-Hashmap/refs/heads/main/assets/Average%20Random%20Insertion%20%26%20Deletion%20Time.png" alt="Average Random Insertion & Deletion Time" style="padding-top: 10px;">

## Installing

To build and install the shared library, run the commands below.

```
    $ mkdir build && cd build
    $ cmake .. -DCMAKE_BUILD_TYPE=Release
    $ sudo make install
```

## License

This software is distributed under the GNU license. Please read [LICENSE](https://github.com/drogalis/Open-Addressing-Hashmap/blob/main/LICENSE) for information on the software availability and distribution.
