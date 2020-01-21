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
const uint16_t kmax = 5;

if (k>kmax) {
	k = kmax;
}

// buffer that stores several lines required for blur computation
static uint32_t buffer [WIDTH][2*kmax+2];
//virtual buffer that maps storage to computation
uint32_t virtual_buffer [2*kmax+1][2*kmax+2];

//column to store the incoming pixel
static int16_t storage_col = 0;
//row offset for storing the incoming pixel
static int16_t storage_row = 2*k+1;
// column of the output pixel
static int16_t output_col = k;
//row of the output pixel
static uint16_t output_row = 0;
//offset used in buffer mapping
static uint16_t output_row_offset = 0;
// flag that is set to true when enough lines are available to start output
static bool past_init = false;


uint64_t channel1,channel2,channel3 = 0;
uint32_t channel1_out,channel2_out,channel3_out = 0;
int16_t i = 0;
int16_t j = 1;

pixel_data p_in;

// Load input data from source
src >> p_in;
pixel_data p_out;

//store the data
    buffer[storage_col][storage_row] = p_in.data;


    //send user signal when a new output frame starts
    if ((output_row == 0)&&(output_col==0)){
    	p_out.user = 1;
    } else {
    	p_out.user = 0;
    }

//map buffer to virtual buffer
    for (j=0;j<(2*kmax+2);j++) {
		for (i = -kmax;i<=kmax;i++) {
			int16_t ii=i+output_col;
			//deal with nonexistent pixels to the left of the frame
			if (ii<0) ii = 0;
			//deal with nonexistent pixels to the right of the frame
			if (ii>=WIDTH) ii = WIDTH;
    		virtual_buffer[i+kmax][(j+output_row_offset)%(2*k+1)] = buffer[ii][j];
    	}
    }

//compute the kernel
if(past_init) {
	for (j= 1;j<(2*kmax+2);j++) {
		for (i = 0;i<=2*kmax;i++) {
			//only do computation if required by the user-defined k
			if ((j < (2*k+2))&&((i>=-k)&&(i<=k))) {
				//do boundary checks
				int16_t jj=j;
				/*
				//ignore the bottom part of the buffer that contains data from the previous frame
				//instead pad numbers
				if ((output_row>k)&(jj>output_row)) jj = output_row;
				//ignore the top part of the buffer that contains data from the next frame
				//instead pad numbers
				if ((output_row)&(j<=output_row)) jj = output_row+1;
				*/
				//add the pixel to the sum
				channel1 += GR(virtual_buffer[i][jj]);
				channel2 += GG(virtual_buffer[i][jj]);
				channel3 += GB(virtual_buffer[i][jj]);
			}
		}
	}
	//divide the sum to get the average, which is the output
	channel1_out = SR(channel1/(2*k+1)^2);
	channel2_out = SG(channel2/(2*k+1)^2);
	channel3_out = SB(channel3/(2*k+1)^2);

	p_out.data = channel1_out|channel2_out|channel3_out;

}

storage_col++;
output_col++;


// preparations for outputting new line
// triggers when the last pixel of a line is put out
if (output_col == WIDTH) {
	p_out.last = 1;
		// reset pixel output column
		output_col = 0;
	     output_row++;
	     output_row_offset++;
		 if (output_row==HEIGHT) {
			 //trigger a new output frame
			 output_row = 0;
		 }
	     if(output_row_offset>(2*k+1)){
	    	 output_row_offset=0;
	     }
	     if ((storage_row == k+1) && past_init==false){
	    	 //triggers when k lines have come in, enough to start the output
	    	 past_init = true;
			 output_row = 0;
			 output_row_offset=k+1;
	     }
} else{
	p_out.last = 0;
}


if(p_in.last) {
     storage_col = 0;
     storage_row--;
     if(storage_row<0){
    	 storage_row=2*k+1;
     }
}

// Write pixel to destination
dst << p_out;

}
