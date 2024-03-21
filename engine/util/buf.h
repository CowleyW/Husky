#pragma once

#include <vector>

template <typename T>
struct Buf {
public:
  Buf(const T *inner_data, uint32_t size)
      : inner_data(inner_data),
        count(size) {
  }
  Buf(const std::vector<T> &buf) : inner_data(buf.data()), count(buf.size()) {
  }

  Buf<T> trim_left(uint32_t amount) const {
    return Buf<T>(this->inner_data + amount, this->count - amount);
  }

  const T *data() const {
    return this->inner_data;
  }

  uint32_t size() const {
    return this->count;
  }

private:
  const T *inner_data;
  uint32_t count;
};

template <typename T>
struct MutBuf {
public:
  MutBuf(const T *inner_data, uint32_t size)
      : inner_data(inner_data),
        count(size) {
  }
  MutBuf(const Buf<T> &buf) : inner_data(buf.data()), count(buf.size()) {
  }

  void trim_left(uint32_t amount) {
    this->inner_data += amount;
    this->count -= amount;
  }

  const T *data() const {
    return this->inner_data;
  }

  uint32_t size() const {
    return this->count;
  }

private:
  const T *inner_data;
  uint32_t count;
};
