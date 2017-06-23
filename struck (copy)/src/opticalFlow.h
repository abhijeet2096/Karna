#ifndef OPTICAL_FLOW_H
#define OPTICAL_FLOW_H

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "Rect.h"

using namespace cv;

class opticalFlow{

private:

	Mat prevFrame;
	float threshold;
	bool updated;

public:

	opticalFlow();
	~opticalFlow();

	void init(Mat img, float thres);
	void updateBlock(Mat img, FloatRect& block, int frameWidth, int frameHeight);

	bool isUpdated();
};

#endif