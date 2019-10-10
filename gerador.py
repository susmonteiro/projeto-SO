#!/usr/bin/python3

import random
import string
import sys

def usage():
    if(len(sys.argv) < 3):
        print("Usage: ./" + sys.argv[0] + " numbLines outputFile commands=*default*cdl")
        exit(-1)

def commandArray():
    if (len(sys.argv) >= 4):
        cmds= ()
        for c in sys.argv[3]:
            if(c in ("c", "d", "l")): #parse dos comandos
                cmds += tuple(c)
        return cmds
    return ("c", "d", "l") #default todos

def gerar():
    fpin = open(sys.argv[2], "w")
    comands=()
    db=[]
    comands = commandArray()
    line = ""
    for i in range(int(sys.argv[1])):
        cmd = random.choice(comands)  
        if cmd == 'c':

            line = cmd + ' '  
            nome = ""
            for j in range(random.randint(1, 40)):
                nome += random.choice(string.ascii_letters)
            line += nome + "\n"
        
            db += [nome]
        elif cmd == 'd':
            if len(db) == 0:
                continue
            nome = random.choice(db)
            db.remove(nome)
            line = cmd + ' ' + nome + '\n'
        elif cmd == 'l':
            if len(db) == 0:
                continue
            nome = random.choice(db)
            line = cmd + ' ' + nome + '\n'

        fpin.write(line)

    fpin.close()


if __name__ == "__main__" :
    usage()
    gerar()
