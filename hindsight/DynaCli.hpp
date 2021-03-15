#pragma once 

#ifndef dynacli_h
#define dynacli_h

#include "cli11.hpp"
#include <unordered_map>
#include <type_traits>
#include <functional>
#include <optional>
#include <variant>
#include <memory>
#include <string>
#include <cstdint>

namespace Hindsight {
    namespace Cli {
        namespace detail {
            /// <summary>
            /// An internal type to keep track of command options, flags and their values.
            /// </summary>
            /// <typeparam name="TTypes">a list of types of values that can be held by DynaCli</typeparam>
            template <typename ...TTypes>
            struct InternalOption {
                CLI::Option*            Option;
                std::variant<TTypes...> Value;

                InternalOption() 
                    : Option(nullptr) {

                }
            };

            /*
                contains<...> can be used to determine if a variadic template argument pack contains a specific type.
                It is used in hindsight to ensure 'bool' is in that list, so that DynaCli can handle flags.
            */

            template <typename T, typename... Args>
            struct contains;

            template <typename T>
            struct contains<T> : std::false_type {};

            template <typename T, typename... Args>
            struct contains<T, T, Args...> : std::true_type {};

            template <typename T, typename A, typename... Args>
            struct contains<T, A, Args...> : contains<T, Args...> {};

            template <typename T, typename... Args>
            constexpr bool contains_v = contains<T, Args...>::value;
        }

        /// <summary>
        /// OptionDescriptor is a struct that should describe command line flags and options.
        /// See ArgumentNames.hpp and hindsight.cpp for usage.
        /// </summary>
        struct OptionDescriptor {
            const char* Name;
            const char* Flag;
            const char* Desc;

            /// <summary>
            /// Construct a constexpr new OptionDescriptor.
            /// </summary>
            /// <param name="name">The option or flag lookup name.</param>
            /// <param name="flag">The option or flag notation in CLI (i.e. "-v,--version", "-b", "positional").</param>
            /// <param name="desc">The description, as seen in the --help and --help-all lists.</param>
            constexpr OptionDescriptor(const char* name, const char* flag, const char* desc) 
                : Name(name), Flag(flag), Desc(desc) {

            }
        };

        /// <summary>
        /// DynaCli is a wrapper around CLI11 which can also hold the values for all options, by internally utilizing std::variant.
        /// </summary>
        /// <typeparam name="TTypes">A list of types supported by the created instance of DynaCli.</typeparam>
        /// <remarks>
        /// note: aliases for types can not be used, has i.e. HANDLE and void* are the same; std::variant wants only one occurrence per type.
        ///       You can use DynaCli<bool, void*> and instance.get<HANDLE>(name) for example, as variant will cast identical types.
        /// </remarks>
        template <typename ...TTypes>
        class DynaCli {
            /// <summary>
            /// Bool is required.
            /// </summary>
            static_assert(
                detail::contains_v<bool, TTypes...>,
                "bool is required as a type to be able to work with flags."
            );

            private:
                CLI::App  m_CreatedCommand;
                CLI::App& m_Command;

                std::unordered_map<std::string, detail::InternalOption<TTypes...>> m_Options;
                std::unordered_map<std::string, std::unique_ptr<DynaCli<TTypes...>>> m_Subcommands;
            
                /// <summary>
                /// Throws an exception when <paramref name="name"/> does not exist as option.
                /// </summary>
                /// <param name="name">The option name to assert the existence for.</param>
                void assert_exist(const std::string& name) const {
                    if (!exists(name))
                        throw std::runtime_error("invalid option name, does not exist: " + name); 
                }

                /// <summary>
                /// Throws an exception when <paramref name="name"/> exists as option.
                /// </summary>
                /// <param name="name">The option name to assert the existence for.</param>
                void assert_not_exists(const std::string& name) const {
                    if (exists(name))
                        throw std::runtime_error("invalid option name, already exists: " + name);
                }

                /// <summary>
                /// Throws an exception when the option named <paramref name="name"/> does not hold the type <typeparamref name="TValue"/>.
                /// </summary>
                /// <param name="name">The option name to assert the type for.</param>
                /// <typeparam name="TValue">The required type.</typeparam>
                template<typename TValue>
                void assert_holds_alternative(const std::string& name) const {
                    if (!holds_alternative<TValue>(name))
                        throw std::runtime_error("invalid option type for: " + name);
                }

