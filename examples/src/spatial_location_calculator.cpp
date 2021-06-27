#include <iostream>
#include <chrono>

#include "utility.hpp"

// Inludes common necessary includes for development using depthai library
#include "depthai/depthai.hpp"

int main() {
    using namespace std;
    using namespace std::chrono;

    // Create pipeline
    dai::Pipeline pipeline;

    // Define sources and outputs
    auto monoLeft = pipeline.create<dai::node::MonoCamera>();
    auto spatialDataCalculator = pipeline.create<dai::node::SpatialLocationCalculator>();

    auto xoutDepth = pipeline.create<dai::node::XLinkOut>();
    auto xoutSpatialData = pipeline.create<dai::node::XLinkOut>();

    xoutDepth->setStreamName("depth");
    xoutSpatialData->setStreamName("spatialData");

    // Properties
    monoLeft->setResolution(dai::MonoCameraProperties::SensorResolution::THE_400_P);
    monoLeft->setBoardSocket(dai::CameraBoardSocket::LEFT);

    // Config
    dai::Point2f topLeft(0.4f, 0.4f);
    dai::Point2f bottomRight(0.6f, 0.6f);

    dai::SpatialLocationCalculatorConfigData config;
    config.depthThresholds.lowerThreshold = 100;
    config.depthThresholds.upperThreshold = 10000;
    config.roi = dai::Rect(topLeft, bottomRight);

    spatialDataCalculator->setWaitForConfigInput(false);
    spatialDataCalculator->initialConfig.addROI(config);

    // Linking
    spatialDataCalculator->passthroughDepth.link(xoutDepth->input);
    monoLeft->out.link(spatialDataCalculator->inputDepth);

    spatialDataCalculator->out.link(xoutSpatialData->input);

    // Connect to device and start pipeline
    dai::Device device(pipeline);

    // Output queue will be used to get the depth frames from the outputs defined above
    auto depthQueue = device.getOutputQueue("depth", 8, false);
    auto spatialCalcQueue = device.getOutputQueue("spatialData", 8, false);

    auto color = cv::Scalar(0, 255, 0);

    auto startTime = steady_clock::now();
    int counter = 0;
    float fps = 0;

    while(true) {
        auto inDepth = depthQueue->get<dai::ImgFrame>();

        counter++;
        auto currentTime = steady_clock::now();
        auto elapsed = duration_cast<duration<float>>(currentTime - startTime);
        if(elapsed > seconds(1)) {
            fps = counter / elapsed.count();
            counter = 0;
            startTime = currentTime;
        }


        cv::Mat depthFrame = inDepth->getCvFrame();

        auto spatialData = spatialCalcQueue->get<dai::SpatialLocationCalculatorData>()->getSpatialLocations();
        for(auto depthData : spatialData) {
            // cout << "id: " << depthData.id << endl;

            // cout << "x: " << depthData.c.x << " y: " << depthData.c.y << endl;

            // cout << "x: " << depthData.p.x << "y: " << depthData.p.y << "width: " << depthData.p.width << "height: " << depthData.p.height << endl;

            auto xmin = (int)depthData.p.x;
            auto ymin = (int)depthData.p.y;
            auto xmax = xmin + (int)depthData.p.width;
            auto ymax = ymin + (int)depthData.p.height;

            std::stringstream idStr;
            idStr << "ID: " << depthData.id;
            cv::putText(depthFrame, idStr.str(), cv::Point(xmin + 10, ymin + 35), cv::FONT_HERSHEY_TRIPLEX, 0.5, color);

            cv::rectangle(depthFrame, cv::Rect(cv::Point(xmin, ymin), cv::Point(xmax, ymax)), color, cv::FONT_HERSHEY_SIMPLEX);

            // for(int i = 0; i < 4; i++) {
            //     std::cout << "x: " << depthData.p[i][0] << " y: " << depthData.p[i][1] << std::endl;
            // }
        }
        // Show the frame

        std::stringstream fpsStr;
        fpsStr << "NN fps:" << std::fixed << std::setprecision(2) << fps;
        cv::putText(depthFrame, fpsStr.str(), cv::Point(2, inDepth->getHeight() - 4), cv::FONT_HERSHEY_TRIPLEX, 0.4, color);

        cv::imshow("depth", depthFrame);

        int key = cv::waitKey(1);
        if(key == 'q') {
            return 0;
        }
    }
    return 0;
}
