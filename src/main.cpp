/*
* MIT License
*
* Tool to clip atlas to different image resources (cpp-atlas-clipper)
*
* Copyright (c) 2022 Golubchikov Mihail <https://github.com/rue-ryuzaki>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#if __cplusplus >= 201703L
#include <filesystem>
#else
#include <sys/types.h>
#include <sys/stat.h>

#include <cstdlib>
#endif  // C++17+

#include <algorithm>
#include <fstream>
#include <iostream>

#include <SDL2/SDL_image.h>

#include <argparse/argparse.hpp>

#include "image.h"

namespace detail {
inline bool
_ends_with(std::string const& s, std::string const& value)
{
#if __cplusplus >= 202002L
    return s.ends_with(value);
#else
    return s.size() >= value.size()
            && 0 == s.compare(s.size() - value.size(), value.size(), value);
#endif  // C++20+
}

inline std::string
_directory_name(std::string const& path)
{
#if __cplusplus >= 201703L
    return std::filesystem::path(path.c_str()).parent_path().string();
#else
    auto pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(0, pos);
    }
    return std::string();
#endif  // C++17+
}

inline std::string
_file_name(std::string const& path)
{
#if __cplusplus >= 201703L
    return std::filesystem::path(path.c_str()).filename().string();
#else
    return path.substr(path.find_last_of("/\\") + 1);
#endif  // C++17+
}

inline bool
_is_directory_exists(std::string const& path)
{
#if __cplusplus >= 201703L
    std::filesystem::path dir(path.c_str());
    return std::filesystem::is_directory(dir);
#else
    struct stat info;
    return stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR);
#endif  // C++17+
}

inline bool
_is_file_exists(std::string const& path)
{
#if __cplusplus >= 201703L
    std::filesystem::path file(path.c_str());
    return std::filesystem::exists(file)
            && !std::filesystem::is_directory(file);
#else
    struct stat info;
    return stat(path.c_str(), &info) == 0 && !(info.st_mode & S_IFDIR);
#endif  // C++17+
}

inline bool
_make_directory(std::string const& path)
{
#if __cplusplus >= 201703L
    std::filesystem::path dir(path.c_str());
    return std::filesystem::create_directory(dir);
#else
    std::string command = "mkdir -p " + path;
    return system(command.c_str()) == 0;
#endif  // C++17+
}

inline std::string
_replace(std::string s, char old, std::string const& value)
{
    auto pos = s.find(old);
    while (pos != std::string::npos) {
        s.replace(pos, 1, value);
        pos = s.find(old, pos + value.size());
    }
    return s;
}

inline std::string
_replace(std::string s,
         std::function<bool(unsigned char)> func, std::string const& value)
{
    std::string res;
    for (auto c : s) {
        if (func(static_cast<unsigned char>(c))) {
            res += value;
        } else {
            res += c;
        }
    }
    return res;
}

inline std::string
_to_upper(std::string s)
{
    std::transform(std::begin(s), std::end(s), std::begin(s),
                   [] (unsigned char c)
    { return static_cast<char>(std::toupper(c)); });
    return s;
}
}  // namespace detail

struct Atlas
{
    inline Atlas() noexcept
        : name(),
          x(),
          y(),
          w(),
          h()
    { }

    std::string name;
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
};

std::istream& operator >>(std::istream& os, Atlas& obj)
{
    os >> obj.name >> obj.x >> obj.y >> obj.w >> obj.h;
    return os;
}

int main(int argc, char const* argv[])
{
    auto parser = argparse::ArgumentParser(argc, argv)
            .description("Tool to clip atlas to different image resources")
            .epilog("by rue-ryuzaki (c) 2022")
            .fromfile_prefix_chars("@")
            .formatter_class(argparse::ArgumentDefaultsHelpFormatter);
    parser.add_argument("-i")
            .action("store")
            .required(true)
            .metavar("ATLAS")
            .type<std::string>()
            .help("input atlas file");
    parser.add_argument("-o")
            .action("append")
            .metavar("'FILE X Y W H'")
            .nargs(1)
            .help("output clipped image");

    if (argc == 1) {
        parser.print_help();
        return 0;
    }

    auto const args = parser.parse_args();

    auto const input = args.get<std::string>("i");

    if (!detail::_is_file_exists(input)) {
        std::cerr << "[FAIL] Input atlas file '" + input + "' not found" << std::endl;
        return 1;
    }
    Image image;
    if (!image.load(input)) {
        std::cerr << "[FAIL] Can't load atlas file '" + input + "' as image" << std::endl;
        return 2;
    }

    auto const outputs = args.get<std::vector<Atlas> >("o");

    for (auto const& atlas : outputs) {
        auto output = atlas.name;
        if (!detail::_ends_with(output, ".png")) {
            output += ".png";
        }
        auto x = atlas.x;
        auto y = atlas.y;
        auto w = atlas.w;
        auto h = atlas.h;

        auto subImage = image.subImage(x, y, w, h);

        auto dir = detail::_directory_name(output);
        if (!detail::_is_directory_exists(dir)) {
            auto res = detail::_make_directory(dir);
            if (res != 0) {
                std::cerr << "[FAIL] Can't create directory '" << dir
                          << "' for output file '" << output << "'" << std::endl;
                return res;
            }
        }
        subImage.save_png(output);
        std::cout << "[ OK ] File '" << output << "' generated" << std::endl;
    }

    return 0;
}