                /// <summary>
                /// Throws an exception when the subcommand named <paramref name="name"/> does not exist.
                /// </summary>
                /// <param name="name">The subcommand name to assert the existence for.</param>
                void assert_subcommand_exist(const std::string& name) const {
                    if (!subcommand_exist(name))
                        throw std::runtime_error("invalid subcommand: " + name);
                }
            public:
                /// <summary>
                /// Construct a new DynaCli instance from a pointer to a <see cref="::CLI::App"/> instance.
                /// </summary>
                /// <param name="command">The CLI11 command.</param>
                DynaCli(CLI::App* command)
                    : m_Command(*command) {

                }

                /// <summary>
                /// Construct a new DynaCli instance from a reference to a <see cref="::CLI::App"/> instance.
                /// </summary>
                /// <param name="command">The CLI11 command.</param>
                DynaCli(CLI::App& command)
                    : m_Command(command) {

                }

                /// <summary>
                /// Construct a new DynaCli instance, which in turn constructs a new <see cref="::CLI::App"/> instance.
                /// </summary>
                /// <param name="description">The description for the command (or program).</param>
                DynaCli(const std::string& description)
                    : m_CreatedCommand(CLI::App(description)), m_Command(m_CreatedCommand) {

                }

                /// <summary>
                /// Construct a new DynaCli instance, which in turn constructs a new <see cref="::CLI::App"/> instance.
                /// </summary>
                /// <param name="description">The description for the command (or program).</param>
                /// <param name="app_name">The name for the command (or program).</param>
                DynaCli(const std::string& description, const std::string& app_name)
                    : m_CreatedCommand(CLI::App(description, app_name)), m_Command(m_CreatedCommand) {

                }

                /// <summary>
                /// Get a const reference to the internal <see cref="CLI::App"/> instance.
                /// </summary>
                /// <returns>The internal <see cref="CLI::App"/> instance.</returns>
                const CLI::App& command() const {
                    return m_Command;
                }

                /// <summary>
                /// Get a reference to the internal <see cref="CLI::App"/> instance.
                /// </summary>
                /// <returns>The internal <see cref="CLI::App"/> instance.</returns>
                CLI::App& command() {
                    return m_Command;
                }

                /// <summary>
                /// Determines if an option with the name <paramref name="name"/> exists.
                /// </summary>
                /// <param name="name">The name of the option.</param>
                /// <returns>When the option exists, true is returned.</returns>
                bool exists(const std::string& name) const {
                    return m_Options.count(name) != 0;
                }

                /// <summary>
                /// Determines if an option with the name <paramref name="name"/> contains the type <typeparamref name="TValue"/>.
                /// </summary>
                /// <param name="name">The name of the option.</param>
                /// <typeparam name="TValue">The required value.</typeparam>
                /// <returns>When the option <paramref name="name"/> holds a <typeparamref name="TValue"/>, true is returned.</returns>
                template <typename TValue>
                bool holds_alternative(const std::string& name) const {
                    assert_exist(name);

                    return std::holds_alternative<TValue>(m_Options.at(name).Value);
                }

                template <typename TValue>
                const TValue& get(const std::string& name) const {
                    assert_exist(name);
                    assert_holds_alternative<TValue>(name);
                    return std::get<TValue>(m_Options.at(name).Value);
                }

                template <typename TValue>
                TValue& get(const std::string& name) {
                    assert_exist(name);
                    assert_holds_alternative<TValue>(name);
                    return std::get<TValue>(m_Options.at(name).Value);
                }

                template <typename TValue>
                void set(const std::string& name, const TValue& value) {
                    assert_exist(name);
                    assert_holds_alternative<TValue>(name);
                    std::get<TValue>(m_Options.at(name).Value) = value;
                }

                template <typename TValue>
                std::optional<TValue> get_optional(const std::string& name) const {
                    if (!exists(name) || !holds_alternative<TValue>(name))
                        return {};
                
                    return std::get<TValue>(m_Options.at(name).Value);
                }

                template <typename TValue>
                std::optional<TValue> get_isset(const std::string& name) const {
                    if (!isset(name))
                        return {};
                    return std::get<TValue>(m_Options.at(name).Value);
                }

