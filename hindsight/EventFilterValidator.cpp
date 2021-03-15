#include "EventFilterValidator.hpp"
#include "String.hpp"

using namespace Hindsight::Cli::CliValidator;

/// <summary>
/// A static collection of valid event names.
/// </summary>
std::set<std::string> EventFilterValidator::Valid = {
	"create_process", "create_thread", "exit_process", "exit_thread", "breakpoint", "exception", "load_dll", "unload_dll", "rip", "debug"
};

/// <summary>
/// The static instance of the validator, for convenience.
/// </summary>
EventFilterValidator EventFilterValidator::Validator = EventFilterValidator();

/// <summary>
/// Construct a new EventFilterValidator
/// </summary>
EventFilterValidator::EventFilterValidator() {
	tname = "EVENT";
	func = [](const std::string& str) -> std::string {
		if (!Valid.count(str))
			return "Invalid event specified: " + str;
		return std::string();
	};
}

/// <summary>
/// Get a comma-separated list of valid options for this validator.
/// </summary>
/// <returns>A comma-separated list of event names.</returns>
std::string EventFilterValidator::GetValid() {
	std::vector<std::string> validList(Valid.begin(), Valid.end());
	return Utilities::String::Join(validList, ", ");
}