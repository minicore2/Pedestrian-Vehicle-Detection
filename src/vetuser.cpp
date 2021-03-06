/**
* [
*  Copyright (c) 2016 by Vehicle-Eye Technology
*  ALL RIGHTS RESERVED.
*
*  The software and information contained herein are proprietary to,  and comprise
*  valuable trade secrets of, Vehicle-Eye Technology.  Disclosure of the software
*  and information will materially harm the business of Vehicle-Eye Technology.
*  This software is furnished pursuant to a written development agreement and may
*  be used, copied, transmitted, and stored only in accordance with the terms of
*  such agreement and with the inclusion of the above copyright notice.  This
*  software and information or any other copies thereof may not be provided or
*  otherwise made available to any other party.
* ]
*/


/*!
* \file vetuser.cpp
* \author [Zeyu Zhang]
* \version [0.1]
* \date 2017-05-23
*/

#include "vetuser.h"

using namespace std;
using namespace cv;

VetUser::VetUser()
{
	cout << "[VetUser::VetUser]: VetUser created" << endl;
}

VetUser::~VetUser()
{
	cout << "[VetUser::~VetUser]: VetUser deleted" << endl;
}

// 用户模式：运行完整的功能（行人检测,车辆检测,光流检测）
void VetUser::start(string video_path)
{
	cout << "[VetUser::start]: user mode starts" << endl;

	VetFastVideoCapture fvs(video_path, 128);

	if( !fvs.isOpened() )
		error(string("Cannot open video:") + video_path);
	fvs.start();

	VetDetectorFactory detector_factory;

	// create detector
	VetOptFlowDetector optFlowDetector;
	VetDetectorStrategy *human_detector = detector_factory.createDetector(CASCADE_DETECTOR, FULLBODY);
	VetDetectorStrategy *front_car_detector = detector_factory.createDetector(HAAR_DETECTOR, FRONT_CAR);
	VetDetectorStrategy *rear_car_detector = detector_factory.createDetector(HAAR_DETECTOR, REAR_CAR);

	// create detected object tracker
	VetTracker human_tracker(0.8, 2, 1, 2);
	VetTracker car_tracker(0.8, 2, 1, 2);

	Mat frame;
	vector<VetROI> rois_car, rois_opt, rois_human, temp_rois;

	bool enable_car_detector = ENABLE_VEHICLE;
	bool enable_people_detector = ENABLE_PEDESTRIAN;
	bool enable_optflow = ENABLE_OPTFLOW;
	
	int cnt = 0;

	namedWindow("frame");
	moveWindow("frame", 25, 25);

	while( fvs.more() )
	{
		if ( fvs.read(frame) )
		{
			if(cnt++ % 2 == 1)
				continue;

			// optical flow enabled
			// 光流检测，检测车旁超车物体
			if( enable_optflow )
			{
				optFlowDetector.detect(frame, rois_opt);
				
				drawRectangles(frame, rois_opt, COLOR_RED);

				if(ENABLE_DEBUG)
				{
					printVetROI(rois_opt);
				}
				
				rois_opt.clear();
			}

			// car detector enabled
			// 车辆检测
			if( enable_car_detector )
			{
				front_car_detector->detect(frame, temp_rois);
				rois_car.insert(rois_car.end(), temp_rois.begin(), temp_rois.end());

				rear_car_detector->detect(frame, temp_rois);
				rois_car.insert(rois_car.end(), temp_rois.begin(), temp_rois.end());
				
				NMS(rois_car, 0.3);
				car_tracker.update(rois_car);
				drawRectangles(frame, rois_car, COLOR_GREEN);

				if(ENABLE_DEBUG)
				{
					printVetROI(rois_car);
				}

				rois_car.clear();
			}
			
			// pedestrain detector enabled
			// 行人检测
			if( enable_people_detector )
			{
				human_detector->detect(frame, rois_human);

				NMS(rois_human, 0.3);
				human_tracker.update(rois_human);
				drawRectangles(frame, rois_human, COLOR_BLUE);

				if(ENABLE_DEBUG)
				{
					printVetROI(rois_human);
				}
				
				rois_human.clear();
			}

			imshow("frame", frame);
		}

		// 按键控制
		char resp = waitKey(30);

		// 退出
		if(resp == KEY_ESC)
		{
			cout << "window: frame closed" << endl;
			destroyWindow("frame");
			break;
		}
		// 暂停
		else if(resp == KEY_SPACE)
		{
			cout << "window: frame paused" << endl;
			cout << "Press any key to continue..." << endl;
			waitKey(-1);
		}
		// 开启/关闭车辆检测
		else if(resp == 'c')
		{
			enable_car_detector = !enable_car_detector;
			printf("%s car detector.\n", (enable_car_detector ? "enable" : "disable"));
		}
		// 开启/关闭行人检测
		else if(resp == 'p')
		{
			enable_people_detector = !enable_people_detector;
			printf("%s people detector.\n", (enable_people_detector ? "enable" : "disable"));
		}
		// 开启/关闭光流检测
		else if(resp == 'o')
		{
			enable_optflow = !enable_optflow;
			printf("%s optical flow detector.\n", (enable_optflow ? "enable" : "disable"));
		}
	}

	fvs.stop();

	delete human_detector;
	delete rear_car_detector;
	delete front_car_detector;

	cout << "[VetUser::start]: user mode ends" << endl;
}