#!/bin/bash

# Start server
$SERVER_BIN &
SERVER_PID=$!
sleep 0.1

if ! kill -0 $SERVER_PID 2>/dev/null; then
	echo "Server crashed immediatly!"
	exit 1
fi

curl -s -i "http://localhost:$SERVER_PORT"
CURL_EXIT=$?

if [[ $CURL_EXIT -ne 0 ]]; then
	echo "Curl failed with exit code $CURL_EXIT"
	exit 1
fi

kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "GET / test passed"
