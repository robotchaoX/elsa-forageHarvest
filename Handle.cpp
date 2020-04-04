//
// Created by elsa on 2020/3/20.
//

#include "Handle.h"

int maxHeight = 500; //落点最大高度


Handle::Handle(Mat &src) {
    image = src;
    armL = image.cols / 2 - image.cols / 16; //左边界 //TODO 参数16
    armR = image.cols / 2 + image.cols / 16; //右边界
}

void Handle::setKeyPoints(vector<Point2f> pt2D, vector<vector<Point3f>> pt3D) {
    keyPoints2D = pt2D;
    keyPoints3D = pt3D;
    for (int i = 4; i < pt2D.size(); ++i) {
        fallPoints2D.push_back(pt2D[i]);
    }
    for (int i = 4; i < pt3D.size(); ++i) {
        point3D pt(pt3D[i]);
        fallPoints3D.push_back(pt);
    }
}

void Handle::process() {

}

void Handle::getPlane() {
    Point3f p1 = keyPoints3D[0][20]; //角点还是使用原点
    Point3f p2 = keyPoints3D[1][20];
    Point3f p3 = keyPoints3D[2][20];

    a = (p2.y - p1.y) * (p3.z - p1.z) - (p2.z - p1.z) * (p3.y - p1.y);
    b = (p2.z - p1.z) * (p3.x - p1.x) - (p2.x - p1.x) * (p3.z - p1.z);
    c = (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
    d = 0 - (a * p1.x + b * p1.y + c * p1.z);

}

void Handle::fallPointOverflow() {

    float distanceSum = 0;
    float distanceAvg = 0;
    for (int i = 0; i < fallPoints3D.size(); ++i) {
        for (int j = 0; j < fallPoints3D[i].pt.size(); ++j) {
            distanceSum += (a * fallPoints3D[i].pt[j].x + b * fallPoints3D[i].pt[j].y + c * fallPoints3D[i].pt[j].z + d) / sqrt(a * a + b * b + c * c);
        }
        distanceAvg = distanceSum / fallPoints3D[i].pt.size(); //取平均值
        fallPoints3D[i].setDistance(distanceAvg);
    }
}


void Handle::drawFallPoints() {

    for (int i = 0; i < fallPoints3D.size(); ++i) {
        float height = fallPoints3D[i].distance;
        if (height > maxHeight)
            circle(image, fallPoints2D[i], 2, cv::Scalar(0, 0, 255), 2);
        else if (height < maxHeight / 2)
            circle(image, fallPoints2D[i], 2, cv::Scalar(0, 255, 0), 2);
        else
            circle(image, fallPoints2D[i], 2, cv::Scalar(0, 255, 255), 2);
    }
}

void Handle::right2Left() {
    for (int i = 0; i < fallPoints3D.size(); ++i) {
        if(fallPoints2D[i].x >= armL && fallPoints2D[i].x <= armR){
            if(fallPoints3D[i].distance < maxHeight){
                //不转
                //无箭头
                //break? 怎么让他停在这里喷洒 不到下一个落点
            } else{
                //左转
                //左箭头
                //下一个落点
            }
        }
        else if(fallPoints2D[i].x > armR){
            if(fallPoints3D[i].distance < maxHeight){
                //右转
                //右箭头
                //转到最右未满的点
            } else{
                //左转
                //左箭头
                //下一个落点
            }
        }
        else{ //落点位于下落柱左边 逻辑好像有问题
            if(fallPoints3D[i].distance < maxHeight){
                //左转
                //左箭头
                //转到当前落点
            } else{
                //左转
                //左箭头
                //下一个落点
            }
        }
    }
}