/* Streamulator test platform
 * Original by Michiel van der Vlag, adapted by Matti Dreef
 * Adapted to obey user and last signals by Douwe Brinkhorst and Shreya Kshirasagar
 */

#include "streamulator.h"


int main ()
{
	// Streams and data
	static ap_uint<32> pixeldata[HEIGHT][WIDTH];
	hls::stream<pixel_data> inputStream;
	hls::stream<pixel_data> outputStream;
	pixel_data streamIn;
	pixel_data streamOut;
	uint32_t k = 16;

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
		avgblur(inputStream, outputStream,k); // Add extra arguments here

	//read the output
	int rows = 0;
	// Wait for user signal
	outputStream.read(streamOut);
		while(!streamOut.user) {
			if (streamOut.last) {
				rows++;
			}
			outputStream.read(streamOut);
		}
	// counter used for determining on which row to store the pixels
	int rowsindex = 0;
	// moving to next row is controlled by last signal from stream
	while(rows < HEIGHT) {
		int cols = 0;
		while (!streamOut.last) {
			pixeldata[rowsindex][cols] = streamOut.data;
			cols++;
			outputStream.read(streamOut);
		}
		//process the final pixel of the row
		pixeldata[rowsindex][cols] = streamOut.data;
		rows++;
		rowsindex++;
		outputStream.read(streamOut);
	}


	// Save image by converting data array to matrix
	// Depth or precision: CV_8UC4: 8 bit unsigned chars x 4 channels = 32 bit per pixel;
	cv::Mat imgCvOut(cv::Size(WIDTH, HEIGHT), CV_8UC4, pixeldata);
	cv::imwrite(OUTPUT_IMG, imgCvOut);


	return 0;
}

