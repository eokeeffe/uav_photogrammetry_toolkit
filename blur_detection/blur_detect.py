#!/usr/bin/env python

import numpy as np
import cv2
from math import sqrt
import os,sys
from auto_canny import auto_canny

#apply laplace transform
def applyLaplace(src):
    # laplacian
    laplacian=np.array([[0, 1, 0],
                        [1,-4, 1],
                        [0, 1, 0]])
    np.fft.fft2(laplacian)
#port of 'LAPM' algorithm (Nayar89)
def modifiedLaplace(src):
    M = np.array((-1,2,-1), np.float64)
    G = cv2.getGaussianKernel(3,-1,cv2.CV_64F)
    Lx = cv2.sepFilter2D(src,cv2.CV_64F,M,G)
    Ly = cv2.sepFilter2D(src,cv2.CV_64F,G,M)
    FM = abs(Lx) + abs(Ly)
    focusMeasure = cv2.mean(FM)[0]
    return focusMeasure
#port of 'LAPV' algorithm (Pech2000)
def varianceOfLaplace(src):
    lap = cv2.Laplacian(src,cv2.CV_64F)
    mu,sigma = cv2.meanStdDev(lap)
    focusMeasure = sigma[0]*sigma[0]
    return focusMeasure[0]
#port of 'TENG' (Krotkov86)
def tenengrad(src,ksize):
    Gx = cv2.Sobel(src,cv2.CV_64F,1,0,ksize)
    Gy = cv2.Sobel(src,cv2.CV_64F,0,1,ksize)
    FM = (Gx*Gx) + (Gy*Gy)
    focusMeasure = cv2.mean(FM)[0]
    return focusMeasure
#port of 'GLVN' (Santos97)
def normalizedGrayLevelVariance(src):
    mu,sigma = cv2.meanStdDev(src)
    focusMeasure = (sigma[0]*sigma[0]/mu[0])
    return focusMeasure[0]
#Get the highest absolute value from the Laplace filter 
def absMaxBlur(img,ksize=3):
    grey = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
    return np.max(cv2.convertScaleAbs(cv2.Laplacian(grey,ksize)))
#look for the threshold Higher spectrum point, blur 
#means there will be less of them
def fft_blur_detection(img,upper_thresh,lower_thresh):
    if len(img.shape)>2:
    	grey = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    grey = img
    f = np.fft.fft2(grey)
    fshift = np.fft.fftshift(f)
    magnitude_spectrum = 20*np.log(np.abs(fshift))
    #find amount of higher/lower frequencies
    #find the mean and std deviation of the frequencies
    mean,sigma = cv2.meanStdDev(magnitude_spectrum)#img
    #normalize mean
    return mean[0][0]
#Canny tracked edges
def corner_detection_method(img):
    #Count the number of pixel representing an edge
    imgCanny = auto_canny(img)
    nCountCanny = cv2.countNonZero(imgCanny);
    #Compute a sharpness grade:
    #< 1.5 = blurred, in movement
    #from 1.5 to 6 = acceptable
    #> 6 =stable, sharp
    height, width = img.shape[:2]
    dSharpness = (nCountCanny * 1000.0 / (width * height));
    return dSharpness

if __name__ == '__main__':
    img = cv2.imread(sys.argv[1])
    gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
    print "Abs",absMaxBlur(img,3)
    print modifiedLaplace(gray)
    print varianceOfLaplace(gray)
    print tenengrad(gray,3)
    print normalizedGrayLevelVariance(gray)
    print corner_detection_method(img)
    print fft_blur_detection(gray,0,0)
