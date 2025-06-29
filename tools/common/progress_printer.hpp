#pragma once

#include <snail/common/progress.hpp>

class progress_printer : public snail::common::progress_listener
{
public:
    progress_printer();

    virtual void start(std::string_view                title,
                       std::optional<std::string_view> message) const override;

    virtual void report(double                          progress,
                        std::optional<std::string_view> message) const override;

    virtual void finish(std::optional<std::string_view> message) const override;
};
