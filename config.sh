#!/bin/bash
# driver.sh - Reverse Proxy Server Test Script

# 서버 환경 설정
PROXY_PORT=1111
TARGET_SERVER="10.198.138.213"  # swist2 IP
TARGET_PORT=2222

# 리버스 프록시 서버 실행
./reverse_proxy ${PROXY_PORT} &  # swist1에서 리버스 프록시 실행

# 대상 서버 실행 (swist2)
python3 -m http.server ${TARGET_PORT} &

# 요청 테스트
curl --proxy http://localhost:${PROXY_PORT} http://localhost/${TARGET_SERVER}/testfile

# 서버 종료
kill $(jobs -p)

