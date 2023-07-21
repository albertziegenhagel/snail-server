#pragma once

#include <array>
#include <bit>
#include <format>
#include <istream>
#include <span>
#include <stdexcept>
#include <type_traits>

namespace snail::common {

template<std::size_t ChunkSize>
class chunked_reader
{
public:
    chunked_reader(std::istream& stream, std::size_t offset, std::size_t desired_size) :
        stream_(stream),
        total_size_(desired_size)
    {
        stream_.seekg(offset);
        if(!stream_.good() || stream_.tellg() != std::streampos(offset))
        {
            throw std::runtime_error("Failed to move to offset");
        }
    }

    bool read_next_chunk()
    {
        // keep left over data from last chunk
        assert(chunk_processed_size_ <= current_chunk_data_.size());
        const auto last_chunk_remaining_bytes = current_chunk_data_.size() - chunk_processed_size_;

        for(std::size_t i = 0; i < last_chunk_remaining_bytes; ++i)
        {
            chunk_buffer_[i] = chunk_buffer_[chunk_buffer_.size() - last_chunk_remaining_bytes + i];
        }

        total_processed_size_ += chunk_processed_size_;

        // read new chunk
        const auto remaining_data_bytes = total_size_ - total_processed_size_;

        if(remaining_data_bytes == 0)
        {
            return false; // there is no more data left. We are done.
        }

        if(!stream_.good())
        {
            throw std::runtime_error("Failed to read chunk from stream: stream is an invalid state.");
        }

        const auto remaining_data_bytes_to_read = remaining_data_bytes - last_chunk_remaining_bytes;

        const auto max_remaining_buffer_size = chunk_buffer_.size() - last_chunk_remaining_bytes;

        const auto chunk_bytes_to_read = remaining_data_bytes_to_read > max_remaining_buffer_size ?
                                             max_remaining_buffer_size :
                                             remaining_data_bytes_to_read;

        const auto initial_pos = stream_.tellg();
        stream_.read(reinterpret_cast<char*>(chunk_buffer_.data() + last_chunk_remaining_bytes), chunk_bytes_to_read);
        const auto chunk_read_bytes = stream_.tellg() - initial_pos;

        if(chunk_read_bytes != static_cast<std::streamoff>(chunk_bytes_to_read))
        {
            throw std::runtime_error(std::format(
                "ERROR: Could not read from stream: Expected to read {} bytes but only got {}.",
                chunk_bytes_to_read,
                chunk_read_bytes));
        }

        current_chunk_data_ = std::span(chunk_buffer_.data(), chunk_read_bytes + last_chunk_remaining_bytes);

        chunk_processed_size_ = 0;
        is_chunk_exhausted_   = false;

        return true;
    }

    std::span<const std::byte> retrieve_data(std::size_t size, bool peek = false)
    {
        assert(size <= (total_size_ - total_processed_size_));

        const auto chunk_remaining_bytes = current_chunk_data_.size() - chunk_processed_size_;
        if(chunk_remaining_bytes < size)
        {
            // not enough data
            is_chunk_exhausted_ = true;
            return {};
        }

        const auto result_data = current_chunk_data_.subspan(chunk_processed_size_, size);

        if(!peek) chunk_processed_size_ += size;

        return result_data;
    }

    bool done() const
    {
        assert(total_processed_size_ <= total_size_);
        return total_processed_size_ == total_size_;
    }

    bool chunk_has_more_data() const
    {
        return chunk_processed_size_ < current_chunk_data_.size();
    }

    bool keep_going()
    {
        if(done()) return false; // We are already done

        // The current chunk might be finished. Try to read the next one.
        if(is_chunk_exhausted_ || !chunk_has_more_data())
        {
            if(!read_next_chunk())
            {
                // The current chunk was the final one. There is no more data to read. We are now done.
                assert(done());
                return false;
            }
        }

        // There is still data in the current chunk, or we have read a new one.
        return true;
    }

private:
    std::istream& stream_;

    std::array<std::byte, ChunkSize> chunk_buffer_;

    std::size_t total_size_;
    std::size_t total_processed_size_ = 0;

    std::span<const std::byte> current_chunk_data_;

    std::size_t chunk_processed_size_ = 0;
    bool        is_chunk_exhausted_   = true;
};

} // namespace snail::common
