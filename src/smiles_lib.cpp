#include "smiles/smiles.h"

namespace smiles {

std::string_view project_name() noexcept {
    return "Smiles";
}

bool is_learning_project() noexcept {
    return true;
}

}  // namespace smiles
