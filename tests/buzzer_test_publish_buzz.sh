#!/bin/bash
for i in {1..500}
do
   curl --data-binary '{"jsonrpc":"1.0","id":"curltext","method":"buzz","params":["*", "DO NOT DISPLAY FOR 1st - '${i}'"]}' -i -H 'content-type: text/plain' http://127.0.0.1:8081
done
