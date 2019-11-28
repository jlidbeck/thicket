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
using std::cin;
using std::endl;


cv::Size renderSizePreview  = cv::Size(200, 200);
cv::Size renderSizeHD       = cv::Size(2000, 1500);


class TreeDemo
{
public:
    cv::Mat loadedImage;
    qcanvas canvas;

    cv::Size renderSize = renderSizePreview;

    ThornTree defaultTree;
    qtree *pTree = nullptr;

    int minNodesProcessedPerFrame = 1;
    int maxNodesProcessedPerFrame = 64;
    bool stepping = false;

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

	// current file pointer. should usually point to existing file "tree%04d"
    int currentFileIndex = -1;


    void run()
    {

        while (1)
        {
            if (restart)
            {
				cout << "--- Starting run --- " << endl;
				
				if (!pTree)
                {
                    pTree = &defaultTree;
					cout << "--- Set default tree\n";
                }

                if (randomize)
                {
                    pTree->setRandomSeed(++presetIndex);
                    //pTree->name = std::string("R") + std::to_string(presetIndex);
                    randomize = false;
					cout << "--- Randomized ["<<presetIndex<<+"]\n";
				}
                pTree->create();
                pTree->transformCounts.clear();

                //json settingsJson;
                //pTree->to_json(settingsJson);
                //cout << settingsJson << endl;

				cout << "--- Starting run: " << pTree->name << ": " << pTree->transforms.size() << " transforms" << endl;

                canvas.image = cv::Mat3b(renderSize);
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

            while (!pTree->nodeQueue.empty())
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

                if (stepping) 
                {
                    key = cv::waitKey(0);
                    break;
                }

                key = cv::waitKey(1);
                if (key > 0) break;
            }   // while(no key pressed and not complete)

            processKey(key);

            if (quit) return;

            if (restart) continue;

            if (pTree->nodeQueue.empty())
            {
                showReport();
                cout << "Run complete.\n";

                cout << "'s' save, 'o','O' open, 'c',' ' restart, '.' step, 'r' randomize, 'l' line color, +/- model radius, ESC to quit.\n";
                int key = cv::waitKey(-1);
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

    void openFile(int idx)
    {
        char filename[50];
        sprintf_s(filename, "tree%04d.settings.json", idx);
        cout << "Opening " << filename << "...\n";

        try {
            std::ifstream infile(filename);
            json j;
            infile >> j;
            pTree = qtree::createTreeFromJson(j);

            std::cout << "Settings read from: " << filename << std::endl;

            if (pTree->name.empty())
                pTree->name = std::to_string(idx);

            restart = true;
            randomize = false;
        }
        catch (std::exception &ex)
        {
            std::cout << "Failed to open " << filename << ":\n" << ex.what() << endl;
        }
    }

    const int MAX = 1000;

    //  finds most recent file and sets {currentFileIndex}.

    void findNextUnusedFileIndex()
    {
        currentFileIndex = MAX;
        findPreviousFile();
        ++currentFileIndex;
    }

    //  currentFileIndex will be set to 0 if no saved files are found
    void findPreviousFile()
    {
        if (currentFileIndex < 0)
            currentFileIndex = 0;

        fs::path jsonPath;
        char filename[40];
        for (int i=currentFileIndex+MAX-1; (i%=MAX) != currentFileIndex; i+=(MAX-1))
        {
            sprintf_s(filename, "tree%04d.settings.json", i);
            if (fs::exists(filename))
            {
                currentFileIndex = i;
                return;
            }
        }

        currentFileIndex = -1;// nothing found
    }

    //  finds most recent file and sets {currentFileIndex}.
    //  currentFileIndex will be set to 0 if no saved files are found
    void findNextFile()
    {
        if (currentFileIndex < 0)
            currentFileIndex = 0;

        fs::path jsonPath;
        char filename[40];
        for (int i = currentFileIndex+1; (i%=MAX) != currentFileIndex; i+=(1))
        {
            sprintf_s(filename, "tree%04d.settings.json", i);
            if (fs::exists(filename))
            {
                currentFileIndex = i;
                return;
            }
        }

        currentFileIndex = -1;// nothing found
    }

    void processKey(int key)
    {
        switch (key)
        {
        case -1:
            return; // no key pressed

        case 's':
        {
            findNextUnusedFileIndex();

            fs::path imagePath;
            char filename[24];
            sprintf_s(filename, "tree%04d.png", currentFileIndex);
            imagePath = filename;

            cv::imwrite(imagePath.string(), canvas.image);

			if (pTree->name.empty())
			{
				sprintf_s(filename, "tree%04d", currentFileIndex);
				pTree->name = filename;
			}

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
            if (currentFileIndex < 0)
            {
                findPreviousFile();
            }

            int idx = -1;
            cout << "Current file index is " << currentFileIndex << ".\nEnter index: ";
			if (std::cin >> idx)
			{
				if (idx < 0)
					idx = currentFileIndex;
				openFile(idx);
			}
            break;
        }

		case 'h':
			renderSize = (renderSize == renderSizePreview ? renderSizeHD : renderSizePreview);
			restart = true;
			break;

        case 'O':
        case 0x210000:      // PageUp
            findPreviousFile();
            openFile(currentFileIndex);
            break;

        case 0x220000:      // PageDown
        {
            findNextFile();
            openFile(currentFileIndex);
            break;
        }

        case 'C':
        {
            pTree = pTree->clone();
            restart = true;
            cout << "Cloned\n";
            return;
        }

        case 'r':
        {
            // restart with new random settings
            restart = true;
            randomize = true;
            return;
        }

        case '+':
            pTree->domain = pTree->domain * 2.0f;
            restart = true;
            break;

        case '-':
            pTree->domain = pTree->domain * 0.5f;
            restart = true;
            break;

        case '0':   // recenter model in domain
            pTree->domain.x = -0.5f * pTree->domain.width;
            pTree->domain.y = -0.5f * pTree->domain.height;
            restart = true;
            break;

        case '1':
        {
            // cycle thru aspect ratios
            float a = pTree->domain.width / pTree->domain.height;
            if (a == 1.0f)
                pTree->domain.width *= 2.0f;
            else if (a == 2.0f)
                pTree->domain.width *= 2.0f;
            else if (a == 4.0f)
                pTree->domain = cv::Rect_<float>(pTree->domain.x, pTree->domain.y, pTree->domain.width / 4.0f, pTree->domain.height * 2.0f);
            else if (a == 0.5f)
                pTree->domain.height *= 2.0f;
            else if (a > 1.0f)
                pTree->domain.width = pTree->domain.height;
            else
                pTree->domain.height = pTree->domain.width;
            restart = true;
            break;
        }

        case '2':
            switch (pTree->domainShape)
            {
            case qtree::DomainShape::RECT: pTree->domainShape = qtree::DomainShape::ELLIPSE; break;
            case qtree::DomainShape::ELLIPSE: pTree->domainShape = qtree::DomainShape::RECT; break;
            }
            restart = true;
            break;
                
        case 0x250000:  // left
            pTree->domain.x += 0.10f * std::min( pTree->domain.width, pTree->domain.height);
            restart = true;
            break;

        case 0x260000:  // up
            pTree->domain.y -= 0.10f * std::min( pTree->domain.width, pTree->domain.height);
            restart = true;
            break;

        case 0x270000:  // right
            pTree->domain.x -= 0.10f * std::min( pTree->domain.width, pTree->domain.height);
            restart = true;
            break;

        case 0x280000:  // down
            pTree->domain.y += 0.10f * std::min( pTree->domain.width, pTree->domain.height);
            restart = true;
            break;

        case 'l':   // cycle thru line options
            if (pTree->lineThickness == 0)
            {
                pTree->lineThickness = 1;
                pTree->lineColor = cv::Scalar(0, 0, 0);
            }
            else if (pTree->lineColor == cv::Scalar(0, 0, 0))
            {
                pTree->lineColor = cv::Scalar(255,255,255);
            }
            else
            {
                pTree->lineThickness = 0;
            }
            restart = true;
            break;

        case 'c':   // randomize colors
            pTree->randomizeTransforms(1);
            restart = true;
            break;

        case 'p':   // randomize drawPolygon
            pTree->randomizeTransforms(4);
            restart = true;
            break;

        case '.':
        {
            stepping = true;
            maxNodesProcessedPerFrame = 1;
            if (pTree->nodeQueue.empty())
                restart = true;
            break;
        }

        case ',':
        {
            stepping = false;
            maxNodesProcessedPerFrame = 64;
            break;
        }

        case ' ':
        {
            // restart with current settings
            stepping = false;
            maxNodesProcessedPerFrame = 64;
            restart = true;
            break;
        }

        case 't':
            cout << "=== " << pTree->name << " transforms: " << endl;
            for (auto const &t : pTree->transforms)
            {
                cout << std::setw(5) << pTree->transformCounts[t.transformMatrixKey] << "   " << t.transformMatrixKey << "   " << t.colorTransform.description() << endl;
            }
            break;

        case 'T':
        {
            int idx;
            cout << "Drop transform? ";
            if (cin >> idx)
            {
                pTree->transforms.erase(pTree->transforms.begin() + idx);
            }
            break;
        }


        case 'x':
        //{
        //    int count = 0;

        //    std::vector<qnode> nodes;
        //    cv::Rect2f rc(2, 3, 10, 7);
        //    pTree->getNodesIntersecting(rc, nodes);
        //    for (auto &node : nodes)
        //    {
        //        std::vector<string> lineage;
        //        pTree->getLineage(node, lineage);
        //        cout << " x: [";
        //        for (auto it = lineage.rbegin(); it != lineage.rend(); ++it) cout << *it << " ";
        //        cout << "]\n";
        //        count += pTree->removeNode(node.id);
        //    }

        //    //for (int idx = 0; idx < 100000 && count<1; ++idx)
        //    //{
        //    //    count += pTree->removeNode(-1);
        //    //}

        //    if (count)
        //    {
        //        cout << "** remove(" << 42 << "): " << count << " nodes removed\n";
        //        pTree->redrawAll(canvas);
        //        cv::imshow("Memtest", canvas.image); // Show our image inside it.
        //        break;
        //    }
        //    break;
        //}

        //case 'X':
        {
            pTree->regrowAll();
            break;
        }

        case 27:    // ESC
        case 'q':
            quit = true;
            break;

        default:
            cout << "? " << key << endl;
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
    auto pt = the.canvas.canvasToModel(seed);
    std::vector<qnode> nodes;

    // display info on node
    the.pTree->getNodesIntersecting(cv::Rect2f(pt, cv::Size2f(0,0)), nodes);
    for (auto& node : nodes)
    {
        vector<string> lineage;
        the.pTree->getLineage(node, lineage);
        cout << "Node[" << node.id << "]:";
        for (auto& tname : lineage)
            cout << " " << tname;
        cout << endl;
    }
    return;

    // delete block
    the.pTree->getNodesIntersecting(cv::Rect2f(pt, cv::Size2f(11, 8)), nodes);
    for (auto &node : nodes)
    {
        the.pTree->removeNode(node.id);
    }
    the.pTree->redrawAll(the.canvas);
    cv::imshow("Memtest", the.canvas.image); // Show our image inside it.

}


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << " Usage: display_image ImageToLoadAndDisplay" << std::endl;
        return -1;
    }


    cv::namedWindow("Memtest", cv::WindowFlags::WINDOW_AUTOSIZE); // Create a window for display.

    cv::setMouseCallback("Memtest", onMouse, 0);


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