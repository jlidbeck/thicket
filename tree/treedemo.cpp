#include "HexGridTree.h"
#include "GridTree.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <filesystem>
#include <random>
#include <vector>
#include <chrono>


namespace fs = std::filesystem;


using std::cout;
using std::endl;


class TreeDemo
{
public:
    cv::Mat loadedImage;
    qcanvas canvas;

    ReptileTree tree1;
    GridTree tree2;
    SelfAvoidantPolygonTree tree3;
    ScaledPolygonTree tree4;
    TrapezoidTree tree5;

    qtree &tree = tree5;

    int minNodesProcessedPerFrame = 1;
    int maxNodesProcessedPerFrame = 64;

    int presetIndex = 4;
    float imagePadding = 0.1f;

    void run()
    {
        tree.randomizeSettings(presetIndex);
        tree.create();

        cv::Mat3b img(cv::Size(1200, 1200));
        img = 0;

        canvas.create(img);
        canvas.setScaleToFit(tree.getBoundingRect(), imagePadding);

        //qtransform  t1( 30, halfRoot(6), cv::Point2f(0, 90)),
        //            t2(-30, halfRoot(6), cv::Point2f(0, 60));

        //qtransform  t1( 90, halfRoot(2), cv::Point2f(0,  0.9)),
        //            t2(-90, halfRoot(2), cv::Point2f(0, -0.9));

        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> dis(0, canvas.image.cols - 50);
        //cv::Rect rect(0, 0, 50, 50);

        while (1)
        {
            cout << "Starting run.\n";

            std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
            double lastReportTime = 0;

            double modelTime = 0;
            int totalNodesProcessed = 0;

            while (!tree.nodeQueue.empty())
            {
                int nodesProcessed = 0;
                while (!tree.nodeQueue.empty()
                    && nodesProcessed < maxNodesProcessedPerFrame
                    //&& tree.nodeQueue.top().det() >= cutoff 
                    && (nodesProcessed < minNodesProcessedPerFrame || tree.nodeQueue.top().beginTime <= modelTime)
                    )
                {
                    auto currentNode = tree.nodeQueue.top();
                    if (!tree.isViable(currentNode))
                    {
                        tree.nodeQueue.pop();
                        continue;
                    }

                    tree.drawNode(canvas, currentNode);

                    nodesProcessed++;
                    tree.process();
                    modelTime = currentNode.beginTime + 1.0;
                }

                // adjust scaling to fit.. can't do this yet b/c tree can't be redrawn
                //canvas.setScaleToFit(tree.getBoundingRect());

                // fade
                //canvas.image *= 0.99;

                // update display
                imshow("Memtest", canvas.image); // Show our image inside it.

                totalNodesProcessed += nodesProcessed;

                std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
                std::chrono::duration<double> lapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t - startTime);
                if (lapsed.count() - lastReportTime > 1.0)
                {
                    lastReportTime = lapsed.count();
                    cout << std::setw(8) << lapsed.count() << ": " << totalNodesProcessed << " nodes processed (" << ((double)totalNodesProcessed) / lapsed.count() << "/s)" << endl;
                }

                int key = cv::waitKey(1);
                if (!processKey(key))
                    return;
            }

            std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
            std::chrono::duration<double> lapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t - startTime);
            cout << std::setw(8) << lapsed.count() << ": " << totalNodesProcessed << " nodes processed (" << ((double)totalNodesProcessed) / lapsed.count() << "/s)" << endl;
            cout << "Run complete. 's' to save, 'r' to randomize, ESC to quit.\n";
            int key = cv::waitKey(0);
            if (!processKey(key))
                return;

        }
    }

    bool processKey(int key)
    {
        switch (key)
        {
        case -1:
            return true; // no key pressed

        case 's':
        {
            fs::path imagePath = "temp0001.png";
            char filename[24];
            for (int count = 1; fs::exists(imagePath); ++count)
            {
                sprintf_s(filename, "temp%04d.png", count);
                imagePath = filename;
            }

            cv::imwrite(imagePath.string(), canvas.image);
            std::cout << "Image saved: " << imagePath << std::endl;
            return true;
        }

        case 'r':
        {
            canvas.image = 0;
            presetIndex = (++presetIndex);
            tree.randomizeSettings(presetIndex);
            tree.create();
            canvas.setScaleToFit(tree.getBoundingRect(), imagePadding);
            return true;
        }

        case '.':
        {
            maxNodesProcessedPerFrame = 1;
            break;
        }

        case ' ':
        {
            // restart with current settings
            maxNodesProcessedPerFrame = 64;
            canvas.image = 0;
            //presetIndex = (presetIndex + ReptileTree::NUM_PRESETS) % (ReptileTree::NUM_PRESETS * 2);
            tree.create();
            canvas.setScaleToFit(tree.getBoundingRect(), imagePadding);
            return true;
        }

        case 27:    // ESC
        default:
            return false;
        }

        return false;
    }

    int load(fs::path imagePath)
    {
        imagePath = fs::absolute(imagePath);
        loadedImage = cv::imread(imagePath.string(), cv::ImreadModes::IMREAD_COLOR); // Read the file
        if (!loadedImage.data) // Check for invalid input
        {
            std::cout << "Could not open or find the image " << imagePath << std::endl;
            return -1;
        }

        //canvas.image = loadedImage;

        return 0;
    }

} the;


static void onMouse(int event, int x, int y, int, void*)
{
    if (event != cv::MouseEventTypes::EVENT_LBUTTONDOWN)
        return;
    cv::Point seed = cv::Point(x, y);

    the.canvas.image = the.loadedImage;
}


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << " Usage: display_image ImageToLoadAndDisplay" << std::endl;
        return -1;
    }


    cv::namedWindow("Memtest", cv::WindowFlags::WINDOW_AUTOSIZE); // Create a window for display.

    cv::setMouseCallback("image", onMouse, 0);


    //try {
        //the.load(argv[1]);
    the.run();
    //}
    //catch (cv::Exception ex)
    //{
    //    cerr << "cv exception: " << ex.what();
    //    return -1;
    //}

    return 0;
}