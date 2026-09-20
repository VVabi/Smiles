#include <memory>
#include <string>
#include <utility>

#include "preprocessor/preprocessor.hpp"

namespace smiles::preprocessor {
class FileInputGenerator : public InputGenerator {
    std::unique_ptr<FileObject> file;

 public:
    FileInputGenerator(std::string input_file) {
        file = std::make_unique<FileObject>(input_file);
    }

    std::unique_ptr<FileLikeObject> get_next() {
        if (!file) {
            return nullptr;
        }
        return std::move(file);
    }
};

std::unique_ptr<InputGenerator> get_input_generator(const smiles::options::CliOptions& options) {
    return std::make_unique<FileInputGenerator>(options.path_to_file);
}

}  // namespace smiles::preprocessor
