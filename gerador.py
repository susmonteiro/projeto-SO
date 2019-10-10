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
    comands = commandArray()
    for i in range(int(sys.argv[1])):
        cmd = random.choice(comands)  
        fpin.write(cmd + " ")
        nome = ""
        for j in range(random.randint(1, 40)):
            nome += random.choice(string.ascii_letters)
       
        #push(nome) depois ver l ou d pop
        nome += "\n"
        fpin.write(nome)

    fpin.close()


if __name__ == "__main__" :
    usage()
    gerar()
