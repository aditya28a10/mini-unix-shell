#ifndef MINI_SHELL_PARSER_HPP
#define MINI_SHELL_PARSER_HPP

#include <string>
#include "command.hpp"

namespace minishell {

// Parses an input line into a Command by splitting on whitespace.
Command parse_command(const std::string& line);

} // namespace minishell

#endif // MINI_SHELL_PARSER_HPP
