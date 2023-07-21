#pragma once

#include <istream>

namespace snail::common {

struct stream_position_resetter
{
    stream_position_resetter(std::istream& stream) :
        stream_(stream),
        pos_(stream.tellg())
    {}

    ~stream_position_resetter()
    {
        stream_.seekg(pos_);
    }

private:
    std::istream&  stream_;
    std::streampos pos_;
};

} // namespace snail::common
