#!/usr/bin/env python

# Программа для компиляции скетча через Arduino IDE

import subprocess
import sys

# Путь к файлу Arduino IDE
arduinoProg = "C:/Program Files (x86)/Arduino"

projectFile = sys.argv[1]

codeFile = open(projectFile, 'r')
startLine = codeFile.readline()[3:].strip()
actionLine = codeFile.readline()[3:].strip()
boardLine = codeFile.readline()[3:].strip()
portLine = codeFile.readline()[3:].strip()
endLine = codeFile.readline()[3:].strip()
codeFile.close()

#~ print projectFile
#~ print startLine
#~ print actionLine
#~ print boardLine
#~ print portLine
#~ print endLine

if (startLine != "python-build-start" or endLine != "python-build-end"):
	print ("Невозможно обработать файл")
	sys.exit()

arduinoCommand = arduinoProg + " --" + actionLine + " --board " + boardLine + " --port " + portLine + " " + projectFile

print ("\n\n -- Arduino Command --")
print (arduinoCommand)

print ("-- Starting %s --\n" %(actionLine))

presult = subprocess.call(arduinoCommand, shell=True)

if presult != 0:
	print ("\n Failed - result code = %s --" %(presult))
else:
	print ("\n-- Success --")