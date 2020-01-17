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

k = 0;

if (k>kmax) {
	k = kmax;
}

// buffer that stores the incoming pixels
static uint32_t buffer [WIDTH][HEIGHT];

//index of the column to store on
static int16_t storage_col= 0;
// index of the output pixel column
static int16_t output_col = 0;
//index of the row to store on
static int16_t storage_row = 0;
// index of the output pixel row
static uint16_t output_row = 0;

// flag that is set to true when enough lines are available to start output
static bool past_init = false;


uint32_t channel1,channel2,channel3 = 0;
uint32_t channel1_out,channel2_out,channel3_out = 0;
int16_t i = 0;
int16_t j = 1;

pixel_data p_in;

// Load input data from source
src >> p_in;
pixel_data p_out;

if(p_in.user) {
	storage_row = 0;
}

//store the data
buffer[storage_col][storage_row] = p_in.data;
//store the first part of the line in row 0

// if past initialization, compute the kernel
if(past_init) {

	for (j= -kmax;j<=kmax;j++) {
		for (i = -kmax;i<=kmax;i++) {
			//only do computation if required by the user-defined k
			if (((j >=-k)&&(j<=k))&&((i>=-k)&&(i<=k))) {
				//do boundary checks
				int16_t ii=i+output_col;
				int16_t jj=j+output_row;
				//deal with nonexistent pixels left of the frame
				if (ii<0)ii = 0;
				//deal with nonexistent pixels right of the frame
				if (ii>=WIDTH) ii = WIDTH-1;
				//deal with nonexistent pixels below the frame
				if (jj>=HEIGHT) jj = HEIGHT-1;
				//deal with nonexistent pixels above the frame
				if (jj<0) jj = 0;
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
if ((output_row == 0)&&(output_col==0)){
	p_out.user = 1;
} else {
	p_out.user = 0;
}

storage_col++;
output_col++;

if(p_in.last) {
    storage_col  = 0;
    storage_row++;
}

// preparations for outputting new line
// triggers when the last pixel of a line is put out
if (output_col == WIDTH) {
		// reset pixel output index
		output_col = 0;
		p_out.last = 1;
	     output_row++;
		 if (output_row==HEIGHT) {
			 //start a new output frame
			 output_row = 0;
		 }
	     if (output_row > k){
	    	 //triggers when k lines have come in, enough to start the output
	    	 past_init = true;
	     }
} else{
	p_out.last = 0;
}

// Write pixel to destination
dst << p_out;

}
