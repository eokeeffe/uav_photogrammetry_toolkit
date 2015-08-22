import cv2
import os,sys
import numpy as np
from collections import deque
from multiprocessing.pool import ThreadPool,Queue
#from matplotlib import pyplot as plt

def normalize(arr):
    for i in range(3):
        minval=arr[...,i].min()
        maxval=arr[...,i].max()
        if minval!=maxval:
            arr[...,i]-=minval
            arr[...,i]*=(255.0/(maxval-minval))
    return arr

def getRanges(hist,lower,upper):
    lower_count=0
    median_count=0
    upper_count=0
    for i in xrange(0,len(hist)):
        if i < lower:
            lower_count+=hist[i][0]
        if i > upper:
            upper_count+=hist[i][0]
        if i < upper and i > lower:
            median_count+=hist[i][0]
    lower_count/=len(hist)
    median_count/=len(hist)
    upper_count/=len(hist)
    return lower_count,median_count,upper_count

def blurMetric(lower,median,upper):
    '''
        @lower,median,upper
        values that calculate number of
        pixels in the frequeny
        @return True if blurred
        False if not
    '''
    if upper<lower:
        return True
    if upper>lower:
        return False

def processSingleImage(image,lower,upper):
    grey=cv2.imread(image,cv2.IMREAD_GRAYSCALE)
    #cv2.imshow("Normal",grey)
    grey = normalize(grey)
    hist=cv2.calcHist(grey,[0],None,[256],[0,256])
    #plt.hist(grey.ravel(),256,[0,256])
    #plt.title('Histogram for grey picture')
    #print hist
    #print len(hist)
    lower,median,upper = getRanges(hist,120,180)
    #print lower,median,upper
    return blurMetric(lower,median,upper)

def processImageVideoThreaded(image,lower,upper):
    grey=cv2.cvtColor(image,cv2.COLOR_BGR2GRAY)
    grey = normalize(grey)
    for i in range(3):
        minval=grey[...,i].min()
        maxval=grey[...,i].max()
        if minval!=maxval:
            grey[...,i]-=minval
            grey[...,i]*=(255.0/(maxval-minval))
    hist=cv2.calcHist(grey,[0],None,[256],[0,256])
    lower_count=0
    median_count=0
    upper_count=0
    for i in xrange(0,len(hist)):
        if i < lower:
            lower_count+=hist[i][0]
        if i > upper:
            upper_count+=hist[i][0]
        if i < upper and i > lower:
            median_count+=hist[i][0]
    lower_count/=len(hist)
    median_count/=len(hist)
    upper_count/=len(hist)
    if upper_count<lower_count:
        return True
    if upper_count>lower_count:
        return False

def processVideoThreaded(video,lower,upper,threshold):
    cap = cv2.VideoCapture(video)
    image_queue = deque()
    threadn = cv2.getNumberOfCPUs()
    pool = ThreadPool(processes=threadn)
    results = []
    total = 0
    while True:
        got_image,image = cap.read()
        if not(got_image):
            break
        total += 1
        results.append(pool.apply(processImageVideoThreaded,(image,lower,upper)))
    #print results
    blurred = results.count(True)
    cap.release()
    blurred_total = float(blurred)/total*100
    #print total,blurred,blurred_total
    if blurred_total > threshold:
        return True
    return False

def processImageVideo(image,lower,upper):
    grey=cv2.cvtColor(image,cv2.COLOR_BGR2GRAY)
    grey = normalize(grey)
    for i in range(3):
        minval=grey[...,i].min()
        maxval=grey[...,i].max()
        if minval!=maxval:
            grey[...,i]-=minval
            grey[...,i]*=(255.0/(maxval-minval))
    hist=cv2.calcHist(grey,[0],None,[256],[0,256])
    lower_count=0
    median_count=0
    upper_count=0
    for i in xrange(0,len(hist)):
        if i < lower:
            lower_count+=hist[i][0]
        if i > upper:
            upper_count+=hist[i][0]
        if i < upper and i > lower:
            median_count+=hist[i][0]
    lower_count/=len(hist)
    median_count/=len(hist)
    upper_count/=len(hist)

    if upper<lower:
        return True
    if upper>lower:
        return False

def processVideo(video,lower,upper,threshold):
    cap = cv2.VideoCapture(video)
    threadn = cv2.getNumberOfCPUs()
    pool = ThreadPool(processes=threadn)
    results = []
    outputs = None
    total = 0
    while True:
        got_image,image = cap.read()
        if not(got_image):
            break
        total += 1
        results.append(processImageVideo(image,lower,upper))
    blurred = results.count(True)
    cap.release()
    blurred_total = float(blurred)/total*100
    if blurred_total > threshold:
        return True
    return False

if __name__=='__main__':
    video = sys.argv[1]
    lower_bound = int(sys.argv[2])
    upper_bound = int(sys.argv[3])
    threshold = int(sys.argv[4])
    print processVideoThreaded(video,lower_bound,upper_bound,threshold)
