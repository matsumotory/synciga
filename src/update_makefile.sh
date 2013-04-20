#!/bin/sh

cd mruby
make CFLAGS=-fPIC
cd ..
export PATH="$PATH":`pwd`/../depot_tools
gclient runhooks
