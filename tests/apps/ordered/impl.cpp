
#include <cassert>
#include <vector>

void add_sequential(std::vector<double>&       result,
                    const std::vector<double>& lhs,
                    const std::vector<double>& rhs)
{
    assert(result.size() == lhs.size());
    assert(lhs.size() == rhs.size());

    const auto size = lhs.size();
    for(std::size_t i = 0; i < size; ++i)
    {
        result[i] = lhs[i] + rhs[i];
    }
}

void add_order_random(std::vector<double>&            result,
                      const std::vector<double>&      lhs,
                      const std::vector<double>&      rhs,
                      const std::vector<std::size_t>& order)
{
    assert(result.size() == lhs.size());
    assert(lhs.size() == rhs.size());
    assert(lhs.size() == order.size());

    for(const auto i : order)
    {
        result[i] = lhs[i] + rhs[i];
    }
}

void add_order_predicable(std::vector<double>&            result,
                          const std::vector<double>&      lhs,
                          const std::vector<double>&      rhs,
                          const std::vector<std::size_t>& order)
{
    assert(result.size() == lhs.size());
    assert(lhs.size() == rhs.size());
    assert(lhs.size() == order.size());

    for(const auto i : order)
    {
        result[i] = lhs[i] + rhs[i];
    }
}

void add_even_subtract_odd_random(std::vector<double>&            result,
                                  const std::vector<double>&      lhs,
                                  const std::vector<double>&      rhs,
                                  const std::vector<std::size_t>& order)
{
    assert(result.size() == lhs.size());
    assert(lhs.size() == rhs.size());

    const auto size = lhs.size();
    for(std::size_t i = 0; i < size; ++i)
    {
        if(order[i] % 2 == 0)
        {
            result[i] = lhs[i] + rhs[i];
        }
        else
        {
            result[i] = lhs[i] - rhs[i];
        }
    }
}

void add_even_subtract_odd_predicable(std::vector<double>&            result,
                                      const std::vector<double>&      lhs,
                                      const std::vector<double>&      rhs,
                                      const std::vector<std::size_t>& order)
{
    assert(result.size() == lhs.size());
    assert(lhs.size() == rhs.size());

    const auto size = lhs.size();
    for(std::size_t i = 0; i < size; ++i)
    {
        if(order[i] % 2 == 0)
        {
            result[i] = lhs[i] + rhs[i];
        }
        else
        {
            result[i] = lhs[i] - rhs[i];
        }
    }
}
