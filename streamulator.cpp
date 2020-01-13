/* Streamulator test platform
 * Original by Michiel van der Vlag, adapted by Matti Dreef
 */

#include "streamulator.h"


int main ()
{
	// Streams and data
	ap_uint<32> pixeldata[HEIGHT][WIDTH];
	hls::stream<pixel_data> inputStream;
	hls::stream<pixel_data> outputStream;
	pixel_data streamIn;
	pixel_data streamOut;
	uint32_t mask = 3;


	// Read input image
	cv::Mat sourceImg = cv::imread(INPUT_IMG);

	// A necessary conversion to obtain the right format...
	cv::cvtColor(sourceImg, sourceImg, CV_BGR2BGRA);


	// Write input data
	for (int rows=0; rows < HEIGHT; rows++)
		for (int cols=0; cols < WIDTH; cols++)
		{
			streamIn.data = sourceImg.at<int>(rows,cols);
			streamIn.user = (rows==0 && cols==0) ? 1 : 0;
			streamIn.last = (cols==WIDTH-1) ? 1 : 0;

			inputStream << streamIn;
		}

	// Call stream processing function
	while (!inputStream.empty())
		medianblur(inputStream, outputStream); // Add extra arguments here


	// Read output data
	for (int rows=0; rows < HEIGHT; rows++)
		for (int cols=0; cols < WIDTH; cols++)
		{
			outputStream.read(streamOut);
			pixeldata[rows][cols] = streamOut.data;
		}


	// Save image by converting data array to matrix
	// Depth or precision: CV_8UC4: 8 bit unsigned chars x 4 channels = 32 bit per pixel;
	cv::Mat imgCvOut(cv::Size(WIDTH, HEIGHT), CV_8UC4, pixeldata);
	cv::imwrite(OUTPUT_IMG, imgCvOut);


	return 0;
}

