#!/bin/bash
for i in {1..20}
do
   curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"buzz","params":["*", "Second(2) - '${i}'"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8080
done
