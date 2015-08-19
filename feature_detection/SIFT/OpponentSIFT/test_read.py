#!/bin/python
import os,sys

file_to_read = open("example","r")
lines = file_to_read.readlines()
files = []
for i in xrange(5,len(lines)):
    if("/" in lines[i]):
        if(lines[i] not in files):
            files.append(lines[i])

print files
file_to_read.close()
