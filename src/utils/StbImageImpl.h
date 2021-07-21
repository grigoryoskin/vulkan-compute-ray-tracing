#pragma once

#include <string>

class StbImageImpl {
    public:
        unsigned char* pixels;

        StbImageImpl(std::string path, int &texWidth, int &texHeight, int &texChannels);

        ~StbImageImpl();
};
