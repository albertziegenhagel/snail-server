#pragma once

#include <snail/common/progress.hpp>

class progress_printer : public snail::common::progress_listener
{
public:
    progress_printer();

    virtual void start() const override;

    virtual void report(double progress) const override;

    virtual void finish() const override;
};
