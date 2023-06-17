#pragma once

#include <memory>
#include <regex>
#include <string>
#include <vector>

namespace snail::analysis {

class path_mapper
{
public:
    virtual ~path_mapper() = default;

    virtual bool try_apply(std::string& input) const = 0;

    virtual std::unique_ptr<path_mapper> clone() const = 0;
};

class path_map
{
public:
    path_map() = default;
    path_map(const path_map& other);
    path_map(path_map&& other) noexcept = default;

    path_map& operator=(const path_map& other);
    path_map& operator=(path_map&& other) noexcept = default;

    void add_rule(std::unique_ptr<path_mapper> mappers);

    bool try_apply(std::string& input) const;

private:
    std::vector<std::unique_ptr<path_mapper>> mappers_;
};

class simple_path_mapper : public path_mapper
{
public:
    explicit simple_path_mapper(std::string pattern, std::string replace_text);

    bool try_apply(std::string& input) const override;

    std::unique_ptr<path_mapper> clone() const override;

private:
    std::string pattern_;
    std::string replace_text_;
};

class regex_path_mapper : public path_mapper
{
public:
    explicit regex_path_mapper(std::regex pattern, std::string replace_text);

    bool try_apply(std::string& input) const override;

    std::unique_ptr<path_mapper> clone() const override;

private:
    std::regex  pattern_;
    std::string replace_text_;
};

} // namespace snail::analysis
