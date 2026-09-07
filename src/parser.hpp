# pragma once

#include <optional>
#include "record.hpp"

std::optional<Record> parse_record(std::string& text);