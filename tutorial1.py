import numpy as np
import cv2

img = cv2.imread('parrot.jpg',1)
img = cv2.medianBlur(img,15)
#kernel = np.mat('[0 0 1 0 0;0 1 0 1 0; 1 0 0 0 1 ; 0 1 0 1 0; 0 0 1 0 0]')
#kernel = kernel/8
#img = cv2.filter2D(img,-1,kernel,(-1,-1))
#dim = img.shape
#centerx = int(dim[0]/2)
#centery = int(dim[1]/2)
#img = cv2.warpPolar(img,dim[0:2],(centerx,centery),1500,0)
#img = cv2.warpPolar(img,dim[0:2],(centerx,centery),1500,0 + cv2.WARP_INVERSE_MAP)
cv2.imshow('image',img)
cv2.waitKey(0)
cv2.destroyAllWindows()
