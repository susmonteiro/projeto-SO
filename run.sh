#!/bin/bash

make
tilix -e "./tecnico-rwlock /tmp/socket out 1"

(cd api-tests; make; ./a.out /tmp/socket))
