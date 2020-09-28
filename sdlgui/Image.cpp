//
// Created by richard on 2020-09-25.
//
#ifdef HAS_FILESYSTEM
#include <filesystem>
#else
#include <dirent.h>
#endif
#include <SDL_image.h>
#include "Image.h"

namespace sdlgui {
#if 0
    SDL_Rect clip_rects(SDL_Rect af, const SDL_Rect& bf) {
        PntRect a{ af.x, af.y, af.x + af.w, af.y + af.h };
        PntRect b{ bf.x, bf.y, bf.x + bf.w, bf.y + bf.h };
        if (a.x1 < b.x1)
            a.x1 = b.x1;
        if (a.y1 < b.y1)
            a.y1 = b.y1;
        if (b.x2 < a.x2)
            a.x2 = b.x2;
        if (b.y2 < a.y2)
            a.y2 = b.y2;

        return { a.x1, a.y1, a.x2 - a.x1, a.y2 - a.y1 };
    }

    PntRect clip_rects(PntRect a, const PntRect& b)
    {
        if (a.x1 < b.x1)
            a.x1 = b.x1;
        if (a.y1 < b.y1)
            a.y1 = b.y1;
        if (b.x2 < a.x2)
            a.x2 = b.x2;
        if (b.y2 < a.y2)
            a.y2 = b.y2;

        return a;
    }
    PntRect srect2pntrect(const SDL_Rect& srect){
        return{ srect.x, srect.y, srect.x + srect.w, srect.y + srect.h };
    }

    SDL_Rect pntrect2srect(const PntRect& frect) {
        return{ frect.x1, frect.y1, frect.x2 - frect.x1, frect.y2 - frect.y1 };
    }
#endif

    ImageData loadImage(SDL_Renderer *renderer, const std::string &path) {
        ImageData result;

        result.path = path;
        result.set(IMG_LoadTexture(renderer, result.path.c_str()));
        if (!result) {
            throw std::runtime_error("Could not open image data!");
        }

        return result;
    }

    bool hasEnding (std::string const &fullString, std::string const &ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }

    ImageDataList loadImageDataDirectory(SDL_Renderer *renderer, const std::string &path) {
        ImageDataList result;

#ifdef HAS_FILESYSTEM
        namespace fs = std::filesystem;

        const fs::path imagePath{path};
        for (const auto &entry : fs::directory_iterator(imagePath)) {
            if (entry.is_regular_file()) {
                const auto ext = entry.path().filename().extension().string();
                auto name = entry.path().filename().string();
                if (ext == ".png" || ext == ".jpg") {
                    auto imageInfo = loadImage(renderer, imagePath.string() + entry.path().filename().string());
                    if (imageInfo.get()) {
                        result.push_back(std::move(imageInfo));
                    } else {
                        cerr << "Could not open file \"" << imageInfo.path << "\"\n";
                    }
                }
            }
        }
#else
        DIR *dp = opendir(path.c_str());
        if (!dp)
            throw std::runtime_error("Could not open image directory!");
        struct dirent *ep;
        while ((ep = readdir(dp))) {
            auto fileName = string(ep->d_name);
            if (hasEnding(fileName, ".png") || hasEnding(fileName, ".jpg")) {
                std::string fullName = path + "/" + std::string(fileName);
                auto imageInfo = loadImage(renderer, fullName);
                imageInfo.name = fileName;
                result.push_back(std::move(imageInfo));
            }
#endif
        }
        return result;
    }

}
