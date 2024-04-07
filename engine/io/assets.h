#pragma once

#include <cstdint>
#include <vector>

enum class AssetType : uint32_t {
  Unknown = 0,
  Texture = 0xCACACA00,
  Mesh = 0xCACACA01,
};

AssetType parse_from_header(std::vector<uint8_t> contents);
