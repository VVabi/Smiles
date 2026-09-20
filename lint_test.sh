if command -v cpplint >/dev/null 2>&1; then
    lint_cmd=(cpplint)
else
    lint_cmd=(python3 -m cpplint)
fi

"${lint_cmd[@]}" \
    --filter=-legal/copyright,-runtime/explicit,-whitespace/line_length,-runtime/references,-build/c++11,-build/c++17 \
    --exclude=build/* \
    --exclude=build_oc/* \
    $(find . -type f -regex ".*\.\(c\|h\|hpp\|cpp\)")
