#include "SelfLimitingPolygonTree.h"
#include "GridTree.h"
#include "ReptileTree.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
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

    SelfLimitingPolygonTree defaultTree;
    qtree *pTree = nullptr;

    int minNodesProcessedPerFrame = 1;
    int maxNodesProcessedPerFrame = 64;

    int presetIndex = 0;
    float imagePadding = 0.1f;

    // current model
    double modelTime;
    // current model run stats
    int totalNodesProcessed;
    std::chrono::steady_clock::time_point startTime;    // wall-clock time
    double lastReportTime;                              // wall-clock time

    // commands
    bool restart = true;
    bool randomize = true;
    bool quit = false;

    int mostRecentFileIndex = 0;


    void run()
    {

        while (1)
        {
            if (restart)
            {
                cout << "Starting run.\n";

                if (!pTree)
                {
                    pTree = &defaultTree;
                }

                if (randomize)
                {
                    pTree->setRandomSeed(++presetIndex);
                    randomize = false;
                }
                pTree->create();

                json settingsJson;
                pTree->to_json(settingsJson);
                cout << settingsJson << endl;

                canvas.image = cv::Mat3b(1200, 1200);
                canvas.image = 0;
                canvas.setScaleToFit(pTree->getBoundingRect(), imagePadding);

                // update display
                imshow("Memtest", canvas.image); // Show our image inside it.

                startTime = std::chrono::steady_clock::now();
                lastReportTime = 0;

                modelTime = 0;
                totalNodesProcessed = 0;

                restart = false;
            }

            int key = -1;

            while (key<0 && !pTree->nodeQueue.empty())
            {
                int nodesProcessed = 0;
                while (!pTree->nodeQueue.empty()
                    && nodesProcessed < maxNodesProcessedPerFrame
                    //&& pTree->nodeQueue.top().det() >= cutoff 
                    && (nodesProcessed < minNodesProcessedPerFrame || pTree->nodeQueue.top().beginTime <= modelTime)
                    )
                {
                    auto currentNode = pTree->nodeQueue.top();
                    if (!pTree->isViable(currentNode))
                    {
                        pTree->nodeQueue.pop();
                        continue;
                    }

                    pTree->drawNode(canvas, currentNode);

                    nodesProcessed++;
                    pTree->process();
                    modelTime = currentNode.beginTime + 1.0;
                }

                // adjust scaling to fit.. can't do this yet b/c tree can't be redrawn
                //canvas.setScaleToFit(pTree->getBoundingRect());

                // fade
                //canvas.image *= 0.99;

                // update display
                imshow("Memtest", canvas.image); // Show our image inside it.

                totalNodesProcessed += nodesProcessed;

                std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
                std::chrono::duration<double> lapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t - startTime);
                if (lapsed.count() - lastReportTime > 1.0)
                {
                    showReport();
                }

                key = cv::waitKey(1);
            }   // while(no key pressed and not complete)

            processKey(key);

            if (quit) return;

            if (restart) continue;

            if (pTree->nodeQueue.empty())
            {
                showReport();
                cout << "Run complete.\n";

                cout << "'s' to save, 'r' to randomize, +/- to change radius, ESC to quit.\n";
                int key = cv::waitKey(0);
                processKey(key);
            }

        }   // while(1)
    }

    void showReport()
    {
        std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
        std::chrono::duration<double> lapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t - startTime);
        lastReportTime = lapsed.count();
        cout << std::setw(8) << lapsed.count() << ": " << totalNodesProcessed << " nodes processed (" << ((double)totalNodesProcessed) / lapsed.count() << "/s)" << endl;
    }

    //  finds most recent file and sets {mostRecentFileIndex}.
    //  mostRecentFileIndex will be set to 0 if no saved files are found
    void findMostRecentFile()
    {
        mostRecentFileIndex = 0;

        fs::path jsonPath;
        char filename[40];
        for (int i=1; i <= 999; ++i)
        {
            sprintf_s(filename, "tree%04d.settings.json", i);
            if (fs::exists(filename))
                mostRecentFileIndex = i;

            sprintf_s(filename, "tree%04d.png", i);
            if (fs::exists(filename))
                mostRecentFileIndex = i;
        }
    }

    void processKey(int key)
    {
        switch (key)
        {
        case -1:
            return; // no key pressed

        case 's':
        {
            if (mostRecentFileIndex == 0)
            {
                findMostRecentFile();
            }

            ++mostRecentFileIndex;
            fs::path imagePath;
            char filename[24];
            sprintf_s(filename, "tree%04d.png", mostRecentFileIndex);
            imagePath = filename;

            cv::imwrite(imagePath.string(), canvas.image);

            // allow extending classes to customize the save
            pTree->saveImage(imagePath);

            // save the settings too
            std::ofstream outfile(imagePath.replace_extension("settings.json"));
            json j;
            pTree->to_json(j);
            outfile << std::setw(4) << j;

            std::cout << "Image saved: " << imagePath << std::endl;

            return;
        }

        case 'o':
        {
            if (mostRecentFileIndex == 0)
            {
                findMostRecentFile();
            }

            int idx = -1;
            cout << "The most recent file index is " << mostRecentFileIndex << ".\nEnter index: ";
            std::cin >> idx;
            if (idx < 0)
                idx = mostRecentFileIndex;

            char filename[50];
            sprintf_s(filename, "tree%04d.settings.json", idx);
            cout << "Opening " << filename << "...\n";

            try {
                std::ifstream infile(filename);
                json j;
                infile >> j;
                pTree = qtree::createTreeFromJson(j);

                std::cout << "Settings read from: " << filename << std::endl;

                restart = true;
                randomize = false;

                return;
            }
            catch (std::exception &ex)
            {
                std::cout << "Failed to open " << filename << ":\n" << ex.what() << endl;
            }
            break;
        }

        case 'c':
        {
            pTree = pTree->clone();
            restart = true;
            cout << "Cloned\n";
            return;
        }

        case 'r':
        {
            // restart with new random settings
            maxNodesProcessedPerFrame = 64;
            restart = true;
            randomize = true;
            return;
        }

        case '+':
            pTree->maxRadius *= 2.0f;
            restart = true;
            break;

        case '-':
            pTree->maxRadius *= 0.5f;
            restart = true;
            break;

        case '.':
        {
            maxNodesProcessedPerFrame = 1;
            break;
        }

        case ' ':
        {
            // restart with current settings
            maxNodesProcessedPerFrame = 64;
            restart = true;
            return;
        }

        case 27:    // ESC
        case 'q':
            quit = true;
            return;

        }
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