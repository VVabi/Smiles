#include <iostream>

#include "smiles/core/loop.hpp"

namespace smiles::core {

void loop(std::unique_ptr<smiles::preprocessor::FileLikeObject> input,
          const options::CliOptions& options) {
    const auto preprocessed_file = smiles::preprocessor::preprocess_file(*input);

    if (options.early_exit == options::EarlyExitStage::preprocessor) {
        preprocessed_file.pretty_print(std::cout);
    }
}

}  // namespace smiles::core
