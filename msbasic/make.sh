#!/bin/bash
set -e

if [ ! -f "msbasic.s" ] && [ -d "sdk/microdos/msbasic" ]; then
  cd sdk/microdos/msbasic
fi

mkdir -p tmp

ca65 -g -D microdos msbasic.s -o tmp/basic.o
ld65 -C microdos.cfg tmp/basic.o -o tmp/basic.app -Ln tmp/basic.lbl --dbgfile tmp/basic.dbg

python3 -c "import os; f = open('tmp/basic.app', 'r+b'); f.seek(8); f.write(os.path.getsize('tmp/basic.app').to_bytes(2, 'little')); f.close()"

mkdir -p ../../output/apps output/apps 2>/dev/null || true
if [ -d "../../output/apps" ]; then
  cp tmp/basic.app ../../output/apps/basic.app
fi
if [ -d "output/apps" ]; then
  cp tmp/basic.app output/apps/basic.app
fi
