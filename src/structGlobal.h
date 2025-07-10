#pragma once
#include <vector>
#include <string>
#include <memory>

struct lSynthesizeArgs
{
    std::unique_ptr<std::string> text;
    std::unique_ptr<std::string> voice_id;
};

struct SynthesizeArgs
{
    std::string *text;
    std::string *lang;
};