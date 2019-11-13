#include "SelfAvoidantPolygonTree.h"
#include "GridTree.h"
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

    ReptileTree tree1;
    GridTree tree2;
    SelfAvoidantPolygonTree tree3;
    ScaledPolygonTree tree4;
    TrapezoidTree tree5;
    ThornTree tree;

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

                if (randomize)
                {
                    tree.setRandomSeed(++presetIndex);
                    randomize = false;
                }
                tree.create();

                json settingsJson = tree.getSettings();
                cout << settingsJson << endl;

                canvas.image = cv::Mat3b(1200, 1200);
                canvas.image = 0;
                canvas.setScaleToFit(tree.getBoundingRect(), imagePadding);

                // update display
                imshow("Memtest", canvas.image); // Show our image inside it.

                startTime = std::chrono::steady_clock::now();
                lastReportTime = 0;

                modelTime = 0;
                totalNodesProcessed = 0;

                restart = false;
            }

            int key = -1;

            while (key<0 && !tree.nodeQueue.empty())
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
                    showReport();
                }

                key = cv::waitKey(1);
            }   // while(no key pressed and not complete)

            processKey(key);

            if (quit) return;

            if (restart) continue;

            if (tree.nodeQueue.empty())
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

    bool processKey(int key)
    {
        switch (key)
        {
        case -1:
            return true; // no key pressed

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
            tree.saveImage(imagePath);

            // save the settings too
            std::ofstream outfile(imagePath.replace_extension("settings.json"));
            outfile << std::setw(4) << tree.getSettings();

            std::cout << "Image saved: " << imagePath << std::endl;

            return true;
        }

        case 'o':
        {
            if (mostRecentFileIndex == 0)
            {
                findMostRecentFile();
            }

            char filename[50];
            sprintf_s(filename, "tree%04d.settings.json", mostRecentFileIndex);

            try {
                std::ifstream infile(filename);
                json j;
                infile >> j;

                tree.settingsFromJson(j);

                std::cout << "Settings read from: " << filename << std::endl;

                restart = true;
                randomize = false;

                return true;
            }
            catch (std::exception &ex)
            {
                std::cout << "Failed to open " << filename << ":\n" << ex.what() << endl;
            }
        }

        case 'r':
        {
            // restart with new random settings
            maxNodesProcessedPerFrame = 64;
            restart = true;
            randomize = true;
            return true;
        }

        case '+':
            tree.maxRadius *= 2.0f;
            restart = true;
            break;

        case '-':
            tree.maxRadius *= 0.5f;
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
            return true;
        }

        case 27:    // ESC
        default:
            quit = true;
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