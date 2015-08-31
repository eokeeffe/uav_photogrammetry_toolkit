#
#   Affine-Opponent-Hellinger-SalientBoosting-Hellinger SIFT
#   Evan O'Keeffe 2015
#
#OpenCV
import numpy as np
import cv2
# built-in modules
import os,sys
import itertools as it
from multiprocessing.pool import ThreadPool
from math import sqrt

def affine_skew(tilt, phi, img, mask=None):
    '''
    affine_skew(tilt, phi, img, mask=None) -> skew_img, skew_mask, Ai

    Ai - is an affine transform matrix from skew_img to img
    '''
    h, w = img.shape[:2]
    if mask is None:
        mask = np.zeros((h, w), np.uint8)
        mask[:] = 255
    A = np.float32([[1, 0, 0], [0, 1, 0]])
    if phi != 0.0:
        phi = np.deg2rad(phi)
        s, c = np.sin(phi), np.cos(phi)
        A = np.float32([[c,-s], [ s, c]])
        corners = [[0, 0], [w, 0], [w, h], [0, h]]
        tcorners = np.int32( np.dot(corners, A.T) )
        x, y, w, h = cv2.boundingRect(tcorners.reshape(1,-1,2))
        A = np.hstack([A, [[-x], [-y]]])
        img = cv2.warpAffine(img, A, (w, h), flags=cv2.INTER_LINEAR, borderMode=cv2.BORDER_REPLICATE)
    if tilt != 1.0:
        s = 0.8*np.sqrt(tilt*tilt-1)
        img = cv2.GaussianBlur(img, (0, 0), sigmaX=s, sigmaY=0.01)
        img = cv2.resize(img, (0, 0), fx=1.0/tilt, fy=1.0, interpolation=cv2.INTER_NEAREST)
        A[0] /= tilt
    if phi != 0.0 or tilt != 1.0:
        h, w = img.shape[:2]
        mask = cv2.warpAffine(mask, A, (w, h), flags=cv2.INTER_NEAREST)
    Ai = cv2.invertAffineTransform(A)
    return img, mask, Ai


def affine_detect(detector, img, mask=None, pool=None):
    '''
    affine_detect(detector, img, mask=None, pool=None) -> keypoints, descrs

    Apply a set of affine transormations to the image, detect keypoints and
    reproject them into initial image coordinates.
    See http://www.ipol.im/pub/algo/my_affine_sift/ for the details.

    ThreadPool object may be passed to speedup the computation.
    '''
    
    params = [(1.0, 0.0)]
    for t in 2**(0.5*np.arange(1,6)):
        for phi in np.arange(0, 180, 72.0 / t):
            params.append((t, phi))

    def f(p):
        t, phi = p
        timg, tmask, Ai = affine_skew(t, phi, img)
        keypoints, descrs = detector.detectAndCompute(timg, tmask)
        for kp in keypoints:
            x, y = kp.pt
            kp.pt = tuple( np.dot(Ai, (x, y, 1)) )
        if descrs is None:
            descrs = []
        return keypoints, descrs

    keypoints, descrs = [], []
    if pool is None:
        ires = it.imap(f, params)
    else:
        ires = pool.imap(f, params)

    for i, (k, d) in enumerate(ires):
        #print 'affine sampling: %d / %d\r' % (i+1, len(params)),
        keypoints.extend(k)
        descrs.extend(d)

    #print
    return keypoints, np.array(descrs)

def affine_detect2(detector, img, kps=[], mask=None, pool=None):
    '''
    affine_detect(detector, img, mask=None, pool=None) -> keypoints, descrs

    Apply a set of affine transormations to the image, detect keypoints and
    reproject them into initial image coordinates.
    See http://www.ipol.im/pub/algo/my_affine_sift/ for the details.

    ThreadPool object may be passed to speedup the computation.
    '''
    
    params = [(1.0, 0.0,kps)]
    for t in 2**(0.5*np.arange(1,6)):
        for phi in np.arange(0, 180, 72.0 / t):
            params.append((t, phi,kps))

    def f(p):
        t, phi,kps = p
        timg, tmask, Ai = affine_skew(t, phi, img)
        keypoints, descrs = detector.detectAndCompute(timg, tmask,kps)
        for kp in keypoints:
            x, y = kp.pt
            kp.pt = tuple( np.dot(Ai, (x, y, 1)) )
        if descrs is None:
            descrs = []
        return keypoints, descrs

    keypoints, descrs = [], []
    if pool is None:
        ires = it.imap(f, params)
    else:
        ires = pool.imap(f, params)

    for i, (k, d) in enumerate(ires):
        print 'affine sampling: %d / %d\r' % (i+1, len(params)),
        keypoints.extend(k)
        descrs.extend(d)

    print
    return keypoints, np.array(descrs)

