#include "image.h"

#include <stdexcept>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <stb_image/stb_image.h>
#pragma GCC diagnostic pop

#include <SDL2/SDL_image.h>

// -----------------------------------------------------------------------------
// -- Image --------------------------------------------------------------------
// -----------------------------------------------------------------------------
Image::Image()
    : m_width(),
      m_height(),
      m_channels(),
      m_data(nullptr)
{
}

// -----------------------------------------------------------------------------
Image::Image(std::string const& file)
    : m_width(),
      m_height(),
      m_channels(),
      m_data(nullptr)
{
    load(file);
}

// -----------------------------------------------------------------------------
Image::Image(std::vector<uint8_t> const& data)
    : m_width(),
      m_height(),
      m_channels(),
      m_data(nullptr)
{
    load(data);
}

// -----------------------------------------------------------------------------
bool Image::load(std::string const& file)
{
    m_data = std::shared_ptr<unsigned char>(
                stbi_load(file.c_str(), &m_width, &m_height, &m_channels, 0),
                [] (unsigned char* ptr) { stbi_image_free(ptr); });
    if (!m_data.get()) {
        std::cerr << "Texture failed to load at path: " << file << std::endl;
        return false;
    }
    onLoad();
    return true;
}

// -----------------------------------------------------------------------------
bool Image::load(std::vector<uint8_t> const& data)
{
    m_data = std::shared_ptr<unsigned char>(
                stbi_load_from_memory(&data[0], static_cast<int>(data.size()),
                                      &m_width, &m_height, &m_channels, 0),
                [] (unsigned char* ptr) { stbi_image_free(ptr); });
    if (!m_data.get()) {
        std::cerr << "Texture failed to load from memory" << std::endl;
        return false;
    }
    onLoad();
    return true;
}

// -----------------------------------------------------------------------------
void Image::savePng(std::string const& file)
{
    int32_t w = width();
    int32_t h = height();
    int32_t bits = channels();

    uint32_t rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int32_t shift = (channels() == 3) ? 8 : 0;
    rmask = 0xff000000 >> shift;
    gmask = 0x00ff0000 >> shift;
    bmask = 0x0000ff00 >> shift;
    amask = 0x000000ff >> shift;
#else // little endian, like x86
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = (channels() == 3) ? 0 : 0xff000000;
#endif  // SDL_BYTEORDER == SDL_BIG_ENDIAN

    auto* surf = SDL_CreateRGBSurfaceFrom(data(), w, h, 8 * bits, w * bits,
                                          rmask, gmask, bmask, amask);

    IMG_SavePNG(surf, file.c_str());

    SDL_FreeSurface(surf);
}

// -----------------------------------------------------------------------------
Image Image::subImage(int32_t x, int32_t y, int32_t w, int32_t h, bool inverseY)
{
    Image result;
    result.m_width    = w;
    result.m_height   = h;
    result.m_channels = m_channels;
    result.m_data     = std::shared_ptr<unsigned char>(
                reinterpret_cast<unsigned char*>(
                    malloc(static_cast<std::size_t>(m_channels * w * h))),
                [] (unsigned char* ptr) { free(ptr); });
    for (int32_t ix = 0; ix < w; ++ix) {
        for (int32_t iy = 0; iy < h; ++iy) {
            for (int32_t c = 0; c < m_channels; ++c) {
                int32_t index1 = m_channels * (ix + (inverseY ? (h - iy - 1) : iy) * w) + c;
                int32_t index2 = m_channels * ((ix + x) + (iy + y) * m_width) + c;
                result.m_data.get()[index1] = m_data.get()[index2];
            }
        }
    }
    return result;
}

// -----------------------------------------------------------------------------
int32_t Image::width() const
{
    return m_width;
}

// -----------------------------------------------------------------------------
int32_t Image::height() const
{
    return m_height;
}

// -----------------------------------------------------------------------------
int32_t Image::channels() const
{
    return m_channels;
}

// -----------------------------------------------------------------------------
unsigned char* Image::data() const
{
    return m_data.get();
}

// -----------------------------------------------------------------------------
void Image::onLoad()
{
    if (!m_data.get()) {
        // set default texture
        m_width   = 1;
        m_height  = 1;
        m_channels= 4;
        m_data    = std::shared_ptr<unsigned char>(
                    reinterpret_cast<unsigned char*>(
                        malloc(static_cast<std::size_t>(m_channels))),
                    [] (unsigned char* ptr) { free(ptr); });
        m_data.get()[0] = static_cast<unsigned char>(255);
        m_data.get()[1] = static_cast<unsigned char>(0);
        m_data.get()[2] = static_cast<unsigned char>(255);
        m_data.get()[3] = static_cast<unsigned char>(255);
    }
}
