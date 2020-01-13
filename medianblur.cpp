/*
 *  By Douwe Brinkhorst and Shreya Kshirasagar
 *  Streaming interface related parts based from work by Michiel van der Vlag and Matti Dreef
 */

#include <stdint.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define WIDTH 1280
#define HEIGHT 720

typedef ap_axiu<32,1,1,1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;

void medianblur(pixel_stream &src, pixel_stream &dst)
{
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis port=&src
#pragma HLS INTERFACE axis port=&dst
#pragma HLS PIPELINE II=1

// k determines the aperture size
// this aperture is then a 2*k+1 by 2*k+1 grid
const int k = 5;
const int bufferheight = 2*k+1;

// buffer that stores several lines required for blur computation
static uint32_t buffer [WIDTH][bufferheight];
// a temporary buffer that stores incoming data of the new line while the previous one is still being processed
static uint32_t tempbuffer [k];

//index of where to store the incoming pixel
static uint16_t storage_index = 0;
// index of the next pixel to go to output
static int16_t output_index = 0;

uint16_t i = 0;
uint16_t j = 1;
uint16_t lowerX = 0;
uint16_t upperX = WIDTH-1;

pixel_data p_in;

// Load input data from source
src >> p_in;
static pixel_data p_out = p_in;

//store the first part of the line in a temporary buffer
if (storage_index<k) {
	tempbuffer[storage_index] = p_in.data;
}
// preparations outputting new line
else if (storage_index == k) {
		// reset pixel output index
		output_index = 0;
        for(j=1;j<=2*k;j++) {
            //shift the shift register
        	for(i=0;i<WIDTH-1;i++) {
              	  buffer[i][j] = buffer[i][j-1];
        	}
        	// move temporary buffer to main buffer
        	for(i=0;i<k-1;i++) {
                  buffer[i][0] = tempbuffer[i];
        	}
        }
        buffer[k][0] = p_in.data;
}

else {
	// store incoming pixel
    buffer[storage_index][0] = p_in.data;
}

// deal with nonexistent pixels to the left of the image
if ((lowerX = output_index-k) < 0){
    lowerX = 0;
}

// deal with nonexistent pixels to the right of the image
if ((upperX = output_index+k)> WIDTH-1){
    upperX = WIDTH-1;
}
//TODO: calculate the actual median
//p_out.data = median(buffer[lowerX:upperX][]);


storage_index++;
output_index++;
if(p_in.last) {
     storage_index = 0;
}

// Write pixel to destination
dst << p_out;

}
