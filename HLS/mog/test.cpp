#include "top.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/imgproc/imgproc.hpp"
#include "hls_opencv.h"

int main (int argc, char** argv)
{
    //获取图像数据
	cv::VideoCapture capture("input.mp4");
	cv::Mat src;
	cv::Mat dst(720, 1280, CV_8UC1);

	hls::stream<RGB_pixel> src_axi;
	hls::stream<GRAY_pixel> dst_axi;
	hls::stream<bgmodel_pixel> bgmodel_in;
	hls::stream<bgmodel_pixel> bgmodel_out;
	static pixel_gmm bgmodel_data[1280*720];

	while (true)
	{
		if (!capture.read(src))
			break;

		for(int row=0; row<720; row++)
			for(int col=0; col<1280; col++){
				int pixel = row*1280+col;
				bgmodel_pixel bgmodel;
				bgmodel.data = bgmodel_data[pixel];
				bgmodel.keep = 1;
				bgmodel.strb = 1;
				bgmodel.id = 0;
				bgmodel.dest = 0;
				bgmodel.user = 0;
				bgmodel.last = 0;
				if(row == 0 && col == 0)
					bgmodel.user = 1;
				else if(col == 1280-1)
					bgmodel.last = 1;
				bgmodel_in << bgmodel;
		}

		cvMat2AXIvideo(src, src_axi);
		mog(src_axi,dst_axi,bgmodel_in,bgmodel_out);
		AXIvideo2cvMat(dst_axi, dst);

		for(int row=0; row<720; row++)
			for(int col=0; col<1280; col++){
				//while(bgmodel_out.empty()){};
				int pixel = row*1280+col;
				bgmodel_pixel bgmodel = bgmodel_out.read();
				bgmodel_data[pixel] = bgmodel.data;
		}

		cv::imshow("image", dst);
		cv::waitKey(1);
	}
}
