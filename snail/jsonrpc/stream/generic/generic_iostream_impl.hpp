
#include <snail/jsonrpc/stream/generic/generic_iostream.hpp>

namespace snail::jsonrpc {

template<typename StreamBufType>
generic_iostream<StreamBufType>::generic_iostream() :
    std::iostream(new StreamBufType())
{}

template<typename StreamBufType>
generic_iostream<StreamBufType>::generic_iostream(const std::filesystem::path& path, std::ios_base::openmode mode) :
    std::iostream(new StreamBufType(path, mode))
{
    if(!static_cast<StreamBufType*>(rdbuf())->is_open())
    {
        this->setstate(std::ios_base::failbit);
    }
}

template<typename StreamBufType>
generic_iostream<StreamBufType>::generic_iostream(generic_iostream&& other) :
    std::iostream(new StreamBufType())
{
    this->swap(other);
    auto* const other_buffer = other.rdbuf();
    other.set_rdbuf(rdbuf());
    this->set_rdbuf(other_buffer);
}

template<typename StreamBufType>
generic_iostream<StreamBufType>::~generic_iostream()
{
    delete rdbuf();
}

template<typename StreamBufType>
generic_iostream<StreamBufType>& generic_iostream<StreamBufType>::operator=(generic_iostream&& other)
{
    delete rdbuf();
    this->set_rdbuf(nullptr); // set to null first so that close() is valid if the following new fails
    this->set_rdbuf(new StreamBufType());

    this->swap(other);

    auto* const other_buffer = other.rdbuf();
    other.set_rdbuf(rdbuf());
    this->set_rdbuf(other_buffer);

    return *this;
}

template<typename StreamBufType>
void generic_iostream<StreamBufType>::open(const std::filesystem::path& path, std::ios_base::openmode mode)
{
    static_cast<StreamBufType*>(rdbuf())->open(path, mode);
    if(!static_cast<StreamBufType*>(rdbuf())->is_open())
    {
        this->setstate(std::ios_base::failbit);
    }
}

template<typename StreamBufType>
bool generic_iostream<StreamBufType>::is_open() const
{
    return static_cast<StreamBufType*>(rdbuf())->is_open();
}

template<typename StreamBufType>
void generic_iostream<StreamBufType>::close()
{
    static_cast<StreamBufType*>(rdbuf())->close();
}

} // namespace snail::jsonrpc
