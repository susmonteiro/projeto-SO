import random
import string
import sys

def usage():
    if(len(sys.argv) != 3):
        print("Usage: ./" + sys.argv[0] + " numbLines outputFile")

fpin = open(sys.argv[2], "w")
print(sys.argv[1])

for i in range(int(sys.argv[1])):
    fpin.write("c ")
    nome = ""
    for j in range(random.randint(1, 40)):
        nome += random.choice(string.ascii_letters)
    nome += "\n"
    fpin.write(nome)

fpin.close()
