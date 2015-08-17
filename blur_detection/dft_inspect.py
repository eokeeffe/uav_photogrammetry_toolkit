import cv2
import os,sys
import numpy as np
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

if __name__=='__main__':
    grey=cv2.imread(sys.argv[1],cv2.IMREAD_GRAYSCALE)
    #cv2.imshow("Normal",grey)
    grey = normalize(grey)
    
    lower_bound = int(sys.argv[2])
    upper_bound = int(sys.argv[3])

    hist=cv2.calcHist(grey,[0],None,[256],[0,256])
    #plt.hist(grey.ravel(),256,[0,256])
    #plt.title('Histogram for grey picture')
    #print hist
    #print len(hist)
    lower,median,upper = getRanges(hist,120,180)
    #print lower,median,upper
    print blurMetric(lower,median,upper)
