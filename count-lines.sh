#!/bin/bash
echo "Source Code:"
cloc src --exclude-list-file=ex-cloc-list.txt

echo "Tests:"
cloc tests

