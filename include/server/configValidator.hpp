#ifndef CONFIG_VALIDATOR_HPP
#define CONFIG_VALIDATOR_HPP

#include <string>
#include <vector>

#define UPPER_PORT_LIMIT 65535
namespace ConfigValidator {
/**
 * @brief Validates the structure of the command-line arguments.
 * @param args The vector of command-line arguments.
 * @return True if the arguments are structurally valid, false otherwise.
 */
bool validateArguments(const std::vector<std::string> &args);
} // namespace ConfigValidator
#endif // CONFIG_VALIDATOR_HPP