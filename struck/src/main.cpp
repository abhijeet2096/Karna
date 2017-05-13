/* 
 * Struck: Structured Output Tracking with Kernels
 * 
 * Code to accompany the paper:
 *   Struck: Structured Output Tracking with Kernels
 *   Sam Hare, Amir Saffari, Philip H. S. Torr
 *   International Conference on Computer Vision (ICCV), 2011
 * 
 * Copyright (C) 2011 Sam Hare, Oxford Brookes University, Oxford, UK
 * 
 * This file is part of Struck.
 * 
 * Struck is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Struck is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Struck.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */
 
#include "Tracker.h"
#include "Config.h"

#include <iostream>
#include <string>
#include <fstream>

#include <opencv/cv.h>
#include <Eigen/Core>
#include <opencv/highgui.h>

using namespace std;
using namespace cv;
using namespace Eigen;

void moveQuadcopter(const FloatRect& bb){

	if(bb.XMax() < 600 && bb.XMin() > 400 && bb.YMin() > 300 && bb.YMax() < 500)
		//Stop Quadcopter

	if(bb.XMax() > 600 || bb.XMin() < 400){
		if(bb.XMax() > 600){
			cout<<"rotate Left";
		}
		else if(bb.XMin() < 400){
			cout<<"rotate Right";
		}
	}

	if(bb.YMin() < 300 || bb.YMax() > 500){
		if(bb.YMin() < 300){
			cout<<"move Forward";
		}
		else if(bb.YMax() > 500){
			cout<<"move Backward";	
		}
	}
}

void rectangle(Mat& rMat, const FloatRect& rRect, const Scalar& rColour)
{
	IntRect r(rRect);
	rectangle(rMat, Point(r.XMin(), r.YMin()), Point(r.XMax(), r.YMax()), rColour);
}

