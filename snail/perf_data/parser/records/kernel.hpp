#pragma once

#include <cstdint>

#include <optional>
#include <type_traits>
#include <vector>

#include <snail/perf_data/parser/event.hpp>
#include <snail/perf_data/parser/event_attributes.hpp>

namespace snail::perf_data::parser {

struct sample_id
{
    std::optional<std::uint32_t> pid;
    std::optional<std::uint32_t> tid;

    std::optional<std::uint64_t> time;

    std::optional<std::uint64_t> id;

    std::optional<std::uint64_t> stream_id;

    std::optional<std::uint32_t> cpu;
    std::optional<std::uint32_t> res;
};

sample_id parse_sample_id(const sample_format_flags& sample_format,
                          std::span<const std::byte> buffer,
                          std::endian                byte_order);

sample_id parse_sample_id_back(const sample_format_flags& sample_format,
                               std::span<const std::byte> buffer,
                               std::endian                byte_order);

struct kernel_event_view : protected parser::attribute_event_view_base
{
    using attribute_event_view_base::attribute_event_view_base;
    using attribute_event_view_base::buffer;
    using attribute_event_view_base::header;

    inline auto sample_id() const
    {
        return parse_sample_id_back(attributes().sample_format, buffer(), byte_order());
    }
};

struct comm_event_view : private kernel_event_view
{
    static inline constexpr parser::event_type event_type = parser::event_type::comm;

    using kernel_event_view::buffer;
    using kernel_event_view::header;
    using kernel_event_view::kernel_event_view;

    inline auto pid() const { return extract<std::uint32_t>(0); }
    inline auto tid() const { return extract<std::uint32_t>(4); }

    inline auto comm() const { return extract_string(8, comm_length); }

    using kernel_event_view::sample_id;

private:
    mutable std::optional<std::size_t> comm_length;
};

struct exit_event_view : private kernel_event_view
{
    static inline constexpr parser::event_type event_type = parser::event_type::exit;

    using kernel_event_view::buffer;
    using kernel_event_view::header;
    using kernel_event_view::kernel_event_view;

    inline auto pid() const { return extract<std::uint32_t>(0); }
    inline auto ppid() const { return extract<std::uint32_t>(4); }

    inline auto tid() const { return extract<std::uint32_t>(8); }
    inline auto ptid() const { return extract<std::uint32_t>(12); }

    inline auto time() const { return extract<std::uint64_t>(16); }

    using kernel_event_view::sample_id;

    static inline constexpr std::size_t static_size = 24; // without sample_id
};

struct fork_event_view : private kernel_event_view
{
    static inline constexpr parser::event_type event_type = parser::event_type::fork;

    using kernel_event_view::buffer;
    using kernel_event_view::header;
    using kernel_event_view::kernel_event_view;

    inline auto pid() const { return extract<std::uint32_t>(0); }
    inline auto ppid() const { return extract<std::uint32_t>(4); }

    inline auto tid() const { return extract<std::uint32_t>(8); }
    inline auto ptid() const { return extract<std::uint32_t>(12); }

    inline auto time() const { return extract<std::uint64_t>(16); }

    using kernel_event_view::sample_id;

    static inline constexpr std::size_t static_size = 24; // without sample_id
};

struct mmap2_event_view : private kernel_event_view
{
    static inline constexpr parser::event_type event_type = parser::event_type::mmap2;

    using kernel_event_view::buffer;
    using kernel_event_view::header;
    using kernel_event_view::kernel_event_view;

    inline auto pid() const { return extract<std::uint32_t>(0); }
    inline auto tid() const { return extract<std::uint32_t>(4); }

    inline auto addr() const { return extract<std::uint64_t>(8); }
    inline auto len() const { return extract<std::uint64_t>(16); }
    inline auto pgoff() const { return extract<std::uint64_t>(24); }

    inline bool has_build_id() const
    {
        return (header().misc() & std::to_underlying(perf_data::parser::header_misc_mask::mmap_build_id)) != 0;
    }

    inline auto maj() const
    {
        assert(!has_build_id());
        return extract<std::uint32_t>(32);
    }
    inline auto min() const
    {
        assert(!has_build_id());
        return extract<std::uint32_t>(36);
    }
    inline auto ino() const
    {
        assert(!has_build_id());
        return extract<std::uint64_t>(40);
    }
    inline auto ino_generation() const
    {
        assert(!has_build_id());
        return extract<std::uint64_t>(48);
    }