                template <typename TValue>
                const TValue get_isset_or(const std::string& name, const TValue& def = {}) const {
                    if (!isset(name))
                        return def;
                    return std::get<TValue>(m_Options.at(name).Value);
                }

                bool isset(const std::string& name) const {
                    assert_exist(name);
                    return *m_Options.at(name).Option;
                }

                bool anyset(const std::initializer_list<std::string>& names) const {
                    for (const auto& name : names) {
                        if (isset(name))
                            return true;
                    }

                    return false;
                }

                bool subcommand_anyset(const std::initializer_list<std::string>& subcommands, const std::initializer_list<std::string>& names) {
                    for (const auto& subcommand : subcommands) {
                        if (get_subcommand(subcommand).anyset(names))
                            return true;
                    }

                    return false;
                }

                CLI::Option* add_flag(const std::string& name, const std::string& flag, const std::string& desc) {
                    assert_not_exists(name);

                    auto& option  = m_Options[name];
                    option.Value  = false;
                    option.Option = m_Command.add_flag(flag, std::get<bool>(option.Value), desc);
                    return option.Option;
                }

                CLI::Option* add_flag(const std::string& name, const std::string& flag, const std::string& desc, std::function<void (size_t)> func) {
                    assert_not_exists(name);

                    auto& option  = m_Options[name];
                    option.Value  = {};
                    option.Option = m_Command.add_flag(flag, func, desc);

                    return option.Option;
                }

                CLI::Option* add_flag(const OptionDescriptor& descriptor) {
                    return add_flag(descriptor.Name, descriptor.Flag, descriptor.Desc);
                }

                CLI::Option* add_flag(const OptionDescriptor& descriptor, std::function<void(size_t)> func) {
                    return add_flag(descriptor.Name, descriptor.Flag, descriptor.Desc, func);
                }

                template <typename TValue>
                CLI::Option* add_option(const std::string& name, const std::string& flag, const std::string& desc) {
                    static_assert(
                        detail::contains_v<TValue, TTypes...>,
                        "invalid type specified in TValue, is not contained in TTypes... of container"
                    );

                    assert_not_exists(name);
                    TValue value  = {};
                    auto& option  = m_Options[name];
                    option.Value  = value;
                    option.Option = m_Command.add_option(flag, std::get<TValue>(option.Value), desc);

                    return option.Option;
                }

                template <typename TValue>
                CLI::Option* add_option(const OptionDescriptor& descriptor) {
                    return add_option<TValue>(descriptor.Name, descriptor.Flag, descriptor.Desc);
                }

                CLI::Option* get_option(const std::string& name) {
                    assert_exist(name);
                    return m_Options.at(name).Option;
                }

                DynaCli<TTypes...>& add_subcommand(const std::string& name, const std::string& description) {
                    auto app = command().add_subcommand(name, description);
                    auto cli = std::make_unique<DynaCli<TTypes...>>(app);
                    m_Subcommands[name] = std::move(cli);
                    return *m_Subcommands[name];
                }

                bool subcommand_exist(const std::string& name) const {
                    return m_Subcommands.count(name) != 0;
                }

                DynaCli<TTypes...>& get_subcommand(const std::string& name) {
                    assert_subcommand_exist(name);
                    return *m_Subcommands.at(name);
                }

                const DynaCli<TTypes...>& get_subcommand(const std::string& name) const {
                    assert_subcommand_exist(name);
                    return *m_Subcommands.at(name);
                }

                bool is_subcommand_chosen() const {
                    for (const auto& pair : m_Subcommands) {
                        if (pair.second->command().parsed())
                            return true;
                    }

                    return false;
                }

                bool is_subcommand_chosen(const std::string& name) const {
                    assert_subcommand_exist(name);
                    return m_Subcommands.at(name)->command().parsed();
                }

                const std::string& get_chosen_subcommand_name() const {
                    for (const auto& pair : m_Subcommands) {
                        if (pair.second->command().parsed())
                            return pair.first;
                    }

                    throw std::runtime_error("no subcommand was chosen.");
                }

                DynaCli<TTypes...>& operator[](const std::string& name) {
                    return get_subcommand(name);
                }

                const DynaCli<TTypes...>& operator[](const std::string& name) const {
                    return get_subcommand(name);
                }
        };
    }
}

#endif 