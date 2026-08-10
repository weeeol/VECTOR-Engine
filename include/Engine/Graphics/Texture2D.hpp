#pragma once

#include <memory>
#include <string>


namespace VECTOR {

enum class TextureFormat {
  Unknown,
  RGBA16F,
  RGBA32F,
  RG16F,
  RG32F,
  Depth32F
};

class Texture2D {
public:
  virtual ~Texture2D() = default;

  virtual void Bind(unsigned int slot = 0) const = 0;
  virtual void Unbind() const = 0;

  virtual unsigned int GetID() const = 0;
  virtual int GetWidth() const = 0;
  virtual int GetHeight() const = 0;

  static std::shared_ptr<Texture2D> Create(const std::string &path);
  static std::shared_ptr<Texture2D> CreateRenderTarget(uint32_t width, uint32_t height, TextureFormat format = TextureFormat::RGBA16F);
};

} // namespace VECTOR