int main(int argc, char* argv[])
{
	// read config file
	string configPath = "config.txt";
	if (argc > 1)
	{
		configPath = argv[1];
	}
	Config conf(configPath);
	cout << conf << endl;
	
	if (conf.features.size() == 0)
	{
		cout << "error: no features specified in config" << endl;
		return EXIT_FAILURE;
	}
	
	ofstream outFile;
	if (conf.resultsPath != "")
	{
		outFile.open(conf.resultsPath.c_str(), ios::out);
		if (!outFile)
		{
			cout << "error: could not open results file: " << conf.resultsPath << endl;
			return EXIT_FAILURE;
		}
	}
	
	// if no sequence specified then use the camera
	bool useCamera = (conf.sequenceName == "");
	
	VideoCapture cap;
	
	int startFrame = -1;
	int endFrame = -1;
	FloatRect initBB;
	string imgFormat;
	float scaleW = 1.f;
	float scaleH = 1.f;
	
	if (!cap.open(0))
	{
		cout << "error: could not start camera capture" << endl;
		return EXIT_FAILURE;
	}
	startFrame = 0;
	endFrame = INT_MAX;
	Mat tmp;
	cap >> tmp;
	scaleW = (float)conf.frameWidth/tmp.cols;
	scaleH = (float)conf.frameHeight/tmp.rows;

	initBB = IntRect(conf.frameWidth/2-conf.liveBoxWidth/2, conf.frameHeight/2-conf.liveBoxHeight/2, conf.liveBoxWidth, conf.liveBoxHeight);
	cout << "Press\n'i' to initialise tracker"<<endl;
	cout << "'s' to add current frame as ground truth" << endl;
	cout << "'t' to start tracking" << endl;
	cout << "'l' to load profile"<<endl;
	cout << "'e' to export data"<<endl;
	cout << "'p' to pause tracking"<<endl;
	cout << "'r' to reset ground truth"<<endl;
	cout << "'esc' to exit"<<endl;
	
	Tracker tracker(conf);
	if (!conf.quietMode)
	{
		namedWindow("result");
	}

	int offsetx = 0, offsety = 0;
	
	Mat result(conf.frameHeight, conf.frameWidth, CV_8UC3);
	bool paused = false;
	bool doInitialise = false;
	bool objectDetected = true;
	srand(conf.seed);
	for (int frameInd = startFrame; frameInd <= endFrame; ++frameInd)
	{
		initBB = IntRect(conf.frameWidth/2-conf.liveBoxWidth/2 + offsetx, conf.frameHeight/2-conf.liveBoxHeight/2 + offsety, conf.liveBoxWidth, conf.liveBoxHeight);
		Mat frame;
		
		Mat frameOrig;
		cap >> frameOrig;
		resize(frameOrig, frame, Size(conf.frameWidth, conf.frameHeight));
		flip(frame, frame, 1);
		frame.copyTo(result);
		if (doInitialise){

			rectangle(result, initBB, CV_RGB(255, 255, 0));
		}
		
		if (!doInitialise && tracker.IsInitialised())
		{
			tracker.Track(frame, Point(conf.frameWidth, conf.frameHeight), Point(conf.liveBoxWidth, conf.liveBoxHeight));
			
			if (!conf.quietMode && conf.debugMode)
			{
				tracker.Debug();
			}
			
			rectangle(result, tracker.GetBB(), CV_RGB(0, 0, 255));

			if(tracker.isDetected())
				moveQuadcopter(tracker.GetBB());
			else{
				//Stop quadcopter
			}
			
			if (outFile)
			{
				const FloatRect& bb = tracker.GetBB();
				outFile << bb.XMin()/scaleW << "," << bb.YMin()/scaleH << "," << bb.Width()/scaleW << "," << bb.Height()/scaleH << endl;
			}
		}
		
		if (!conf.quietMode)
		{
			imshow("result", result);
			int key = waitKey(paused ? 0 : 1);
			if (key != -1)
			{
				if (key == 27) // esc
				{
					break;
				}
				else if (key == 112) // p
				{
					paused = !paused;
				}
				else if (key == 105 && useCamera)//i
				{
					doInitialise = true;
				}
				else if (key == 116 && useCamera){ //t
					doInitialise = false;
				}
				else if (key == 115 && useCamera){//s

					tracker.Initialise(frame, initBB);
				}
				else if (key == 114 && useCamera){//r

					tracker.Reset();
					cout<<"**Reinitialize before tracking"<<endl;
				}
				else if (key == 101 && useCamera){//e

					string profileName, filePath = "../Profiles/", path;
					char overwrite;

					cout<<"Enter profile name : ";
					cin>>profileName;
					path = filePath + profileName;

					fstream profileExport;

					profileExport.open((path + string("/config.dat")).c_str(), ios::in);

					while(profileExport){
						
						cout << "\nProfile already exists.\nDo you want to overwrite(y/n) : ";
						cin >> overwrite;

						if(overwrite == 'y')
							break;
					
						cout<<"Enter new profile name : ";
						cin>>profileName;
						path = filePath + profileName;						
						
						profileExport.close();
						profileExport.open((path + string("/config.dat")).c_str(), ios::in);						
					}

					profileExport.close();
					if(overwrite != 'y')
						if(system((string("mkdir -p ") + path).c_str()) == -1)
							return EXIT_FAILURE;
					
					conf.write(profileName.c_str());
					tracker.write(profileName.c_str());
				}
				else if (key == 108 && useCamera){ //l

					string profileName, filePath = "../Profiles/", path;
					char overwrite;

					cout<<"Enter profile name : ";
					cin>>profileName;
					path = filePath + profileName;

					fstream profileExport;

					profileExport.open((path + string("/config.dat")).c_str(), ios::in);

					if(!profileExport){
						
						cout << "\nProfile does not exists."<<endl;							
					}
					else{
						conf.read(profileName.c_str());

						tracker.Reset();

						tracker.read(profileName.c_str());

						doInitialise = true;
					}	
				}
				else if(key == 65432 && doInitialise){
					conf.liveBoxWidth += 10;
				}
				else if(key == 65433 && doInitialise){
					conf.liveBoxWidth -= 10;
				}
				else if(key == 65431 && doInitialise){
					conf.liveBoxHeight += 10;
				}
				else if(key == 65433 && doInitialise){
					conf.liveBoxHeight -= 10;
				}
				else if(key == 65361 && doInitialise){
					offsetx -= 10;
				}
				else if(key == 65363 && doInitialise){
					offsetx += 10;
				}
				else if(key == 65362 && doInitialise){
					offsety -= 10;
				}
				else if(key == 65364 && doInitialise){
					offsety += 10;
				}
			}
			if (conf.debugMode && frameInd == endFrame)
			{
				cout << "\n\nend of sequence, press any key to exit" << endl;
				waitKey();
			}
		}
	}
	
	if (outFile.is_open())
	{
		outFile.close();
	}
	
	return EXIT_SUCCESS;
}
