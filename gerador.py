#!/usr/bin/python3

import random
import string
import sys

def usage():
    if(len(sys.argv) < 3):
        print("Usage: ./" + sys.argv[0] + " numbLines outputFile commands=*default*cdlr")
        exit(-1)

def commandArray():
    availableCommands=("c", "d", "l", "r")
    if (len(sys.argv) >= 4):
        cmds= ()
        for c in sys.argv[3]:
            if(c in availableCommands): #parse dos comandos
                cmds += tuple(c)
        return cmds
    return availableCommands #default todos


def createName():
    nome = ""
    for j in range(random.randint(1, 40)):
        nome += random.choice(string.ascii_letters)
    
    return nome


def gerar():
    fpin = open(sys.argv[2], "w")
    comands=()
    db=[]
    comands = commandArray()
    line = ""
    for i in range(int(sys.argv[1])):
        cmd = random.choice(comands)  
        if cmd == 'c':
            nome = createName()
            line = cmd + ' '  
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
        elif cmd == 'r':
            if(len(db)==0): 
                continue
            if(not (random.randint(0,10)%10)): #(Prob 1/10)mesmo nome para teste de erros
                nome = random.choice(db)
                line += cmd + ' ' + nome + ' ' + nome + '\n'
                #mantemos o nome da db ja que e alterado para o mesmo
            else:
                prevName = random.choice(db)
                nome = createName()
                line += cmd + ' ' + prevName + ' ' + nome + '\n'
                db.remove(prevName)
                db += nome
                #muda para novo nome
        
        fpin.write(line)
    fpin.close()


if __name__ == "__main__" :
    usage()
    gerar()
