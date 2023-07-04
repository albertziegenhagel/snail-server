#include <snail/analysis/path_map.hpp>

using namespace snail::analysis;

path_map::path_map(const path_map& other)
{
    mappers_.reserve(other.mappers_.size());
    for(const auto& mapper : other.mappers_)
    {
        mappers_.push_back(mapper->clone());
    }
}

path_map& path_map::operator=(const path_map& other)
{
    mappers_.clear();
    mappers_.reserve(other.mappers_.size());
    for(const auto& mapper : other.mappers_)
    {
        mappers_.push_back(mapper->clone());
    }
    return *this;
}

void path_map::add_rule(std::unique_ptr<path_mapper> mapper)
{
    mappers_.push_back(std::move(mapper));
}

bool path_map::try_apply(std::string& input) const
{
    for(const auto& mapper : mappers_)
    {
        if(mapper->try_apply(input)) return true;
    }
    return false;
}

simple_path_mapper::simple_path_mapper(std::string pattern, std::string replace_text) :
    pattern_(std::move(pattern)),
    replace_text_(std::move(replace_text))
{}

bool simple_path_mapper::try_apply(std::string& input) const
{
    if(!input.starts_with(pattern_)) return false;

    input.replace(input.begin(), input.begin() + pattern_.size(), replace_text_);
    return true;
}

std::unique_ptr<path_mapper> simple_path_mapper::clone() const
{
    return std::make_unique<simple_path_mapper>(*this);
}

regex_path_mapper::regex_path_mapper(std::regex pattern, std::string replace_text) :
    pattern_(std::move(pattern)),
    replace_text_(std::move(replace_text))
{}

bool regex_path_mapper::try_apply(std::string& input) const
{
    auto output = std::regex_replace(input, pattern_, replace_text_);
    if(output != input)
    {
        input = std::move(output);
        return true;
    }
    return false;
}

std::unique_ptr<path_mapper> regex_path_mapper::clone() const
{
    return std::make_unique<regex_path_mapper>(*this);
}
