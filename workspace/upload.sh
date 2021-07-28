#!/bin/bash

host=192.168.16.109:9000
s3_key='F2IHVVX44WVGYUIA1ESX'
s3_secret='UiJuXEG4V6ZLqCZ9ZbD9lqKEG8WwtaKeA3kh7Lui'

remote_file="/test-bucket/echo.txt"
local_file="workspace/echo.txt"

content_type="application/octet-stream"
date=`date -R`
_signature="PUT\n\n${content_type}\n${date}\n${remote_file}"
signature=`echo -en ${_signature} | openssl sha1 -hmac ${s3_secret} -binary | base64`

curl -v -X PUT -T "${local_file}" \
          -H "Host: $host" \
          -H "Date: ${date}" \
          -H "Content-Type: ${content_type}" \
          -H "Authorization: AWS ${s3_key}:${signature}" \
          http://$host${remote_file}