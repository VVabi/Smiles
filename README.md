# Smiles
Programming language for learning

## Project Layout

- `include/`: public headers
- `src/`: production source files
- `tests/`: test sources

## Build

```sh
cmake -S . -B build
cmake --build build --target smiles smiles_playground smiles_tests
```

## Run

```sh
./build/smiles
./build/smiles_playground
ctest --test-dir build --output-on-failure
```
