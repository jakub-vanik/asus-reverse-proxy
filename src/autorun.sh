#!/bin/sh

cp proxy hosts filter.conf /tmp &&
(cd /tmp && nohup ./proxy >/dev/null) &
