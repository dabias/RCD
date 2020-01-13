import numpy as np

int r = 1
width = 1280
height = 720
buffer = zeros(width,2*r+1)
int i = 0
int d = r

try:
    while(True):
        if d>0:
            tempbuffer(i) = data
            
            if d == 1:
                #shift the shift register
                for j in 1:2*r:
                    buffer(:,j)=buffer(:,j-1)
                    buffer(0:r-1,0)=tempbuffer
        else:
            buffer(i,0) = data
            data_out = medianBlur(i-r,r)
        i++
        if(last):
            d = r
            i = 0

def medianBlur(int x, int y):
    if (lowerX = x-r) < 0:
        lowerX = 0
    if (upperX = x+r)> width-1:
        upperX = width-1
    data = buffer(lowerX:upperX,:)
    out = median(data)
    return out