    inline auto build_id() const
    {
        assert(has_build_id());
        const auto build_id_size = extract<std::uint8_t>(32);
        assert(build_id_size <= 20);
        return buffer().subspan(36, build_id_size);
    }

    inline auto prot() const { return extract<std::uint32_t>(56); }
    inline auto flags() const { return extract<std::uint32_t>(60); }

    inline auto filename() const { return extract_string(64, filename_length); }

    using kernel_event_view::sample_id;

private:
    mutable std::optional<std::size_t> filename_length;
};

enum class sample_stack_context_marker : std::uint64_t
{
    hv     = static_cast<std::uint64_t>(-32),
    kernel = static_cast<std::uint64_t>(-128),
    user   = static_cast<std::uint64_t>(-512),

    guest        = static_cast<std::uint64_t>(-2048),
    guest_kernel = static_cast<std::uint64_t>(-2176),
    guest_user   = static_cast<std::uint64_t>(-2560),

    max = static_cast<std::uint64_t>(-4095),
};

struct sample_event
{
    static inline constexpr parser::event_type event_type = parser::event_type::sample;

    std::optional<std::uint64_t> id;
    std::optional<std::uint64_t> ip;

    std::optional<std::uint32_t> pid;
    std::optional<std::uint32_t> tid;

    std::optional<std::uint64_t> time;
    std::optional<std::uint64_t> addr;

    std::optional<std::uint64_t> stream_id;

    std::optional<std::uint32_t> cpu;
    std::optional<std::uint32_t> res;

    std::optional<std::uint64_t> period;

    //  *	{ struct read_format	values;	  } && PERF_SAMPLE_READ

    std::optional<std::vector<std::uint64_t>> ips;

    std::optional<std::vector<std::uint8_t>> data;

    //  *	{ u64                   nr;
    //  *	  { u64	hw_idx; } && PERF_SAMPLE_BRANCH_HW_INDEX
    //  *        { u64 from, to, flags } lbr[nr];
    //  *      } && PERF_SAMPLE_BRANCH_STACK
    //  *
    //  * 	{ u64			abi; # enum perf_sample_regs_abi
    //  * 	  u64			regs[weight(mask)]; } && PERF_SAMPLE_REGS_USER
    //  *
    //  * 	{ u64			size;
    //  * 	  char			data[size];
    //  * 	  u64			dyn_size; } && PERF_SAMPLE_STACK_USER
    //  *
    //  *	{ union perf_sample_weight
    //  *	 {
    //  *		u64		full; && PERF_SAMPLE_WEIGHT
    //  *	#if defined(__LITTLE_ENDIAN_BITFIELD)
    //  *		struct {
    //  *			u32	var1_dw;
    //  *			u16	var2_w;
    //  *			u16	var3_w;
    //  *		} && PERF_SAMPLE_WEIGHT_STRUCT
    //  *	#elif defined(__BIG_ENDIAN_BITFIELD)
    //  *		struct {
    //  *			u16	var3_w;
    //  *			u16	var2_w;
    //  *			u32	var1_dw;
    //  *		} && PERF_SAMPLE_WEIGHT_STRUCT
    //  *	#endif
    //  *	 }
    //  *	}
    //  *	{ u64			data_src; } && PERF_SAMPLE_DATA_SRC
    //  *	{ u64			transaction; } && PERF_SAMPLE_TRANSACTION
    //  *	{ u64			abi; # enum perf_sample_regs_abi
    //  *	  u64			regs[weight(mask)]; } && PERF_SAMPLE_REGS_INTR
    //  *	{ u64			phys_addr;} && PERF_SAMPLE_PHYS_ADDR
    //  *	{ u64			size;
    //  *	  char			data[size]; } && PERF_SAMPLE_AUX
    //  *	{ u64			data_page_size;} && PERF_SAMPLE_DATA_PAGE_SIZE
    //  *	{ u64			code_page_size;} && PERF_SAMPLE_CODE_PAGE_SIZE
};

template<>
sample_event parse_event(const event_attributes&    attributes,
                         std::span<const std::byte> buffer,
                         std::endian                byte_order);

} // namespace snail::perf_data::parser
