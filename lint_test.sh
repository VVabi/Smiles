cpplint --filter=-legal/copyright,-runtime/explicit,-whitespace/line_length,-runtime/references,-build/c++11 --exclude=build/* --exclude=build_oc/* $(find . -type f -regex ".*\.\(c\|h\|hpp\|cpp\)")
