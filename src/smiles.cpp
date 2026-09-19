#include <iostream>

#include "smiles/cli.hpp"
#include "smiles/smiles.h"

int main(int argc, char** argv) {
    const auto parse_result = smiles::parse_cli_args(argc, argv);
    if (!parse_result.ok) {
        std::cerr << parse_result.error << '\n';
        return 1;
    }

    std::cout << smiles::project_name() << '\n';
    return 0;
}
