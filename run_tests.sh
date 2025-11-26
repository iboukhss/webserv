#!/bin/bash
set -e

TEST_DIR="tests"

export SERVER_BIN="build/bin/webserv"
export SERVER_PORT=8080

# Make sure the server is built
if [[ ! -x $SERVER_BIN ]]; then
	echo "Building server..."
	make || { echo "Build failed!"; exit 1; }
fi

for test in $TEST_DIR/*.sh; do
	echo "Running $test..."
	bash $test || { echo "$test failed"; exit 1; }
done

echo "All tests passed!"
