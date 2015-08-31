import cv2
import os,sys
from multiprocessing.pool import ThreadPool
import numpy as np
from collections import deque

max_value=255

def sinv(x):
    global max_value
    return max(x,max_value-x)

def semi_inverse_image(image):
    func = np.vectorize(sinv)
    image[:,:,0]=func(image[:,:,0])
    image[:,:,1]=func(image[:,:,1])
    image[:,:,2]=func(image[:,:,2])
    return image

def difference(image,sinv):
    hsv1 = cv2.cvtColor(image,cv2.COLOR_BGR2HSV)
    hsv2 = cv2.cvtColor(sinv,cv2.COLOR_BGR2HSV)
    return cv2.absdiff(hsv1[:,:,0],hsv2[:,:,0])

def percentage(difference,rho):
    count = 0
    count = (difference<rho).sum()
    return float(count)/(image.shape[0]*image.shape[1])*100

def hazed(percentage,threshold):
    if percentage>threshold:
        return True
    else:
        return False

def processImage(image,rho,threshold):
    #get semi-inverse
    func = np.vectorize(sinv)
    semi_inverse = image.copy()
    semi_inverse[:,:,0]=func(semi_inverse[:,:,0])
    semi_inverse[:,:,1]=func(semi_inverse[:,:,1])
    semi_inverse[:,:,2]=func(semi_inverse[:,:,2])
    #get absolute difference
    hsv1 = cv2.cvtColor(image,cv2.COLOR_BGR2HSV)
    hsv2 = cv2.cvtColor(semi_inverse,cv2.COLOR_BGR2HSV)
    diff = cv2.absdiff(hsv1[:,:,0],hsv2[:,:,0])
    #count number of values below rho
    count = (diff<rho).sum()
    percentage = float(count)/(image.shape[0]*image.shape[1])*100
    if percentage>threshold:
        return True
    else:
        return False

def processVideoThreaded(video,rho,threshold):
    cap = cv2.VideoCapture(video)
    image_queue = deque()
    pool = ThreadPool(processes=cv2.getNumberOfCPUs()*2)
    results = []
    total = 0
    while True:
        got_image,image = cap.read()
        if not(got_image):
            break
        total += 1
        results.append(pool.apply(processImage,args=(image,rho,threshold)))
    print results
    hazed = results.count(True)
    cap.release()
    hazed_total = float(hazed)/total*100
    print total,hazedhazed_total
    if hazed_total > 0:
        return True
    return False

if __name__=='__main__':
    global data_type
    video = sys.argv[1]
    #image = cv2.imread(sys.argv[1])
    rho = int(sys.argv[2])
    threshold = int(sys.argv[2])
    #semi_inverse = semi_inverse_image(image.copy())
    #cv2.imwrite("inv.jpg",semi_inverse)

    print processVideoThreaded(video,rho,threshold)
