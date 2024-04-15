
#include <algorithm>
#include <cassert>
#include <charconv>
#include <iostream>
#include <numeric>
#include <random>
#include <string_view>
#include <vector>

inline constexpr std::size_t default_work_size = 10'000'000;

std::vector<std::size_t> make_random_order(std::size_t size)
{
    std::vector<std::size_t> order(size);
    std::iota(order.begin(), order.end(), 0);

    std::ranlux24_base engine{42};
    std::ranges::shuffle(order, engine);

    return order;
}

std::vector<std::size_t> make_predicable_order(std::size_t size)
{
    std::vector<std::size_t> order(size);
    std::iota(order.begin(), order.end(), 0);

    for(std::size_t i = 0; i < (size / 3); ++i)
    {
        std::swap(order[i * 3], order[i * 3 + 3]);
    }
    return order;
}

void add_sequential(std::vector<double>&       result,
                    const std::vector<double>& lhs,
                    const std::vector<double>& rhs);

void add_order_random(std::vector<double>&            result,
                      const std::vector<double>&      lhs,
                      const std::vector<double>&      rhs,
                      const std::vector<std::size_t>& order);

void add_order_predicable(std::vector<double>&            result,
                          const std::vector<double>&      lhs,
                          const std::vector<double>&      rhs,
                          const std::vector<std::size_t>& order);

void add_even_subtract_odd_random(std::vector<double>&            result,
                                  const std::vector<double>&      lhs,
                                  const std::vector<double>&      rhs,
                                  const std::vector<std::size_t>& order);

void add_even_subtract_odd_predicable(std::vector<double>&            result,
                                      const std::vector<double>&      lhs,
                                      const std::vector<double>&      rhs,
                                      const std::vector<std::size_t>& order);

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

    if(work_size == 0) return 1;

    std::vector<double> a(work_size);
    std::vector<double> b(work_size);
    std::vector<double> c(work_size);

    const auto random_order     = make_random_order(work_size);
    const auto predicable_order = make_predicable_order(work_size);

    std::ranges::fill(a, 1.2345);
    std::ranges::fill(b, 6.7890);

    add_sequential(c, a, b);

    add_order_random(a, b, c, random_order);

    add_order_predicable(b, c, a, predicable_order);

    add_even_subtract_odd_random(b, c, a, random_order);

    add_even_subtract_odd_predicable(c, a, b, predicable_order);

    std::cout << "Final value: " << c.back() << "\n";

    return 0;
}
