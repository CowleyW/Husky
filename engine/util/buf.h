#pragma once

#include "core/types.h"
#include <vector>

template <typename T> struct Buf {
public:
  Buf(const T *inner_data, u32 size) : inner_data(inner_data), count(count) {}
  Buf(const std::vector<u8> buf) : inner_data(buf.data()), count(buf.size()) {}

  Buf<T> trim_left(u32 amount) const {
    return Buf<T>(this->inner_data + amount, this->count - amount);
  }

  const T *data() const { return this->inner_data; }

  u32 size() const { return this->count; }

private:
  const T *inner_data;
  u32 count;
};

template <typename T> struct MutBuf {
public:
  MutBuf(const T *inner_data, u32 size)
      : inner_data(inner_data), count(count) {}
  MutBuf(const Buf<T> &buf) : inner_data(buf.data()), count(buf.size()) {}

  void trim_left(u32 amount) {
    this->inner_data += amount;
    this->count -= amount;
  }

  const T *data() const { return this->inner_data; }

  u32 size() const { return this->count; }

private:
  const T *inner_data;
  u32 count;
};
