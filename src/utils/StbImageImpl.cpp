#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "StbImageImpl.h"

StbImageImpl::StbImageImpl(std::string path, int &texWidth, int &texHeight, int &texChannels) {
    pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
}

StbImageImpl::~StbImageImpl() {
    stbi_image_free(pixels);
}