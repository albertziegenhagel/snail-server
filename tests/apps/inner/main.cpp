
#include <cassert>
#include <charconv>
#include <iostream>
#include <random>
#include <string_view>
#include <vector>

inline constexpr std::size_t default_work_size = 1'000'000;

void make_random_vector(std::vector<double>& data, std::size_t size)
{
    data.reserve(size);

    static std::random_device random_device;

    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    std::mt19937                           engine(random_device());

    for(std::size_t i = 0; i < size; ++i)
    {
        data.push_back(distribution(engine));
    }
}

double compute_inner_product(const std::vector<double>& lhs,
                             const std::vector<double>& rhs)
{
    assert(lhs.size() == rhs.size());

    double result = 0.0;

    const auto size = lhs.size();
    for(std::size_t i = 0; i < size; ++i)
    {
        result += lhs[i] * rhs[i];
    }
    return result;
}

std::size_t get_work_size(int argc, char* argv[])
{
    if(argc < 2) return default_work_size;

    const auto work_size_arg = std::string_view(argv[1]);

    const auto* const work_size_arg_begin = work_size_arg.data();
    const auto* const work_size_arg_end   = work_size_arg.data() + work_size_arg.size();

    std::size_t work_size;
    auto        result = std::from_chars(work_size_arg_begin, work_size_arg_end, work_size);
    if(result.ec != std::errc{} || result.ptr != work_size_arg_end) return default_work_size;

    return work_size;
}

int main(int argc, char* argv[])
{
    // Parse the first argument.
    // If it is present and a valid integer, use it as work-size.
    // Otherwise fall-back to using `default_work_size`.
    const auto work_size = get_work_size(argc, argv);

    std::cout << "Using work size: " << work_size << "\n";

    std::vector<double> lhs;
    std::vector<double> rhs;

    make_random_vector(lhs, work_size);
    make_random_vector(rhs, work_size);

    const auto result = compute_inner_product(lhs, rhs);

    std::cout << "Inner product of random vectors: " << result << "\n";

    return 0;
}
