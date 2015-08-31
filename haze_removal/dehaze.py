#! /usr/bin/env python
from cv2 import imread
import os,sys
import subprocess
sys.path.insert(0, '/home/evan/UAV_Photogrammetry_toolkit/')
from haze_detection import haze_detection

def guidedFilter(inputFile,outputFile):
    '''
        Use the guided filter to remove haze
        creates a new file and copies the exif from previous file
    '''
    cmd = "~/UAV_Photogrammetry_toolkit/haze_removal/main %s"%inputFile
    process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
    process.wait()
    outputFile+="_dehazed"+".jpg"
    cmd2 = "exiftool -overwrite_original -TagsFromFile  %s %s"%(inputFile,outputFile)
    process = subprocess.Popen(cmd2, shell=True, stdout=subprocess.PIPE)
    process.wait()
    return process.returncode

def process_folder(direct,rho=10,threshold=25):
    '''
        Dehaze images in directory
    '''
    extensions =["ppm","PPM",
            "pbm","PBM",
            "pgm","PGM",
            "png","PNG",
            "jpg","JPG",#jpeg group extensions
            "jpeg","JPEG",
            "jpe","JPE",
            "tiff","TIFF",
            "tif","TIF",
            "bmp","BMP",
            "sr","SR",#Sun raster format
            "ras","RAS",
            "jp2","JP2",#Jasper images
    ]
    files = os.listdir(direct)
    if direct[-1] == os.sep:
        directory = direct
    else:
        directory = direct+os.sep
    features = {}
    #Do the Sift detection
    for filen in files:
        full_file =  directory+filen
        filename,ext = os.path.splitext(os.path.basename(filen))
        if "_dehazed" in filename:
            continue
        ext=ext.replace(".","")
        if ext in extensions:#its and image file
            if haze_detection.processImage(imread(full_file),rho,threshold):
                guidedFilter(full_file,directory+filename)
    return

if __name__=='__main__':
    print "Dehazing image directory:",sys.argv[1]
    rho = int(sys.argv[2])
    threshold = int(sys.argv[2])
    process_folder(sys.argv[1],rho,threshold)
