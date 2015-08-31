#
#   Affine-Opponent-Hellinger SIFT 
#   detecion & matching
#   Evan O'Keeffe 2015
#
#   14 Ardrone images
#   real	4m7.005s
#   user	14m42.259s
#   sys	    0m7.866s
#
import numpy as np
import cv2
# built-in modules
import os,sys
import itertools as it
from multiprocessing.pool import ThreadPool
import math
import pickle
import collections
from itertools import izip_longest

FLANN_INDEX_KDTREE = 1
FLANN_INDEX_LSH = 6

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

def bgr_detect(detector, img, desc=None,useProvided=False):
    '''
        Preform detection on individual B,G,R channels
        merge them and return keypoints and descriptors
    '''
    channels = cv2.split(img)
    if desc==None:
        keypoints, descrs = detector.detectAndCompute(channels[0],None)
    else:
        kps1, descrs = detector.detectAndCompute(channels[0],None,desc,useProvided)
    kps2, desc2 = detector.detectAndCompute(channels[1],None,descrs,useProvided)
    keypoints, descrs = detector.detectAndCompute(channels[2],None,desc2,useProvided)
    
    return keypoints, np.array(descrs)

def hsv_detect(detector, img, desc=None, useProvided=False):
    '''
        Preform detection on the H,S channels
        merge them and return keypoints and descriptors
    '''
    img2 = cv2.cvtColor(img,cv2.COLOR_BGR2HSV)
    channels = cv2.split(img)
    if desc==None:
        keypoints, descrs = detector.detectAndCompute(channels[0], None)
    else:
        keypoints, descrs = detector.detectAndCompute(channels[0], None, desc, useProvided)
    keypoints, descrs = detector.detectAndCompute(channels[1], None, descrs, useProvided)
    keypoints, descrs = detector.detectAndCompute(channels[2], None, descrs, useProvided)
    return keypoints, np.array(descrs)

def opponent_detect(detector, img, desc=None,useProvided=False):
    o1,o2,o3 = convertBGRtoOpponent(img)
    cv2.imwrite("o1.jpg",o1)
    cv2.imwrite("o2.jpg",o2)
    cv2.imwrite("o3.jpg",o3)
    #o1,o2,o3 = boostSalientColours(o1,o2,o3)
    if desc==None:
        keypoints, descrs = detector.detectAndCompute(o1,None)
    else:
        keypoints, descrs = detector.detectAndCompute(o1, None, desc, useProvided)
    keypoints, descrs = detector.detectAndCompute(o2, None, descrs, useProvided)
    keypoints, descrs = detector.detectAndCompute(o3, None, descrs, useProvided)
    return keypoints, np.array(descrs)

def convertBGRtoOpponent(img):
    '''
        Convert BGR to Opponent Colour Space
        Done in Matlab by:
        
        casted to float32 values
        values are normalized from 0->1
        
        o1 = (R-G)/math.sqrt(2)
        o2 = (R+G-2*B)/math.sqrt(6)
        o3 = (R+G+B)/math.sqrt(3)
        
        the values are renormalized back to 
        0->255 and then cast back to uint8
    '''
    channels = cv2.split(img)
    #convert to float first
    B=channels[0].astype(np.float32)
    G=channels[1].astype(np.float32)
    R=channels[2].astype(np.float32)
    #normalize to 0.0,1.0
    cv2.normalize(src=o1,dst=o1,alpha=0,beta=1,norm_type=cv2.NORM_MINMAX)
    cv2.normalize(src=o2,dst=o2,alpha=0,beta=1,norm_type=cv2.NORM_MINMAX)
    cv2.normalize(src=o3,dst=o3,alpha=0,beta=1,norm_type=cv2.NORM_MINMAX)
    #Do the conversion
    o1 = (R-G)/math.sqrt(2)
    o2 = (R+G-2*B)/math.sqrt(6)
    o3 = (R+G+B)/math.sqrt(3)
    #o1 = (0.5 *  (255+G-R))
    #o2 = (0.25 * (510+R+G-2*B))
    #o3 = (1.0/3.0 * (R+G+B))
    #First renormalize values between 0 and 1
    #cv2.normalize(src=o1,dst=o1,alpha=0,beta=1,norm_type=cv2.NORM_MINMAX)
    #cv2.normalize(src=o2,dst=o2,alpha=0,beta=1,norm_type=cv2.NORM_MINMAX)
    #cv2.normalize(src=o3,dst=o3,alpha=0,beta=1,norm_type=cv2.NORM_MINMAX)
    #Renormalize values for uint8, so 0->255
    cv2.normalize(src=o1,dst=o1,alpha=0,beta=255,norm_type=cv2.NORM_MINMAX)
    cv2.normalize(src=o2,dst=o2,alpha=0,beta=255,norm_type=cv2.NORM_MINMAX)
    cv2.normalize(src=o3,dst=o3,alpha=0,beta=255,norm_type=cv2.NORM_MINMAX)
    #convert to uint8 and return
    return o1.astype(np.uint8),o2.astype(np.uint8),o3.astype(np.uint8)
    

