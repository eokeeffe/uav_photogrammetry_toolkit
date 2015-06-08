#!/bin/python
import numpy as np
import cv2
import glob
 
grid_size = [8,8]
 
#termination criteria
criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)
 
#prepare object points, like (0,0,0),(1,0,0),(2,00),...,(6,5,0)
objp = np.zeros((grid_size[0]*grid_size[1],3),np.float32)
objp[:,:,2] = np.mgrid[0:grid_size[0],0:grid_size[1]].T.reshape(-1,2)
 
objpoints = []#3D real space points 
imgpoints = []#2D image plane points
 
images = glob.glob('*.jpg')
 
for filename in images:
   img = cv2.imread(filename)
   grey = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
    
   #find chessboard corners
   ret,corners = cv2.findChessboardCorners(gray,
   (grid_size[0],grid_size[1]),
   None)
    
   if ret == True:
       objpoints.append(objp)
       
       corners2 = cv2.cornerSubPix(gray,corners,(11,11),(-1,-1),criteria)
       imgpoints.append(corners2)
       
       # Draw and display the corners
       img = cv2.drawChessboardCorners(img,(grid_size[0],grid_size[1]),corners2,ret)
       cv2.imshow(img)
       cv2.waitKet(500);
cv2.destroyAllWindows()

ret,mtx,dist,rvecs,tvecs = cv2.calibrateCamera(objpoints,
imgpoints,
gray.shape[::-1],
None,
None)

img = cv2.imread('00001.jpg')
h,w = img.shape[:2]
newcameramtx, roi = cv2.getOptimalNewCameraMatrix(mtx,dist,(w,h),1,(w,h))

#undistort
mapx,mapy = cv2.initUndistortRectifyMap(mtx,dist,None,newcameramtx,(w,h),5)
dst = cv2.remap(img,mapx,mapy,cv2.INTER_LINEAR)

#crop the image
x,y,w,h = roi
dst = dst[y:y+h,x:x+w]
cv2.imwrite('calibrated_image.jpg',dst)

np.savetxt('camera_matrix.txt',mtx)
np.savetxt('distortion_coefficients.txt',dist)
np.savetxt('rotation_vectors.txt',rvecs)
np.savetxt('translation_vectors.txt',tvecs)

print "Camera calibration Complete"
print "Matrix information saved in .txt format"
