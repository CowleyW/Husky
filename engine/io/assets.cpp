#include "assets.h"

#include "util/buf.h"
#include "util/serialize.h"

AssetType parse_from_header(std::vector<uint8_t> contents) {
  if (contents.size() < 4) {
    return AssetType::Unknown;
  }

  MutBuf buf(contents);
  uint32_t header = Serialize::deserialize_u32(buf);

  switch (header) {
  case (uint32_t)AssetType::Texture:
    return AssetType::Texture;
  case (uint32_t)AssetType::Mesh:
    return AssetType::Mesh;
  default:
    return AssetType::Unknown;
  }
}
