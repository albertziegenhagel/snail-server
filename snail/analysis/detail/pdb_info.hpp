#pragma once

#include <cstdint>
#include <string>

#include <snail/common/guid.hpp>

namespace snail::analysis::detail {

struct pdb_info
{
    std::string   pdb_name;
    common::guid  guid;
    std::uint32_t age;

    [[nodiscard]] friend bool operator==(const pdb_info& lhs, const pdb_info& rhs)
    {
        return lhs.pdb_name == rhs.pdb_name &&
               lhs.guid == rhs.guid &&
               lhs.age == rhs.age;
    }
};

} // namespace snail::analysis::detail