def boostSalientColours(o1,o2,o3):
    '''
        normalize the opponent channels
        then apply weights(0.850,0.524,0.065) to each
        individual channel
        Works in Matlab, not here yet
    '''
    #Convert to float type
    o1 = o1.astype(np.float32)
    o2 = o2.astype(np.float32)
    o3 = o3.astype(np.float32)
    #salient colour boosting
    try: o1 = o1/(o1+o2+o3)
    except:
        pass
    try: o2 = o2/(o1+o2+o3)
    except:
        pass
    try: o3 = o3/(o1+o2+o3)
    except:
        pass
    o1 *= 0.850
    o2 *= 0.524
    o3 *= 0.065
    #Convert back to unsigned char type
    o1 = o1.astype(np.uint8)
    o2 = o2.astype(np.uint8)
    o3 = o3.astype(np.uint8)
    return o1,o2,o3

def hellingerKernel(descs):
    # apply the Hellinger kernel by first L1-normalizing and taking the
	# square-root
	descs /= (descs.sum(axis=1, keepdims=True) + 1e-7)
	descs = np.sqrt(descs)
	#second L2 normalization not required apparently
	#descs /= (np.linalg.norm(descs, axis=1, ord=2) + 1e-7)
	#renormalize values between 0 and 512
	#descs = cv2.normalize(src=descs,dst=None,alpha=0,beta=512,norm_type=cv2.NORM_MINMAX)
	descs *= 512.0#How SIFT normalized values, didn't think of this one originally
	return descs

def process_affine(image,nfeatures=250):
    img = cv2.imread(image)
    detector = cv2.xfeatures2d.SIFT_create(nfeatures=250)#nfeatures=200
    pool=ThreadPool(processes = cv2.getNumberOfCPUs())
    keypoints, descriptors = affine_detect(detector, img, pool=pool)
    return keypoints,descriptors

def process_bgr(image,desc=None,nfeatures=4096):
    img = cv2.imread(image)
    detector = cv2.xfeatures2d.SIFT_create(nfeatures=nfeatures)
    if desc==None:
        keypoints, descriptors = bgr_detect(detector, img)
    else:
        keypoints, descriptors = bgr_detect(detector, img, desc, False)
    return keypoints,descriptors
    
def process_hsv(image,desc=None,nfeatures=4096):
    img = cv2.imread(image)
    detector = cv2.xfeatures2d.SIFT_create(nfeatures=nfeatures)
    if desc==None:
        keypoints, descriptors = hsv_detect(detector, img)
    else:
        keypoints, descriptors = hsv_detect(detector, img, desc)
    return keypoints,descriptors

def process_opponent(image,kps=None,nfeatures=4096):
    img = cv2.imread(image)
    detector = cv2.xfeatures2d.SIFT_create(nfeatures=nfeatures)
    if kps==None:
        keypoints, descriptors = opponent_detect(detector, img)
    else:
        keypoints, descriptors = opponent_detect(detector, img, kps)
    return keypoints,descriptors

def pickle_keypoints(keypoints,descriptors,filename):
    '''
        Store keypoints,Descriptors in Python
        Pickle File, used for the post detection
        matching process
    '''
    i=0
    temp_array = []
    for point in keypoints:
        temp = (point.pt,
        point.size,
        point.angle,
        point.response,
        point.octave,
        point.class_id,descriptors[i])
        ++i
        temp_array.append(temp)
    pickle.dump(temp_array,open(filename,"wb"))
    return

def unpickle_keypoints(filename):
    '''
        UnPickle the OpenCV Keypoints and Descriptors
    '''
    array = pickle.load(open(filename,"rb"))
    keypoints = []
    descriptors = []
    for point in array:
        temp_feature = cv2.KeyPoint(x=point[0][0],y=point[0][1],_size=point[1],_angle=point[2],_response=point[3],_octave=point[4],_class_id=point[5])
        temp_descriptor=point[6]
        keypoints.append(temp_feature)
        descriptors.append(temp_descriptor)
    return keypoints,np.array(descriptors)

def writeSift(name,keypoints,desc):
    '''
        Write Lowe SIFT binary format:
        
        total_features length_of_descriptors(128 in most cases)
        <y> <x> <scale> <orientation in radians> (Floats)
        <128 x descriptors>                      (unsigned char)
        
        <y> <x> <scale> <orientation in radians>
        <128 x descriptors>
        
        <y> <x> <scale> <orientation in radians>
        <128 x descriptors>
        .
        .
        .
        
    '''
    #print "Saving: ",name+".sift"
    sift = open(name+".sift", 'w')
    sift.write("%d %d \n"%(desc.shape[0],desc.shape[1]))
    for i in xrange(0,len(keypoints)):
        kpt = "%f %f %f %f\n"%(keypoints[i].pt[1], keypoints[i].pt[0],keypoints[i].size, keypoints[i].angle*math.pi/180.0)
        sift.write(kpt)
        for j in xrange(0,len(desc[i])): 
            sift.write("%d "%desc[i,j])
            if ((j+1)%19==0): sift.write("\n")
        sift.write("\n")
    sift.close()
    return

