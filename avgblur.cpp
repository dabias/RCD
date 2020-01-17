/*
 *  By Douwe Brinkhorst and Shreya Kshirasagar
 *	Many parrots were harmed in the making of this code
 */

#include <stdint.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define WIDTH 1280
#define HEIGHT 720
#define GB(v) ((v)&0xFF)
#define GG(v) (((v)&0xFF00)>>8)
#define GR(v) (((v)&0xFF0000)>>16)
#define SB(v) ((v)&0xFF)
#define SG(v) (((v)&0xFF)<<8)
#define SR(v) (((v)&0xFF)<<16)

typedef ap_axiu<32,1,1,1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;

void avgblur(pixel_stream &src, pixel_stream &dst,uint16_t k)
{
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis port=&src
#pragma HLS INTERFACE axis port=&dst
#pragma HLS INTERFACE s_axilite port=k
#pragma HLS PIPELINE II=1

// input k determines the aperture size
// this aperture is then a 2*k+1 by 2*k+1 grid
// this can be a user input
// kmax is the maximum k enabled by the hardware
const uint16_t kmax = 16;

if (k>kmax) {
	k = 0;
}

// buffer that stores several lines required for blur computation
static uint32_t buffer [WIDTH][2*kmax+2];

//index of where to store the incoming pixel
static int16_t storage_index = 0;
// index of the output pixel
static int16_t output_index = 0;
//counter that tracks when the next output frame starts
static uint16_t line_counter = 0;
// flag that is set to true when enough lines are available to start output
static bool past_init = false;
//flag that is set when the user signal comes in
static bool user_flag;


uint16_t channel1,channel2,channel3 = 0;
uint32_t channel1_out,channel2_out,channel3_out = 0;
int16_t i = 0;
int16_t j = 1;

pixel_data p_in;

// Load input data from source
src >> p_in;
pixel_data p_out;

//store the first part of the line in row 0
if (storage_index < k) {
    buffer[storage_index][0] = p_in.data;
}

else {
	// after the buffer has been shifted, store new pixels in row 1
    buffer[storage_index][1] = p_in.data;
}

// if past initialization, compute the kernel
if(past_init) {

	for (j= 1;j<(2*kmax+2);j++) {
		for (i = -kmax;i<=kmax;i++) {
			//only do computation if required by the user-defined k
			if ((j < (2*k+2))&&((i>=-k)&&(i<=k))) {
				//do boundary checks
				int16_t ii=i+output_index;
				int16_t jj=j;
				//deal with nonexistent pixels to the left of the frame
				if (ii<0)ii = 0;
				//deal with nonexistent pixels to the right of the frame
				if (ii>=WIDTH) ii = WIDTH;
				//ignore the bottom part of the buffer that contains data from the previous frame
				//instead pad numbers
				if ((line_counter>k)&(j>line_counter)) jj = line_counter;
				//ignore the top part of the buffer that contains data from the next frame
				//instead pad numbers
				if ((line_counter<=k)&(j<=line_counter)) jj = line_counter+1;
				//add the pixel to the sum
				channel1 += GR(buffer[ii][jj]);
				channel2 += GG(buffer[ii][jj]);
				channel3 += GB(buffer[ii][jj]);
			}
		}
	}
	//divide the sum to get the average, which is the output
	channel1_out = SR(channel1/(2*k+1));
	channel2_out = SG(channel2/(2*k+1));
	channel3_out = SB(channel3/(2*k+1));

	p_out.data = channel1_out|channel2_out|channel3_out;

}

//send user signal when a new output frame starts
if ((line_counter == k+1)&&(output_index==0)){
	p_out.user = 1;
} else {
	p_out.user = 0;
}

storage_index++;
output_index++;

if(p_in.last) {
     storage_index = 0;
}

// preparations for outputting new line
// triggers when the last pixel of a line is put out
if (storage_index == k) {
		// reset pixel output index
		output_index = 0;
		p_out.last = 1;
	     line_counter++;
		 if (user_flag) {
			 //if a new output frame starts
			 line_counter = 0;
			 user_flag = false;
		 }
	     if (line_counter > k){
	    	 //triggers when k lines have come in, enough to start the output
	    	 past_init = true;
	     }
         //shift the shift register
        for(j=1;j<(2*kmax+2);j++) {
        	for(i=0;i<WIDTH;i++) {
              	  buffer[i][j] = buffer[i][j-1];
        	}
        }
} else{
	p_out.last = 0;
}

if(p_in.user) {
	user_flag = true;
}

// Write pixel to destination
dst << p_out;

}
