#!/usr/bin/env python3

"""Generate namespace dependency edges for C++ source trees.

The script scans C++ files, detects declared namespaces, and infers edges
"A -> B" when code in namespace A references symbols in namespace B.
It can also emit a Graphviz DOT file.
"""

from __future__ import annotations

import argparse
import pathlib
import re
from collections import defaultdict


CPP_EXTENSIONS = {".h", ".hpp", ".hh", ".hxx", ".c", ".cc", ".cpp", ".cxx"}
DEFAULT_SCAN_DIRS = ("include", "src", "tests")
DEFAULT_EXCLUDE_DIRS = {".git", "build", "build_oc"}

NAMESPACE_DECL_RE = re.compile(r"\bnamespace\s+([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*\{")
QUALIFIED_TOKEN_RE = re.compile(r"\b([A-Za-z_]\w*(?:::[A-Za-z_]\w*)+)\b")


def iter_cpp_files(root: pathlib.Path, scan_dirs: list[str], exclude_dirs: set[str]) -> list[pathlib.Path]:
    files: list[pathlib.Path] = []
    for scan_dir in scan_dirs:
        base = root / scan_dir
        if not base.exists() or not base.is_dir():
            continue

        for path in base.rglob("*"):
            if not path.is_file() or path.suffix not in CPP_EXTENSIONS:
                continue
            if any(part in exclude_dirs for part in path.parts):
                continue
            files.append(path)
    return files


def load_text(path: pathlib.Path) -> str:
    try:
        return path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return path.read_text(encoding="utf-8", errors="ignore")


def collect_declared_namespaces(files: list[pathlib.Path]) -> dict[pathlib.Path, set[str]]:
    declared_by_file: dict[pathlib.Path, set[str]] = {}
    for file in files:
        text = load_text(file)
        declared = set(NAMESPACE_DECL_RE.findall(text))
        declared_by_file[file] = declared
    return declared_by_file


def pick_owner_namespace(declared: set[str]) -> str | None:
    if not declared:
        return None
    # Prefer the most specific namespace in case multiple declarations exist.
    return max(declared, key=lambda ns: ns.count("::"))


def namespace_prefixes(token: str) -> list[str]:
    parts = token.split("::")
    if len(parts) < 2:
        return []
    prefixes = []
    for size in range(1, len(parts)):
        prefixes.append("::".join(parts[:size]))
    # Try longest prefixes first.
    prefixes.reverse()
    return prefixes


def owner_search_bases(owner: str | None) -> list[str]:
    if not owner:
        return [""]

    parts = owner.split("::")
    bases = [""]
    for size in range(1, len(parts) + 1):
        bases.append("::".join(parts[:size]))
    # Prefer nearest scope first (full owner), then outer scopes, then global.
    bases.reverse()
    return bases


def resolve_reference_namespace(
    ref_token: str,
    owner: str | None,
    known_namespaces: set[str],
) -> str | None:
    best: str | None = None
    best_score = -1

    candidate_prefixes = namespace_prefixes(ref_token)
    if not candidate_prefixes:
        return None

    for ns_prefix in candidate_prefixes:
        for base in owner_search_bases(owner):
            candidate = f"{base}::{ns_prefix}" if base else ns_prefix
            if candidate not in known_namespaces:
                continue

            score = candidate.count("::")
            if score > best_score:
                best = candidate
                best_score = score

    return best


def build_dependency_edges(
    files: list[pathlib.Path],
    declared_by_file: dict[pathlib.Path, set[str]],
) -> set[tuple[str, str]]:
    all_namespaces: set[str] = set()
    for declared in declared_by_file.values():
        all_namespaces.update(declared)

    edges: set[tuple[str, str]] = set()

    for file in files:
        owner = pick_owner_namespace(declared_by_file[file])
        if owner is None:
            continue

        text = load_text(file)
        for token in QUALIFIED_TOKEN_RE.findall(text):
            if token == owner or token.startswith(owner + "::"):
                continue
            target = resolve_reference_namespace(token, owner, all_namespaces)
            if not target or target == owner:
                continue
            edges.add((owner, target))

    return edges


