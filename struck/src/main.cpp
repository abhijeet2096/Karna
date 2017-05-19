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

#include "PracticalSocket.h" // For UDPSocket and SocketException
#include <cstdlib>           // For atoi()
#define BUF_LEN 65540 // Larger than maximum UDP packet size
#define FRAME_HEIGHT 720
#define FRAME_WIDTH 1280
#define FRAME_INTERVAL (1000/30)
#define PACK_SIZE 4096 //udp pack size; note that OSX limits < 8100 bytes
#define ENCODE_QUALITY 80

#include <Eigen/Core>
#include <opencv/highgui.h>

using namespace std;
using namespace cv;
using namespace Eigen;



Mat takeFrame(){
	c:

	unsigned short servPort = 5003;
	char buffer[BUF_LEN]; // Buffer for echo string
	int recvMsgSize; // Size of received message
	string sourceAddress; // Address of datagram source
	unsigned short sourcePort; // Port of datagram source
	  
	UDPSocket sock(servPort);
	clock_t last_cycle = clock(); 
			
	try{
 		
		do{
		    recvMsgSize = sock.recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort);
		} while (recvMsgSize > sizeof(int));
		        
		int total_pack = ((int * ) buffer)[0];

	    char * longbuf = new char[PACK_SIZE * total_pack];
	    for (int i = 0; i < total_pack; i++) {
	    
	    	recvMsgSize = sock.recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort);
		    if (recvMsgSize != PACK_SIZE) 
		    {
		        continue;
		    }
		   
		    memcpy( & longbuf[i * PACK_SIZE], buffer, PACK_SIZE);
	    }

	    Mat rawData = Mat(1, PACK_SIZE * total_pack, CV_8UC1, longbuf);
	    
	    Mat frame = imdecode(rawData, CV_LOAD_IMAGE_COLOR);
	   
	    if(!frame.data)
	    {
	    	goto c;
	    }
	    if (frame.size().width == 0) {
	    
	      	goto c;
	    }
	    free(longbuf);
		waitKey(1);
		clock_t next_cycle = clock();
		double duration = (next_cycle - last_cycle) / (double) CLOCKS_PER_SEC;
		last_cycle = next_cycle;

	    return frame;  
	}
	catch (SocketException & e) {
     	goto c;
    }
}

int centerX, centerY;

void moveQuadcopter(const FloatRect& bb, int width, int height){

	int range = width > height ? width : height;

	if((bb.XMax() < centerX + range) &&
	 (bb.XMin() > centerX - range) && 
	 (bb.YMin() > centerY - range) && 
	 (bb.YMax() < centerY + range)){
		cout<<"Hover"<<endl;
	}
	
	if((bb.XMax() > centerX + range) ||
	 (bb.XMin() < centerX - range)){
		
		if(bb.XMax() > centerX + range){
			cout<<"rotate Left"<<endl;
		}
		else{
			cout<<"rotate Right"<<endl;
		}
	}

	if((bb.YMin() < centerY - range) || 
	 (bb.YMax() > centerY + range)){
		if(bb.YMin() < centerY - range){
			cout<<"move Forward"<<endl;
		}
		else{
			cout<<"move Backward"<<endl;	
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
	
	// if (!cap.open())
	// {
	// 	cout << "error: could not start camera capture" << endl;
	// 	return EXIT_FAILURE;
	// }
	startFrame = 0;
	endFrame = INT_MAX;
	Mat tmp;
	tmp = takeFrame();
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
		frameOrig = takeFrame();
		resize(frameOrig, frame, Size(conf.frameWidth, conf.frameHeight));
		//flip(frame, frame, 1);
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
				moveQuadcopter(tracker.GetBB(), conf.liveBoxWidth, conf.liveBoxHeight);
			else{
				cout<<"Hover"<<endl;
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

					centerX = tracker.GetBB().XMin() + conf.liveBoxWidth/2;
					centerY = tracker.GetBB().YMin() + conf.liveBoxHeight/2;
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
				else if(key == 65430 && doInitialise){
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
