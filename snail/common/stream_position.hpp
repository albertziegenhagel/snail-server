#pragma once

#include <fstream>

namespace snail::common {

struct stream_position_resetter
{
    stream_position_resetter(std::ifstream& stream) :
        stream_(stream),
        pos_(stream.tellg())
    {}

    ~stream_position_resetter()
    {
        stream_.seekg(pos_);
    }

private:
    std::ifstream& stream_;
    std::streampos pos_;
};

} // namespace snail::common
