#include <iostream>
#include <memory>
#include <utility>

#include "smiles/core/loop.hpp"
#include "smiles/options.hpp"
#include "smiles/smiles.h"
#include "preprocessor/preprocessor.hpp"

int main(int argc, char** argv) {
    const auto parse_result = smiles::options::parse_cli_args(argc, argv);
    if (!parse_result.ok) {
        std::cerr << parse_result.error << '\n';
        return 1;
    }

    std::cout << smiles::project_name() << '\n';

    auto generator = smiles::preprocessor::get_input_generator(parse_result.options);

    std::unique_ptr<smiles::preprocessor::FileLikeObject> next;
    while (next = generator->get_next()) {
        smiles::core::loop(std::move(next), parse_result.options);
    }

    return 0;
}
