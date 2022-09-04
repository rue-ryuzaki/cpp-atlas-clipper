#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// -----------------------------------------------------------------------------
// -- Image --------------------------------------------------------------------
// -----------------------------------------------------------------------------
class Image
{
public:
    // -- constructor ----------------------------------------------------------
    Image();
    explicit Image(std::string const& file);
    explicit Image(std::vector<uint8_t> const& data);

    // -- functions ------------------------------------------------------------
    bool load(std::string const& file);
    bool load(std::vector<uint8_t> const& data);

    void save_png(std::string const& file);

    Image subImage(int32_t x, int32_t y, int32_t w, int32_t h, bool revert = false);

    // -- data -----------------------------------------------------------------
    int32_t width() const;
    int32_t height() const;
    int32_t channels() const;
    unsigned char* data() const;

private:
    void onLoad();

    // -- data -----------------------------------------------------------------
    int32_t m_width;
    int32_t m_height;
    int32_t m_channels;
    std::shared_ptr<unsigned char> m_data;
};

#endif  // _IMAGE_H_