def convertBGRtoOpponent(channels):
    '''
        Convert BGR to Opponent Colour Space
    '''
    #convert to float for this
    B=channels[0].astype(np.float64)
    G=channels[1].astype(np.float64)
    R=channels[2].astype(np.float64)
    #Do the conversion
    o1 = R-G/sqrt(2)
    o2 = R+G-2*B/sqrt(6)
    o3 = R+G+B/sqrt(3)
    #convert to uint8 for this
    o1 = o1.astype(np.uint8)
    o2 = o2.astype(np.uint8)
    o3 = o3.astype(np.uint8)
    return o1,o2,o3
    

def boostSalientColours(o1,o2,o3):
    #Convert to float type
    o1 = o1.astype(np.float64)
    o2 = o2.astype(np.float64)
    o3 = o3.astype(np.float64)
    #salient colour boosting
    o1 = (o1/(o1+o2+o3))*0.065
    o2 = (o2/(o1+o2+o3))*0.524
    o3 = (o3/(o1+o2+o3))*0.850
    #Convert back to natural type
    o1 = o1.astype(np.uint8)
    o2 = o2.astype(np.uint8)
    o3 = o3.astype(np.uint8)
    return o1,o2,o3

def processImage(image):
    img = cv2.imread(image)
    channels = cv2.split(img)
    
    o1,o2,o3 = convertBGRtoOpponent(channels)
    #o1,o2,o3 = boostSalientColours(o1,o2,o3)
    
    #cv2.imwrite("o1.jpg",o1)   
    #cv2.imwrite("o2.jpg",o2)
    #cv2.imwrite("o3.jpg",o3)
    
    detector = cv2.xfeatures2d.SIFT_create(nfeatures=50)
    pool=ThreadPool(processes = cv2.getNumberOfCPUs())
    keypoints, descriptors = affine_detect(detector, o1, pool=pool)
    kp1, desc1 = affine_detect(detector, o2, pool=pool)
    kp2, desc2 = affine_detect(detector, o3, pool=pool)
    
    keypoints.extend(kp1)
    keypoints.extend(kp2)
    #print descriptors.shape
    #print desc1.shape
    #print desc2.shape
    descriptors = np.concatenate((descriptors,desc1))
    descriptors = np.concatenate((descriptors,desc2))
    
    #Apply Hellinger Kernel
    descriptors = hellingerKernel(descriptors)
    return img,keypoints,descriptors

def addDesc(keypoints,descriptors):
    seen = {}
    identicals = {}
    combined_descriptors = []
    combined_keypoints = []
    i = 0
    
    for keypoint in keypoints:
        x,y = keypoint.pt
        if (x,y) in seen.keys():
            identicals[(x,y)].append(i)
        else:
            seen[(x,y)]=i
            identicals[(x,y)]=[i]
        i+=1
    
    for key in identicals.keys():
        combined_keypoints.append(keypoints[identicals[key][0]])
        for loc in identicals[key]:
            a=0
    
    #print identicals.values()
    #print len(keypoints)," has ",len(seen.keys())," unique keypoints"
    #new_descriptors = []
    #for keypoint in seen.keys():
        
def hellingerKernel(descs):
    # apply the Hellinger kernel by first L1-normalizing and taking the
	# square-root
	descs /= (descs.sum(axis=1, keepdims=True) + np.finfo(np.float32).eps)
	descs = np.sqrt(descs)
	#descs /= (np.linalg.norm(descs, axis=1, ord=2) + eps)
	return descs
	
def writeSift(name,keypoints,desc):
    #print "Saving: ",name+".sift"
    sift = open(name+".sift", 'w')
    sift.write("SIFT V4.0\n")
    sift.write(str(len(keypoints))+" ")
    sift.write(str(5)+" ")
    sift.write(str(128)+"\n")
    i=0
    for keypoint in keypoints:
        kpt = "%f %f 1 %f %f\n"%(keypoint.pt[1], keypoint.pt[1],keypoint.size, keypoint.angle)
        sift.write(kpt)
        for d in desc[i]:
            sift.write("%f "%d)
        sift.write("\n")
        i+=1
    sift.close()
    return

def processDirectory(direct):
    '''
        
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
    count = 0
    length = len(files)
    for filen in files:
        full_file =  directory+filen
        filename,ext = os.path.splitext(os.path.basename(filen))
        ext=ext.replace(".","") 
        if ext in extensions:
            #print "Processing:",full_file
            _,kps,desc = processImage(full_file)
            #print "Creating:",directory+filename+".sift"
            #print percentage
            percentage = count/length*100
            print "%",percentage,
            count+=1
            #write sift features to file
            writeSift(directory+filename,kps,desc)
            
    return

if __name__=='__main__':
    print "Affine Opponent Hellinger SIFT"
    image_path = sys.argv[1]
    print "Processing: ",image_path
    processDirectory(image_path)
    #img,kps,desc = processImage(image_path)
    #addDesc(kps,desc)
    #print "#Kps:",len(kps)
    #print "#Descs:",len(desc)
    #writeSift(image_path,kps,desc)
    
    #img2=img.copy()
    #cv2.drawKeypoints(img,kps,img2)
    #cv2.imshow("points",img2)
    #cv2.waitKey(0)
