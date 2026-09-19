# Smiles
Programming language for learning

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
