#pragma once

#include <memory>

#include "preprocessor/preprocessor.hpp"
#include "smiles/options.hpp"

namespace smiles::core {

void loop(std::unique_ptr<smiles::preprocessor::FileLikeObject> input,
          const options::CliOptions& options);

}  // namespace smiles::core
