#!/bin/bash

make
tilix -e "bash -c \"./tecnicofs-rwlock /tmp/socket out 1; exec bash\""
sleep 1 
(cd api-tests; make; echo -e "\n\n=====cliente===="; ./a.out /tmp/socket)
