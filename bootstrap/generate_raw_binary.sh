#!/bin/sh
d="$1"; shift
while [ "$1" != "--" ]; do
  cp "$1" "$d/$(basename "$1")"; shift
done
