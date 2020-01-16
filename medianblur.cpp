/*
 *  By Douwe Brinkhorst and Shreya Kshirasagar
 *
 */

#include <stdint.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define WIDTH 1280
#define HEIGHT 720
#define GR(v) ((v)&0xFF)
#define GG(v) (((v)&0xFF00)>>8)
#define GB(v) (((v)&0xFF0000)>>16)
#define SR(v) ((v)&0xFF)
#define SG(v) (((v)&0xFF)<<8)
#define SB(v) (((v)&0xFF)<<16)

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

int m;

// buffer that stores several lines required for blur computation
static uint32_t buffer [WIDTH][2*k+2] = {0};

//index of where to store the incoming pixel
static int16_t storage_index = 0;
// index of the next pixel to go to output
static int16_t output_index = WIDTH;
//counter that tracks when the next output frame starts
static uint16_t line_counter = k;
// counter that suppresses output during initialization
static uint16_t init_counter = line_counter;
// flag that is set to true when enough lines are available to start output
static bool past_init = false;

// channel 1 = alpha, 2 = red, 3 = green, 4 = blue
uint64_t channel1,channel2,channel3,channel4 = 0;
uint32_t channel1_out,channel2_out,channel3_out,channel4_out;
int weight = 0;
int pixel_val = 0;
int16_t i = 0;
int16_t j = 1;
int16_t lowerX = 0;
int16_t upperX = WIDTH;

pixel_data p_in;

// Load input data from source
src >> p_in;
pixel_data p_out = p_in;

//store the first part of the line in row 0
if (storage_index < k) {
    buffer[storage_index][0] = p_in.data;
}

else {
	// after the buffer has been shifted, store new pixels in row 1
    buffer[storage_index][1] = p_in.data;
}



// if past initialization, compute the kernel
if(past_init && (output_index<WIDTH)) {
	// kernel is a uniform blur (for now at least)
	//TODO: calculate the actual median
	lowerX = output_index-k;
	upperX = output_index+k;

	//compute the weight
	//weight could also be defined as 1/n instead of n,
	//which would allow for multiplication instead of division
	//however, then 0<n<1 which would require using float
	weight = (2*k+1)^2;
	//i = output_index;
	for (j= 1;j<2*k+2;j++) {
		for (i = lowerX;i<=upperX;i++) {
			//if the pixel exists
			if ((i>=0)&&(i<(WIDTH))) {
				channel2 += GR(buffer[i][j]);
				channel3 += GG(buffer[i][j]);
				channel4 += GB(buffer[i][j]);
			}
		}
	}

	//channel1_out = ((channel1)/weight)&0xFF000000;
	channel2_out = ((channel2)/weight)&0x00FF0000;
	channel3_out = ((channel3)/weight)&0x0000FF00;
	channel4_out = ((channel4)/weight)&0x000000FF;

	//set specific channels to pass through
	//channel1_out = buffer[output_index][k+1]&0xFF000000;
	//channel2_out = buffer[output_index][k+1]&0x00FF0000;
	//channel3_out = buffer[output_index][k+1]&0x0000FF00;
	//channel4_out = buffer[output_index][k+1]&0x000000FF;

	//set specific channels to be off
	channel1_out = 0;
	//channel2_out = 0;
	//channel3_out = 0;
	//channel4_out = 0;

	//p_out.data = buffer[output_index][k];
	p_out.data = channel2_out|channel3_out|channel4_out;

}

storage_index++;
output_index++;

//send user signal when a new output frame starts
if (line_counter == 0){
	p_out.user = 1;
} else {
	p_out.user = 0;
}

if(p_in.last) {
     storage_index = 0;
     if (line_counter > 0) {
     line_counter--;
     } else if (line_counter == 0){
    	 //triggers when k lines have come in, enough to start the output
    	 past_init = true;
     }
}

// preparations for outputting new line
// triggers when the last pixel of a line is put out
if (storage_index == k) {
		// reset pixel output index
		output_index = 0;
		p_out.last = 1;
        for(j=1;j<2*k+1;j++) {
            //shift the shift register
        	for(i=0;i<WIDTH;i++) {
              	  buffer[i][j] = buffer[i][j-1];
        	}
        }
} else{
	p_out.last = 0;
}

if(p_in.user) {
	line_counter = k;
}

// Write pixel to destination
dst << p_out;

}
