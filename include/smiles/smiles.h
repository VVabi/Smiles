#pragma once

#include <string_view>

namespace smiles {

std::string_view project_name() noexcept;
bool is_learning_project() noexcept;

}  // namespace smiles
