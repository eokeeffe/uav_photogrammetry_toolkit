#!/usr/bin/env python
#
#  make sure to run install.sh before trying this script
#  for exif data manipulation
#  For creating calibration exif images using only text data
#  input.
#  Example included in directory
#
#  Example:
#  python create_exif.py -input /media/evan/My\ Passport/Pictures/PhoneBackup_Jun2015/AR.Drone/picture_20150527_194520.jpg -details details.txt -output ArDrone.jpg

import cv,cv2
from gi.repository import GExiv2
from fractions import Fraction
import argparse,re,time,os,sys
import random,math

parser = argparse.ArgumentParser(description='This program integrates data into Image EXIF')
parser.add_argument('-input', action="store",
    help='file to transform',
    dest="input", default=None)
parser.add_argument('-output', action="store",
    help='output file name',
    dest="output", default=None)
parser.add_argument('-details', action="store",
    help='.txt file to extract EXIF information from',
    dest="details", default=None)

def createComments(information):
    '''
        Anything that is not integrated EXIF
        information will be pushed in the
        Image description tag.
        Tags will be seperated by '|' symbol
    '''
    total = ""
    real = ['ImageDescription',
    'Make',
    'Model',
    'DateTime',
    'Software',
    'Orientation',
    'Flash',
    'FNumber',
    'FocalLength',
    'ApertureValue',
    'ExposureTime',
    'ExposureBiasValue',
    'ISOSpeedRatings',
    ]
    for key in information.keys():
        if key not in real:
            total+=key
            total+=':'
            total+=information[key]
            total+="|\n"
    return total

def seperate(information):
    total = information.split(':')
    return total[0],total[1]

def makeDictionary(information):
    dictionary = {}
    for line in information:
        name,value = seperate(line)
        dictionary[name] = value
    return dictionary

args = parser.parse_args()
if args.details==None:
    print "No input details file"
    exit(0)

details_file = open(args.details,"r+")
details = []
for line in details_file.readlines():
    if '#' in line:
        continue
    else:
        details.append(line.strip())
information = makeDictionary(details)
comment = createComments(information)

path = args.input
output_path = args.output
img1 = cv2.imread(path)
cv2.imwrite(output_path,img1)

exif = GExiv2.Metadata(output_path)
t = os.path.getctime(path)
ctime = time.strftime('%d/%m/%Y %H:%M:%S', time.localtime(t))

exif['Exif.Image.ImageDescription'] = comment#information['ImageDescription']
exif['Exif.Image.Make'] = information['Make']
exif['Exif.Image.Model'] = information['Model']
exif['Exif.Image.DateTime'] = ctime
exif['Exif.Image.Software'] = information['Software']
exif['Exif.Image.Orientation'] = information['Orientation']
exif['Exif.Photo.UserComment'] = ""
exif['Exif.Photo.Flash'] = information['Flash']
exif['Exif.Photo.FNumber'] = information['FNumber']
exif['Exif.Photo.FocalLength'] = information['FocalLength']
exif['Exif.Photo.ApertureValue'] = information['ApertureValue']
exif['Exif.Photo.ExposureTime'] = information['ExposureTime']
exif['Exif.Photo.ExposureBiasValue'] = information['ExposureBiasValue']
exif['Exif.Photo.ISOSpeedRatings'] = information['ISOSpeedRatings']


exif.save_file()

print "Modified Image Exif information"
