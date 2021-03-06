//
// Created by richard on 2020-09-25.
//

#pragma once

#include <memory>
#include <chrono>
#include <cstdint>
#include <future>
#include <utility>
#include <vector>
#include <SDL.h>
#include <sdlgui/theme.h>

namespace sdlgui {
    using namespace std;

#ifdef DISPLAY800X480
    static constexpr int DISPLAY_WIDTH = 800;
    static constexpr int DISPLAY_HEIGHT = 480;
    static constexpr int EARTH_BIG_W = 660;
    static constexpr int EARTH_BIG_H = 330;
    static constexpr std::string_view EARTH_BIG_S = "660x330";
#define EARTH_BIG_S 660x330
#else
    static constexpr int DISPLAY_WIDTH = 800;
    static constexpr int DISPLAY_HEIGHT = 480;
    static constexpr int EARTH_BIG_W = 660;
    static constexpr int EARTH_BIG_H = 330;
#define EARTH_BIG_S 660x330
#endif

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    static constexpr uint32_t rmask = 0xff000000;
    static constexpr uint32_t rshift = 24U;
    static constexpr uint32_t gmask = 0x00ff0000;
    static constexpr uint32_t gshift = 16U;
    static constexpr uint32_t bmask = 0x0000ff00;
    static constexpr uint32_t bshift = 8U;
    static constexpr uint32_t amask = 0x000000ff;
    static constexpr uint32_t ashift = 0U;
    static constexpr uint32_t cmask = rmask | gmask | bmask;
#else
    static constexpr uint32_t rmask = 0x000000ff;
    static constexpr uint32_t rshift = 0U;
    static constexpr uint32_t gmask = 0x0000ff00;
    static constexpr uint32_t gshift = 8U;
    static constexpr uint32_t bmask = 0x00ff0000;
    static constexpr uint32_t bshift = 16U;
    static constexpr uint32_t amask = 0xff000000;
    static constexpr uint32_t ashift = 24U;
    static constexpr uint32_t cmask = rmask | gmask | bmask;
#endif

    static constexpr uint32_t set_a_value(uint32_t pixel, uint32_t a) { return (pixel & cmask) | a << ashift; }

    struct TextureDeleter {
        void operator()(SDL_Texture *texture) {
            SDL_DestroyTexture(texture);
        }
    };

    typedef unique_ptr<SDL_Texture, TextureDeleter> TexturePtr;

    struct ImageData {
    private:
        SDL_Texture *tex{nullptr};

    public:
        int w{}, h{};
        string path{};
        string name{};
        bool dirty{true};
        std::chrono::time_point<std::chrono::system_clock> loaded{}, life{};

        ~ImageData() {
            if (tex)
                SDL_DestroyTexture(tex);
        }

        ImageData() = default;

        ImageData(ImageData &) = delete;
        ImageData(const ImageData &) = delete;
        ImageData &operator=(ImageData &) = delete;
        ImageData &operator=(const ImageData &) = delete;

        explicit ImageData(Texture &&texture) noexcept {
            tex = texture.tex;
            texture.tex = nullptr;
            w = texture.w();
            h = texture.h();
            dirty = false;
        }

        ImageData(ImageData &&other) noexcept {
            tex = other.tex;
            other.tex = nullptr;
            path = std::move(other.path);
            name = std::move(other.name);
            w = other.w;
            h = other.h;
            dirty = other.dirty;
        }

        ImageData &operator=(ImageData && other) noexcept {
            w = other.w;
            h = other.h;
            tex = other.tex;
            other.tex = nullptr;
            path = std::move(other.path);
            name = std::move(other.name);
            dirty = other.dirty;
            return *this;
        }

        void set(SDL_Texture *texture) {
            if (tex)
                SDL_DestroyTexture(tex);
            tex = texture;
            SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
            dirty = false;
        }

        SDL_Texture* get() {
            return tex;
        }

        explicit operator bool() const { return tex != nullptr; }
    };

    typedef vector<ImageData> ImageDataList;
}
