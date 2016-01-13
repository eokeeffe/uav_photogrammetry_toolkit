#!/bin/python
import os,sys,subprocess
import ConfigParser
from blur_detection.blur_detect import *
from blur_detection.auto_canny import *
from blur_detection.dft_inspect import *
from videoExtractor.extractFrameAuto import *

def detect_blur(config):
    #detect_blur
    method = int(ConfigSectionMap(config,"video_detection")['blur_detect_method'])
    if method==0:
        return True
    elif method==1:
        return False
    elif method==2:
        return True
    elif method==3:
        return False
    elif method==4:
        return True
    elif method==5:
        return False
    elif method==6:
        return True
    elif method==7:
        return processVideoThreaded(ConfigSectionMap(config,"video_input")['path'],
            int(ConfigSectionMap(config,"video_detection")['blur_lower_bound']),
            int(ConfigSectionMap(config,"video_detection")['blur_upper_bound']),
            int(ConfigSectionMap(config,"video_detection")['blur_threshold']),
            )
    else:
        return False

def detect_haze(config):
    #detect_haze
    present = True
    if present: return True
    else: return False

def preform_stabilization(config):
    #stabilize

    return

def preform_dehazing(config):
    #stabilize
    return

def preform_image_extraction(config):
    #extract the images
    return

def feature_detection(config):
    #preform feature detection
    method = int(ConfigSectionMap(config,"feature_detection")['method'])

    return

def feature_matching(images):
    #feature matching
    feature_method = int(ConfigSectionMap(config,"feature_detection")['method'])
    return

def start_vsfm(images,matches):
    #automated vsfm process
    return

def ConfigSectionMap(Config,section):
    dict1={}
    options = Config.options(section)
    for option in options:
        try:
            dict1[option]=Config.get(section,option)
            if dict1[option]==-1:
                DebugPrint("skip:%s"%option)
        except:
            print "exception on %s!"%option
            dict1[option]=None
    return dict1

if __name__=='__main__':
    if len(sys.argv) < 2:
        print "usage:process_video configuration_file.ini"
        print "one ini per video file"
        exit(-1)

    config = ConfigParser.RawConfigParser()
    config.read(sys.argv[1])
    sections = config.sections()
    print "Begining processing on:",ConfigSectionMap(config,"video_input")['path']

    #blur detection first
    blur_detected = False
    if config.getboolean("video_detection","blur_detect"):
        blur_detected = detect_blur(config)
        print "Blur found?:",blur_detected
    if config.getboolean("video_detection","blur_correction"):
        
    #image extraction and correction
    if config.getboolean("image_correction","haze_detection"):
        #do extraction,detection,correction
        #to do
        print "to do"
    #Do the Feature Detection here
    feature_detection(config)
