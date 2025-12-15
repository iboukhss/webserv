#!/usr/bin/env python3

import os
import sys

def main():
    # Read input
    method = os.environ.get("REQUEST_METHOD", "")
    data = ""

    if method == "GET":
        data = os.environ.get("QUERY_STRING", "")
    elif method == "POST":
        length = int(os.environ.get("CONTENT_LENGTH", "0"))
        data = sys.stdin.read(length)

    # Transform input
    result = data.upper()

    # Output CGI response
    print("Status: 200 OK")
    print("Content-Type: text/plain")
    print()
    print(result)
    sys.stdout.flush()

if __name__ == "__main__":
    main()
