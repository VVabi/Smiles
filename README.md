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

## Namespace Dependency Graph

Generate inferred namespace dependencies and a Graphviz DOT file:

```sh
python3 scripts/namespace_deps.py --dot namespace_deps.dot
```

Verify production namespaces are acyclic (`include` + `src` only):

```sh
python3 scripts/namespace_deps.py --root . --fail-on-cycle --scan-dir include --scan-dir src
```

Optional rendering (if Graphviz is installed):

```sh
dot -Tpng namespace_deps.dot -o namespace_deps.png
```