def match_images(kps1,kps2,desc1,desc2):
    '''
        Preform FLANN matching
    '''
    #Matches
    flann_params = dict(algorithm = FLANN_INDEX_KDTREE, trees = 5)
    matcher = cv2.FlannBasedMatcher(flann_params, {})
    raw_matches = matcher.knnMatch(desc1, trainDescriptors = desc2, k = 2) #2
    #Filter the matches
    p1, p2, kp_pairs = filter_matches(kps1, kps2, raw_matches)
    return p1,p2,kp_pairs

def ransac(p1,p2,kp_pairs):
    '''
        Preform RANSAC to remove outliers
    '''
    if len(p1) >= 4:
        H, status = cv2.findHomography(p1, p2, cv2.RANSAC, 5.0)
        print '%d / %d  inliers/matched' % (np.sum(status), len(status))
        # do not draw outliers (there will be a lot of them)
        kp_pairs = [kpp for kpp, flag in zip(kp_pairs, status) if flag]
        return kp_pairs
    return kp_pairs
    
def filter_matches(kp1, kp2, matches, ratio = 0.75):
    '''
        Filter matches based on Lowe's ratio test
    '''
    mkp1, mkp2 = [], []
    for m in matches:
        if len(m) == 2 and m[0].distance < m[1].distance * ratio:
            m = m[0]
            mkp1.append( kp1[m.queryIdx] )
            mkp2.append( kp2[m.trainIdx] )
    p1 = np.float32([kp.pt for kp in mkp1])
    p2 = np.float32([kp.pt for kp in mkp2])
    kp_pairs = zip(mkp1, mkp2)
    return p1, p2, kp_pairs

def write_matFile(f,filename1,filename2,kp_pairs):
    '''
        Write VisualSFM match file
    '''
    f.write("%s %s %d\n"%(filename1,filename2,len(kp_pairs)))
    for i in xrange(len(kp_pairs)): f.write(kp_pairs[i][0].pt[0])
    f.write("\n")
    for i in xrange(len(kp_pairs)): f.write(kp_pairs[i][1].pt[0])
    f.write("\n")

def removeDuplicates(keypoints,descriptors):
    '''
        Get rid of duplciate keypoints/descriptors
    '''
    seen = {}
    new_keypoints=[]
    new_descriptors=[]
    for index in xrange(0,len(keypoints)):
        point = keypoints[index].pt
        if not (point[0],point[1]) in seen:
            seen[point]=[]
            new_keypoints.append(keypoints[index])
            new_descriptors.append(descriptors[index])
    print len(keypoints)-len(new_keypoints),"duplicates removed"
    return new_keypoints,np.array(new_descriptors)

def appendDescriptors(kps,desc,kps2,desc2):
    kps.extend(kps2)
    desc = np.vstack((desc,desc2))
    return kps,desc

def processDirectory(direct,applyRootSIFT=False):
    '''
        Do Feature detection in the specified directory
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
        ext=ext.replace(".","") 
        if ext in extensions:
            #print "Detection Running:",full_file
            kps,desc = process_affine(full_file)
            kps2,desc2 = process_bgr(full_file,desc)
            #kps3,desc3 = process_opponent(full_file,desc)
            
            kps,desc = appendDescriptors(kps,desc,kps2,desc2)
            #kps,desc = appendDescriptors(kps,desc,kps3,desc3)
            kps,desc = removeDuplicates(kps,desc)
            
            print "#Kps:",len(kps),",#Descs:",len(desc)
            if applyRootSIFT:
                 #Apply Hellinger Kernel to the descriptors
                desc = hellingerKernel(desc)
            #write sift features to file
            writeSift(directory+filename,kps,desc)
            
            #pickle_keypoints(kps,desc,directory+filename+".features")
            #used for the matching phase
            #features[full_file] = directory+filename+".features"
            
    #Do the feature matching
    #for key in features.keys():
    #    filename,ext = os.path.splitext(os.path.basename(key))
    #    matching_file = directory+filename+".mat"
    #    f = open(matching_file,"w")
    #    keypoints1,descriptors1 = unpickle_keypoints(features[key])
    #    for key2 in features.keys():
    #        if not key == key2:
    #            keypoints2,descriptors2 = unpickle_keypoints(features[key2])
    #            p1,p2,kp_pairs=match_images(keypoints1,keypoints2,descriptors1,descriptors2)
    #            kp_pairs = ransac(p1,p2,kp_pairs)
    #    f.close()         
            
    return

if __name__=='__main__':
    print "Affine Opponent Hellinger SIFT"
    image_path = sys.argv[1]
    if len(sys.argv)>2:
        applyRSIFT = bool(sys.argv[2])
        print "Applying RootSIFT:",applyRSIFT
    else:
        applyRSIFT=False
    print "Processing: ",image_path
    processDirectory(image_path,applyRSIFT)
    
    #img,kps,desc = processImage(image_path)
    #addDesc(kps,desc)
    #print "#Kps:",len(kps)
    #print "#Descs:",len(desc)
    #writeSift(image_path,kps,desc)
    
    #img2=img.copy()
    #cv2.drawKeypoints(img,kps,img2)
    #cv2.imshow("points",img2)
    #cv2.waitKey(0)
