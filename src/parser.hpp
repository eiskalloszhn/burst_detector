# pragma once

#include "record.hpp"

#include <optional>
#include <string>


std::optional<Record> parse_record(std::string& text);