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

void avgblur(pixel_stream &src, pixel_stream &dst,uint16_t x, uint16_t y)
{
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis port=&src
#pragma HLS INTERFACE axis port=&dst
#pragma HLS INTERFACE s_axilite port=x
#pragma HLS INTERFACE s_axilite port=y
#pragma HLS PIPELINE II=1

// input x and y determines the aperture size
// this aperture is then a 2*x+1 by 2*y+1 grid
// this can be a user input
// kmax is the maximum x and y enabled by the hardware
const uint16_t kmax = 16;

y=5;
x=16;

if (x>kmax) {
	x = kmax;
}
if (y>kmax) {
	y = kmax;
}

// buffer that stores several lines required for blur computation
static uint32_t buffer [WIDTH][2*kmax+2];
//buffers that stores the averages of the vertical dimension, per color channel
static uint32_t channel1buffer [WIDTH],channel2buffer [WIDTH],channel3buffer [WIDTH];
//virtual buffer that maps storage to computation
uint32_t virtual_buffer [2*kmax+2];

//column to store the incoming pixel
static uint16_t storage_col = 0;
//row offset for storing the incoming pixel
static int16_t storage_row = 2*kmax+1;
// column of the output pixel
static int16_t output_col = 0;
//row of the output pixel
static uint16_t output_row = WIDTH+1;
//offset used in buffer mapping
static uint16_t output_row_offset = 0;


uint32_t channel1,channel2,channel3 = 0;
uint32_t channel1_out,channel2_out,channel3_out = 0;
int16_t i;
int16_t j;

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
    virtual_buffer[(j+output_row_offset)%(2*kmax+1)] = buffer[storage_col][j];
}

channel1buffer[storage_col] = 0;
channel2buffer[storage_col] = 0;
channel3buffer[storage_col] = 0;

//compute the kernel in the vertical axis and store it
for (j= -kmax;j<=(kmax);j++) {
	if ((j>=-y)&&(j<=y)) {
		int16_t jj = j+kmax+1;
		channel1buffer[storage_col] += GR(virtual_buffer[jj]);
		channel2buffer[storage_col] += GG(virtual_buffer[jj]);
		channel3buffer[storage_col] += GB(virtual_buffer[jj]);
	}
}
channel1buffer[storage_col] = channel1buffer[storage_col]/(2*y+1);
channel2buffer[storage_col] = channel2buffer[storage_col]/(2*y+1);
channel3buffer[storage_col] = channel3buffer[storage_col]/(2*y+1);

//compute the kernel in the horizontal axis and output it
for (i = -kmax;i<=kmax;i++) {
	if ((i>=-x)&&(i<=x)) {
		int16_t ii = i+output_col;
		if (ii<0) ii = 0;
		if (ii>WIDTH-1) ii = WIDTH-1;
		channel1 += channel1buffer[ii];
		channel2 += channel2buffer[ii];
		channel3 += channel3buffer[ii];
	}
	//divide the sum to get the average, which is the output
	channel1_out = SR(channel1/(2*x+1));
	channel2_out = SG(channel2/(2*x+1));
	channel3_out = SB(channel3/(2*x+1));

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
	     if(output_row_offset>(2*kmax+1)){
	    	 output_row_offset=0;
	     }
	     if (storage_row == y+1){
	    	 //triggers when y lines have come in, enough to start the output
			 output_row = 0;
	     }
} else{
	p_out.last = 0;
}


if(p_in.last) {
     storage_col = 0;
     storage_row--;
     if(storage_row<0){
    	 storage_row=2*kmax+1;
     }
}

// Write pixel to destination
dst << p_out;

}
