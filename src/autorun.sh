#!/bin/sh

cp proxy hosts /tmp &&
(cd /tmp && nohup ./proxy >/dev/null) &
