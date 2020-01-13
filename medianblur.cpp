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

typedef ap_axiu<32,1,1,1> pixel_data;
typedef hls::stream<pixel_data> pixel_stream;

void medianblur(pixel_stream &src, pixel_stream &dst)
{
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE axis port=&src
#pragma HLS INTERFACE axis port=&dst
#pragma HLS INTERFACE s_axilite port=l
#pragma HLS INTERFACE s_axilite port=c
#pragma HLS INTERFACE s_axilite port=r
#pragma HLS PIPELINE II=1

const int k = 5;				//half-aperture of median blur
const int bufferheight = 2*k+1;

static uint32_t buffer [WIDTH][bufferheight];
static uint32_t tempbuffer [k];
static uint16_t i = 0;
static uint16_t d = k;
uint16_t j = 1;
uint16_t lowerX = 0;
uint16_t upperX = 0;

pixel_data p_in;

// Load input data from source
src >> p_in;
static pixel_data p_out = p_in;

if (d>0) {
	tempbuffer[i] = p_in.data;

        if (d == 1) {
              //shift the shift register
              for(j=1;j<=2*k;j++) {
              	  buffer[:][j] = buffer[:][j-1];
                  buffer[0:k-1][0] = tempbuffer;
              }
        }
}
else {
    buffer[i][0] = p_in.data;
}

if ((lowerX = i-2*k) < 0){
    lowerX = 0;
}
if ((upperX = i)> WIDTH-1){
    upperX = WIDTH-1;
}
//p_out.data = median(buffer[lowerX:upperX][]);


i++;
if(p_in.last) {
     d = k;
     i = 0;
}

// Write pixel to destination
dst << p_out;

}
