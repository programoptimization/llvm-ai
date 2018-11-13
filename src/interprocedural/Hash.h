
#ifndef HASH_H_
#define HASH_H_

#include <cstdint>
#include <utility>

namespace pcpo {
// Adapted from:
// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
inline void HashCombine(std::size_t &seed) {}
template <typename T, typename... Rest>
void HashCombine(std::size_t &seed, T &&v, Rest &&... rest) {
  std::hash<typename std::decay<T>::type> hasher;
  seed ^= hasher(std::forward<T>(v)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  HashCombine(seed, std::forward<Rest>(rest)...);
}

/// Returns a combined hash for all the given arguments
template <typename... T> std::size_t HashArguments(T &&... args) {
  std::size_t seed = 0;
  HashCombine(seed, std::forward<T>(args)...);
  return seed;
}

/// Returns a combined hash for all the given arguments
template <typename Iterator>
template <typename... T>
std::size_t HashRange(Iterator begin, Iterator end) {
  std::size_t seed = 0;
  auto itr = begin;
  for (; begin != end; ++itr) {
    HashCombine(seed, *itr);
  }
  return seed;
}
} // namespace pcpo

#endif // HASH_H_
