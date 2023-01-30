
#include <snail/jsonrpc/stream/windows/pipe_iostream.hpp>

#include <snail/jsonrpc/stream/windows/pipe_streambuf.hpp>

using namespace snail::jsonrpc;

pipe_iostream::pipe_iostream() :
    std::iostream(new pipe_streambuf())
{}

pipe_iostream::pipe_iostream(const std::filesystem::path& path, std::ios_base::openmode mode) :
    std::iostream(new pipe_streambuf(path, mode))
{}

pipe_iostream::pipe_iostream(pipe_iostream&& other) :
    std::iostream(new pipe_streambuf())
{
    this->swap(other);
    auto* const other_buffer = other.rdbuf();
    other.set_rdbuf(rdbuf());
    this->set_rdbuf(other_buffer);
}

pipe_iostream::~pipe_iostream()
{
    delete rdbuf();
}

pipe_iostream& pipe_iostream::operator=(pipe_iostream&& other)
{
    delete rdbuf();
    this->set_rdbuf(nullptr); // set to null first so that close() is valid if the following new fails
    this->set_rdbuf(new pipe_streambuf());

    this->swap(other);

    auto* const other_buffer = other.rdbuf();
    other.set_rdbuf(rdbuf());
    this->set_rdbuf(other_buffer);

    return *this;
}

void pipe_iostream::open(const std::filesystem::path& path, std::ios_base::openmode mode)
{
    static_cast<pipe_streambuf*>(rdbuf())->open(path, mode);
    if(!static_cast<pipe_streambuf*>(rdbuf())->is_open())
    {
        this->setstate(std::ios_base::failbit);
    }
}

bool pipe_iostream::is_open() const
{
    return static_cast<pipe_streambuf*>(rdbuf())->is_open();
}

void pipe_iostream::close()
{
    static_cast<pipe_streambuf*>(rdbuf())->close();
}
