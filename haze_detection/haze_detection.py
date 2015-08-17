#
# A Fast Semi inverse approach to detect and remove the
# haze from a single image - ACCV 2010 
#
import cv2
import numpy as np
import os,sys
from math import *

def semiInverseImage(img):
    r_semi_inverse = np.zeros((img.shape[0],img.shape[1]),img.dtype)
    g_semi_inverse = np.zeros((img.shape[0],img.shape[1]),img.dtype)
    b_semi_inverse = np.zeros((img.shape[0],img.shape[1]),img.dtype)
    
    max_value = np.iinfo(img.dtype).max
    
    b,g,r = cv2.split(img)
    #b = np.float32(b)
    #g = np.float32(g)
    #r = np.float32(r)
    #values from 0->1
    #cv2.normalize(b,b,0,1,cv2.NORM_MINMAX)
    #cv2.normalize(g,g,0,1,cv2.NORM_MINMAX)
    #cv2.normalize(r,r,0,1,cv2.NORM_MINMAX)
    
    rows,cols=r.shape
    for i in xrange(0,rows):
        for j in xrange(0,cols):
            #print max_Irx,max_Igx,max_Ibx
            r_semi_inverse[i,j] = max(r[i,j],max_value-r[i,j])
            g_semi_inverse[i,j] = max(g[i,j],max_value-g[i,j])
            b_semi_inverse[i,j] = max(b[i,j],max_value-b[i,j])
        
    cv2.normalize(b_semi_inverse,b_semi_inverse,0,255,cv2.NORM_MINMAX)
    cv2.normalize(g_semi_inverse,g_semi_inverse,0,255,cv2.NORM_MINMAX)
    cv2.normalize(r_semi_inverse,r_semi_inverse,0,255,cv2.NORM_MINMAX)
    
    semi_inverse = cv2.merge((b_semi_inverse,g_semi_inverse,r_semi_inverse))
    return semi_inverse

def hazeDetection(img,semi_inv_img,rho=10,img_haze_size=3):
    '''
        Fast Semi 
    '''
    rho = float(rho)/1000
    #print rho
    hsv_img = cv2.cvtColor(img,cv2.COLOR_BGR2LAB)
    hsv_semi_inv_img = cv2.cvtColor(semi_inv_img,cv2.COLOR_BGR2LAB)
    L,A,B = cv2.split(hsv_img)
    L2,A2,B2 = cv2.split(hsv_semi_inv_img)    
    
    #convert CIE LAB to LCHab
    h = np.arctan2(B,A)
    h2 = np.arctan2(B2,A2)
    
    rows,cols,depth = img.shape
    count=0
    for i in xrange(0,rows):
        for j in xrange(0,cols):
            #print h[i,j],h2[i,j],abs(h[i,j] - h2[i,j]),rho
            if abs(h[i,j] - h2[i,j]) < rho:
                count+=1
    #print count
    if count > (rows*cols/img_haze_size):
        return True
    else:
        return False
    

if __name__=='__main__':
    image = cv2.imread(sys.argv[1])
    semi_inverse = semiInverseImage(image.copy())
    print "Is image hazy?:",hazeDetection(image,semi_inverse,10)
    #images = np.hstack((image,semi_inverse))
    #cv2.imshow('result',cv2.WINDOW_NORMAL)
    #cv2.imshow("result",images)
    #cv2.waitKey(0)
    #cv2.destroyAllWindows()
    cv2.imwrite("reg.jpg",image)
    cv2.imwrite("sinv.jpg",semi_inverse)
    exit(0)
