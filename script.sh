#!/bin/bash

make
konsole -e "bash -c \"valgrind ./tecnicofs-rwlock /tmp/socket out 1; exec bash\""&
sleep 3 
(cd api-tests; make; echo -e "\n\n=====cliente===="; ./a.out /tmp/socket)
