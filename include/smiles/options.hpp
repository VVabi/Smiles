#pragma once

#include <string>

namespace smiles::options {

enum class EarlyExitStage {
    none,
    preprocessor,
    lexer,
    ast,
};

struct CliOptions {
    std::string path_to_file;
    EarlyExitStage early_exit = EarlyExitStage::none;
};

struct ParseCliResult {
    bool ok = false;
    CliOptions options;
    std::string error;
};

std::string usage();
ParseCliResult parse_cli_args(int argc, char** argv);

}  // namespace smiles::options