def build_dot(namespaces: set[str], edges: set[tuple[str, str]]) -> str:
    lines = [
        "digraph namespace_dependencies {",
        "  rankdir=LR;",
        "  node [shape=box, style=rounded];",
    ]

    for namespace in sorted(namespaces):
        lines.append(f'  "{namespace}";')

    for source, target in sorted(edges):
        lines.append(f'  "{source}" -> "{target}";')

    lines.append("}")
    return "\n".join(lines) + "\n"


def find_cycle(edges: set[tuple[str, str]]) -> list[str] | None:
    adjacency: dict[str, set[str]] = defaultdict(set)
    nodes: set[str] = set()
    for source, target in edges:
        adjacency[source].add(target)
        nodes.add(source)
        nodes.add(target)

    state: dict[str, int] = {}
    stack: list[str] = []

    def dfs(node: str) -> list[str] | None:
        state[node] = 1
        stack.append(node)

        for neighbor in sorted(adjacency.get(node, set())):
            if state.get(neighbor, 0) == 0:
                cycle = dfs(neighbor)
                if cycle is not None:
                    return cycle
            elif state.get(neighbor) == 1:
                start_index = stack.index(neighbor)
                return stack[start_index:] + [neighbor]

        stack.pop()
        state[node] = 2
        return None

    for node in sorted(nodes):
        if state.get(node, 0) == 0:
            cycle = dfs(node)
            if cycle is not None:
                return cycle

    return None


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate namespace dependency graph for C++ files.")
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."), help="Repository root (default: current directory)")
    parser.add_argument(
        "--scan-dir",
        action="append",
        default=[],
        help="Directory to scan (can be passed multiple times). Defaults to include, src, tests.",
    )
    parser.add_argument(
        "--exclude-dir",
        action="append",
        default=[],
        help="Directory name to exclude (can be passed multiple times).",
    )
    parser.add_argument("--dot", type=pathlib.Path, default=None, help="Write Graphviz DOT output to this path")
    parser.add_argument(
        "--fail-on-cycle",
        action="store_true",
        help="Exit with non-zero code when a namespace dependency cycle is found.",
    )

    args = parser.parse_args()
    root = args.root.resolve()
    scan_dirs = args.scan_dir if args.scan_dir else list(DEFAULT_SCAN_DIRS)
    exclude_dirs = DEFAULT_EXCLUDE_DIRS | set(args.exclude_dir)

    files = iter_cpp_files(root, scan_dirs, exclude_dirs)
    declared_by_file = collect_declared_namespaces(files)

    all_namespaces: set[str] = set()
    for declared in declared_by_file.values():
        all_namespaces.update(declared)

    edges = build_dependency_edges(files, declared_by_file)

    reverse_deps: dict[str, set[str]] = defaultdict(set)
    for source, target in edges:
        reverse_deps[source].add(target)

    print("Namespaces:")
    for namespace in sorted(all_namespaces):
        print(f"  - {namespace}")

    print("\nDependencies:")
    if not edges:
        print("  (none)")
    else:
        for source in sorted(reverse_deps):
            targets = ", ".join(sorted(reverse_deps[source]))
            print(f"  - {source} -> {targets}")

    if args.dot is not None:
        dot_path = args.dot if args.dot.is_absolute() else (root / args.dot)
        dot_path.parent.mkdir(parents=True, exist_ok=True)
        dot_path.write_text(build_dot(all_namespaces, edges), encoding="utf-8")
        print(f"\nDOT written to: {dot_path}")

    if args.fail_on_cycle:
        cycle = find_cycle(edges)
        if cycle is None:
            print("\nCycle check: no cycles found")
        else:
            print("\nCycle check: cycle found")
            print("  " + " -> ".join(cycle))
            return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
