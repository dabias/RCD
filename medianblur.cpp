/*
 *  By Douwe Brinkhorst and Shreya Kshirasagar
 */

#include <stdint.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define WIDTH 1280
#define HEIGHT 720

// k determines the aperture size
// this aperture is then a 2*k+1 by 2*k+1 grid
#define k 0

typedef ap_axiu<32,1,1,1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;

void medianblur(pixel_stream &src, pixel_stream &dst)
{
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis port=&src
#pragma HLS INTERFACE axis port=&dst
#pragma HLS PIPELINE II=1

const int bufferheight = 2*k+1;
int m;

// buffer that stores several lines required for blur computation
static uint32_t buffer [WIDTH][bufferheight] = {0};
// a temporary buffer that stores incoming data of the new line while the previous one is still being processed

static uint32_t tempbuffer [k+1] = {0};


//index of where to store the incoming pixel
static uint16_t storage_index = 0;
// index of the next pixel to go to output
static int16_t output_index = WIDTH;
// counter that suppresses output during initialization
static uint16_t init_counter = k;

int channel1,channel2,channel3,channel4 = 0;
int channel1_out,channel2_out,channel3_out,channel4_out;
int weight = 0;
int pixel_val = 0;
uint16_t i = 0;
uint16_t j = 1;
uint16_t lowerX = 0;
uint16_t upperX = WIDTH;

pixel_data p_in;

// Load input data from source
src >> p_in;
pixel_data p_out = p_in;

//store the first part of the line in a temporary buffer
if (storage_index < k) {
	tempbuffer[storage_index] = p_in.data;
}
// preparations for outputting new line
else if (storage_index == k) {
		// reset pixel output index
		output_index = 0;
        for(j=1;j<2*k+1;j++) {
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
if ((upperX = output_index+1+k)> WIDTH){
    upperX = WIDTH;
}


// if past initialization, compute the kernel
if(init_counter == 0 & output_index<WIDTH) {
	// kernel is a uniform blur (for now at least)
	//TODO: calculate the actual median
	//compute the weight
	weight = 1/((2*k+1)*(upperX-lowerX));
	//weight = 1;
	for (j= 0;j<2*k+1;j++) {
		for (i = lowerX;i<upperX;i++) {
			 channel1 += weight*(buffer[i][j]&0xFF000000);
			 channel2 += weight*(buffer[i][j]&0x00FF0000);
			 channel3 += weight*(buffer[i][j]&0x0000FF00);
			 channel4 += weight*(buffer[i][j]&0x000000FF);
		}
	}
	channel1_out = (channel1)&0xFF000000;
	channel2_out = (channel2)&0x00FF0000;
	channel3_out = (channel3)&0x0000FF00;
	channel4_out = (channel4)&0x000000FF;
	p_out.data = channel1_out|channel2_out|channel3_out|channel4_out;

}


storage_index++;
output_index++;
if(p_in.last) {
     storage_index = 0;
     if (init_counter > 0) {
     init_counter--;
     }
}

// Write pixel to destination
dst << p_out;

}
