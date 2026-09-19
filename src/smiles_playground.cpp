#include <iostream>

#include "smiles/smiles.h"

int main() {
    std::cout << "Welcome to the " << smiles::project_name() << " playground.\n";
    return smiles::is_learning_project() ? 0 : 1;
}
