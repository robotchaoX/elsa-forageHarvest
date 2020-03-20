//
// Created by elsa on 2020/3/19.
//

#include "Cam.h"

int Cam::cameraStart() {

    // Set configuration parameters
    init_parameters.camera_resolution = RESOLUTION_HD720; // Use HD720 video mode
    init_parameters.camera_fps = 15; // Set fps at 15
    init_parameters.depth_mode = DEPTH_MODE_PERFORMANCE; //Use performance depth mode
    init_parameters.coordinate_units = UNIT_MILLIMETER; // Set units in millimeter
    init_parameters.coordinate_system = COORDINATE_SYSTEM_RIGHT_HANDED_Y_UP; // OpenGL's coordinate system is right_handed

    // Open the camera
    ERROR_CODE err = zed.open(init_parameters);
    if (err != ERROR_CODE::SUCCESS) {
        std::cout << "Error" << err << ", exit program.\n";
        zed.close();
        return EXIT_FAILURE;
    }

    // Set runtime parameters after opening the camera
    runtime_parameters.sensing_mode = SENSING_MODE_STANDARD; // Use STANDARD sensing mode

    // A new image is available if grab() returns ERROR_CODE::SUCCESS
    // TODO 这里一直运行吗？还是到main()里面一直运行
    if (zed.grab(runtime_parameters) == ERROR_CODE::SUCCESS) {
        // Retrieve left image
        zed.retrieveImage(color, VIEW_LEFT);
        // Retrieve depth map. Depth is aligned on the left image
        zed.retrieveMeasure(depth, MEASURE_DEPTH);
        // Retrieve colored point cloud. Point cloud is aligned on the left image.
        zed.retrieveMeasure(pointCloud, MEASURE_XYZRGBA);
    }

}

cv::Mat Cam::slMat2cvMat(sl::Mat &input) {
    // Mapping between MAT_TYPE and CV_TYPE
    int cv_type = -1;
    switch (input.getDataType()) {
        case sl::MAT_TYPE_32F_C1:
            cv_type = CV_32FC1; // 5
            break;
        case sl::MAT_TYPE_32F_C2:
            cv_type = CV_32FC2; // 13
            break;
        case sl::MAT_TYPE_32F_C3:
            cv_type = CV_32FC3; // 21
            break;
        case sl::MAT_TYPE_32F_C4:
            cv_type = CV_32FC4; // 29
            break;
        case sl::MAT_TYPE_8U_C1:
            cv_type = CV_8UC1; // 0
            break;
        case sl::MAT_TYPE_8U_C2:
            cv_type = CV_8UC2; // 8
            break;
        case sl::MAT_TYPE_8U_C3:
            cv_type = CV_8UC3; // 16
            break;
        case sl::MAT_TYPE_8U_C4:
            cv_type = CV_8UC4; // 24
            break;
        default:
            break;
    }
    // Since cv::Mat data requires a uchar* pointer, we get the uchar1 pointer from sl::Mat (getPtr<T>())
    // * cv::Mat and sl::Mat will share a single memory structure
    // getPtr() // Returns the CPU or GPU data pointer
    return cv::Mat(input.getHeight(), input.getWidth(), cv_type, input.getPtr<sl::uchar1>(sl::MEM_CPU));
}

cv::Mat Cam::getImage(int key) {
    if(key == 0)
        return slMat2cvMat(color);
    else if(key == 1)
        return slMat2cvMat(depth);
    else
        return slMat2cvMat(pointCloud);
}

void Cam::getDistance(vector<Point2f> point, vector<float> &pointDistance) {
    for (int i = 0; i < point.size(); ++i) {
        pointCloud.getValue(point[i].x, point[i].y, &point_cloud_value);
        float distance = sqrt(point_cloud_value.x * point_cloud_value.x + point_cloud_value.y * point_cloud_value.y + point_cloud_value.z * point_cloud_value.z);
        pointDistance.push_back(distance);
    }
}