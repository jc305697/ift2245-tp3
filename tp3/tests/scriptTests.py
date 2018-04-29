from random import randrange
import random

def main():
    nbTests = 5
    for i in range(nbTests):
        f = open("testTPOS" + str(i) + ".txt", "w+")
        for j in range(500):
            adresse = random.randint(0, 65535)
            mychar = chr(randrange(32, 127))
            if random.random() < 0.7:
                op = "R"
                result = op + str(adresse)
            else:
                op = "W"
                result = op + "'" + mychar + "'"
            f.write(result + ";\n")
        f.close()


if __name__ == "__main__":
    main()