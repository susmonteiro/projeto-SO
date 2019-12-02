#!/bin/bash

make
tilix -e "bash -c \"valgrind --log-file=outvalgrind.txt ./tecnicofs-rwlock socket out 1 > out.txt; exec bash\""&
#tilix -e "bash -c \"valgrind ./tecnicofs-rwlock socket out 1; exec bash\""&
sleep 1 
(cd client/api-tests; make; echo -e "\n\n=====cliente===="; ./a.out socket)
