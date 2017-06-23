#include "opticalFlow.h"

using namespace cv;

opticalFlow::opticalFlow():
updated(false)
{

}

opticalFlow::~opticalFlow(){

}

bool opticalFlow::isUpdated(){
	return updated;
}

void opticalFlow::init(Mat img, float thres){

	cvtColor(img, prevFrame, CV_RGB2GRAY);
	threshold = thres;
}

void opticalFlow::updateBlock(Mat img, FloatRect& block, int frameWidth, int frameHeight){

	Mat flow;

	cvtColor(img, img, CV_RGB2GRAY);

	calcOpticalFlowFarneback(prevFrame, img, flow, 0.4, 1, 12, 2, 8, 1.2, 0);

	float totalTop = 0, totalBot = 0, totalLef = 0, totalRig = 0;
	for(int i = 0; i < img.cols; i += 10)
		for(int j = 0; j < img.rows; j += 10){

			Point2f fxy =  flow.at<Point2f>(j,i);

			if(j < img.rows/2)
				totalTop += fxy.y;
			else
				totalBot += fxy.y;

			if(i < img.cols/2)
				totalLef += fxy.x;
			else
				totalRig += fxy.x;

			line(img, Point(i, j), Point(cvRound(i + fxy.x) , cvRound(j + fxy.y)), CV_RGB(255, 255, 0));
			circle(img, Point(i, j), 1, CV_RGB(255, 0, 255));
		}

	totalRig /= (img.rows * img.cols/200);
	totalLef /= (img.rows * img.cols/200);
	totalTop /= (img.cols * img.rows/200);
	totalBot /= (img.cols * img.rows/200);

	float WIDTH = block.Width();
	float HEIGHT = block.Height();
	float X = block.XMin();
	float Y = block.YMin();

	updated = false;

	if(totalRig > threshold and totalLef < -1 * threshold and totalBot > threshold and totalTop < -1 * threshold){
		WIDTH *= 1.1; 

		if(WIDTH > frameWidth/2)
			WIDTH = frameWidth/2;

		HEIGHT *= 1.1;
		
		if(HEIGHT > frameHeight/2)
			HEIGHT = frameHeight/2;

		X -= (0.05) * WIDTH;
		
		if(X < 0)
			X = 0;
		else if(X > frameWidth - WIDTH)
			X = frameWidth - WIDTH;

		Y -= (0.05) * HEIGHT;

		if(Y < 0)
			Y = 0;
		else if(Y > frameHeight - HEIGHT)
			Y = frameHeight - HEIGHT;

		 updated = true;
	}
	else if(totalLef > threshold and totalRig < -1 * threshold and totalTop > threshold and totalBot < -1 * threshold){
		WIDTH *= 0.9; 

		if(WIDTH < frameWidth/15)
			WIDTH = frameWidth/15;
	
		HEIGHT *= 0.9;

		if(HEIGHT < frameHeight/15)
			HEIGHT = frameHeight/15;
	
		X += (0.05) * WIDTH;
		
		if(X < 0)
			X = 0;
		else if(X > frameWidth - WIDTH)
			X = frameWidth - WIDTH;

		Y += (0.05) * HEIGHT;

		if(Y < 0)
			Y = 0;
		else if(Y > frameHeight - HEIGHT)
			Y = frameHeight - HEIGHT;

		 updated = true;
	}

	
	imshow("flow", img);

	block.SetXMin(X);
	block.SetYMin(Y);
	block.SetWidth(WIDTH);
	block.SetHeight(HEIGHT);
}