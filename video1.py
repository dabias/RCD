import numpy as np
import cv2

cap = cv2.VideoCapture(0)

try:
    while(True):
        # Capture frame-by-frame
        ret, frame = cap.read()

        # Our operations on the frame come here
        blurred = cv2.medianBlur(frame,319)
        # Display the resulting frame
        cv2.imshow('frame',blurred)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

# When everything done, release the capture
finally:
    cap.release()
    cv2.destroyAllWindows()
