#!/bin/sh

export PATH="$PATH":`pwd`/../depot_tools
gclient runhooks
