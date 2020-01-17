/* Streamulator test platform
 * Original by Michiel van der Vlag, adapted by Matti Dreef
 * Adapted to obey user and last signals by Douwe Brinkhorst and Shreya Kshirasagar
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
	uint32_t k = 7;

	// Read input image
	cv::Mat sourceImg = cv::imread(INPUT_IMG);

	// A necessary conversion to obtain the right format...
	cv::cvtColor(sourceImg, sourceImg, CV_BGR2BGRA);


	// Write input data
	for (int i=0;i<2;i++){
		for (int rows=0; rows < HEIGHT; rows++)
			for (int cols=0; cols < WIDTH; cols++)
			{
				streamIn.data = sourceImg.at<int>(rows,cols);
				streamIn.user = (rows==0 && cols==0) ? 1 : 0;
				streamIn.last = (cols==WIDTH-1) ? 1 : 0;

				inputStream << streamIn;
			}
	}

	// Call stream processing function
	while (!inputStream.empty())
		avgblur(inputStream, outputStream,k); // Add extra arguments here

	//read the output
	// Wait for user signal
	outputStream.read(streamOut);
		while(!streamOut.user) {
			outputStream.read(streamOut);
		}
	// counter used for determining on which row to store the pixels
	// moving to next row is controlled by last signal from stream
	bool flag = false;
	int rows = 0;
	while(!outputStream.empty()) {
		if (streamOut.user) {
			int rows = 0;
			if (flag) {
				// if one frame has been completed, write it to disk
				cv::Mat imgCvOut(cv::Size(WIDTH, HEIGHT), CV_8UC4, pixeldata);
				cv::imwrite(OUTPUT1_IMG, imgCvOut);
			}
			flag  = true;
		}
		int cols = 0;
		while (!streamOut.last) {
			pixeldata[rows][cols] = streamOut.data;
			outputStream.read(streamOut);
			cols++;
			if (outputStream.empty()) break;
		}
		//process the final pixel of the row
		pixeldata[rows][cols] = streamOut.data;
		if (!outputStream.empty()) outputStream.read(streamOut);
		rows++;
	}


	// Save image by converting data array to matrix
	// Depth or precision: CV_8UC4: 8 bit unsigned chars x 4 channels = 32 bit per pixel;
	cv::Mat imgCvOut(cv::Size(WIDTH, HEIGHT), CV_8UC4, pixeldata);
	cv::imwrite(OUTPUT1_IMG, imgCvOut);


	return 0;
}

