/* Streamulator test platform
 * Original by Michiel van der Vlag, adapted by Matti Dreef
 */

#ifndef INC_H
#define INC_H


#include <stdint.h>
#include <hls_stream.h>
#include <hls_video.h>
#include <hls_opencv.h>
#include <ap_axi_sdata.h>


// Image dimensions
#define WIDTH 1280
#define HEIGHT 720


// Pixel and stream types
typedef ap_axiu<32,1,1,1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;


// Stream processing function
void stream (pixel_stream&, pixel_stream&);
void invstripe(pixel_stream &src, pixel_stream &dst, uint32_t mask);
void medianblur(pixel_stream &src, pixel_stream &dst);


// Streamulator image paths
#define INPUT_IMG  "/home/douwe/Downloads/PYNQ_lab2019/examples/parrot.jpg"
#define OUTPUT_IMG "/home/douwe/Downloads/PYNQ_lab2019/examples/output.jpg"


#endif // INC_H
