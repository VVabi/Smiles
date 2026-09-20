#include <getopt.h>

#include <array>
#include <string>
#include <utility>

#include "smiles/options.hpp"

namespace smiles::options {

namespace {

struct CliOptionDefinition {
    option long_option;
    char short_option;
    const char* value_description = nullptr;
};

struct EarlyExitStageDefinition {
    const char* name;
    EarlyExitStage stage;
};

constexpr std::array<EarlyExitStageDefinition, 3> early_exit_stage_definitions = {{
    {"preprocessor", EarlyExitStage::preprocessor},
    {"lexer", EarlyExitStage::lexer},
    {"ast", EarlyExitStage::ast},
}};

std::string join_early_exit_stage_names() {
    std::string joined_names;
    for (std::array<EarlyExitStageDefinition, 3>::size_type index = 0; index < early_exit_stage_definitions.size(); ++index) {
        if (index > 0) {
            joined_names += '|';
        }
        joined_names += early_exit_stage_definitions[index].name;
    }
    return joined_names;
}

std::string get_option_value_description(const CliOptionDefinition& option_definition) {
    if (option_definition.value_description != nullptr) {
        return option_definition.value_description;
    }

    if (option_definition.short_option == 'e') {
        return "{" + join_early_exit_stage_names() + "}";
    }

    return "VALUE";
}

constexpr CliOptionDefinition cli_option_definitions[] = {
    {{"early_exit", required_argument, nullptr, 'e'}, 'e'},
};

constexpr option cli_long_options[] = {
    {"early_exit", required_argument, nullptr, 'e'},
    {nullptr, 0, nullptr, 0},
};

constexpr char short_options[] = "e:";

ParseCliResult make_error(const std::string& message) {
    return {false, {}, message + "\n" + usage()};
}

bool parse_early_exit_stage(const std::string& value, EarlyExitStage& stage) {
    for (const auto& early_exit_stage_definition : early_exit_stage_definitions) {
        if (value == early_exit_stage_definition.name) {
            stage = early_exit_stage_definition.stage;
            return true;
        }
    }
    return false;
}

}  // namespace

std::string usage() {
    std::string usage_text = "Usage: smiles";
    for (const auto& option_definition : cli_option_definitions) {
        usage_text += " [-";
        usage_text += option_definition.short_option;
        usage_text += "|--";
        usage_text += option_definition.long_option.name;
        if (option_definition.long_option.has_arg == required_argument) {
            usage_text += ' ';
            usage_text += get_option_value_description(option_definition);
        }
        usage_text += ']';
    }
    usage_text += " [path_to_file]";
    return usage_text;
}

ParseCliResult parse_cli_args(int argc, char** argv) {
    CliOptions options;

    opterr = 0;
    optind = 0;

    int option_index = 0;
    int opt;
    while ((opt = getopt_long(argc, argv, short_options, cli_long_options, &option_index)) != -1) {
        switch (opt) {
            case 'e': {
                const std::string value = optarg;
                if (!parse_early_exit_stage(value, options.early_exit)) {
                    return make_error("Invalid early exit stage '" + value + "'. Expected: " + join_early_exit_stage_names());
                }
                break;
            }
            default:
                return make_error("Failed to parse command line arguments");
        }
    }

    const int remaining_arguments = argc - optind;
    if (remaining_arguments == 0) {
        return make_error("Missing input file path");
    }

    if (remaining_arguments > 1) {
        return make_error("Expected a single input file path");
    }

    options.path_to_file = argv[optind];
    return {true, std::move(options), ""};
}

}  // namespace smiles::options
