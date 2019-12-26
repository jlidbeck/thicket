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
#include <conio.h>


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
    qtree *pBreedTree = nullptr;

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
                key = cv::waitKey(1);   // allows redraw

                totalNodesProcessed += nodesProcessed;

                std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
                std::chrono::duration<double> lapsed = std::chrono::duration_cast<std::chrono::duration<double>>(t - startTime);
                if (lapsed.count() - lastReportTime > 1.0)
                {
                    showReport();
                }

                if (stepping) 
                {
                    //key = cv::waitKey(0);
                    key = ::_getch();
                    break;
                }

                if (::_kbhit())
                {
                    key = ::_getch();
                    break;
                }
            }   // while(no key pressed and not complete)

            processKey(key);

            if (quit) return;

            if (restart) continue;

            if (pTree->nodeQueue.empty())
            {
                showReport();
                cout << "Run complete.\n";

                cout << "| 'q' quit, 's' save, 'o',PgUp,PgDn open, 'C',' ' restart, '.'/',' step/continue, 'r' randomize, 'c' color, 'l' line color, 'p' polygon,\n"
                     << "| domain adjustments: +/-/arrows/0/1/2, 't' transforms,\n"
                     << "| breeding: ctrl-b swap, B stash, b breed, ESC to quit.\n";
                int key = ::_getch();
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

            cout << "Settings read from: " << filename << endl;

            if (pTree->name.empty())
                pTree->name = std::to_string(idx);

            restart = true;
            randomize = false;
        }
        catch (std::exception &ex)
        {
            cout << "Failed to open " << filename << ":\n" << ex.what() << endl;
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

            cout << "Saving image and settings: " << currentFileIndex << endl;

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

            cout << "Image saved: " << imagePath << endl;

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

        case 'O':
        case 0x210000:      // PageUp
        case 73:
            findPreviousFile();
            openFile(currentFileIndex);
            break;

        case 0x220000:      // PageDown
        case 81:
        {
            findNextFile();
            openFile(currentFileIndex);
            break;
        }

        case 'h':           // HD/preview toggle
            renderSize = (renderSize == renderSizePreview ? renderSizeHD : renderSizePreview);
            restart = true;
            break;

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
        case 75:
            pTree->domain.x += 0.10f * std::min( pTree->domain.width, pTree->domain.height);
            restart = true;
            break;

        case 0x260000:  // up
        case 72:
            pTree->domain.y -= 0.10f * std::min( pTree->domain.width, pTree->domain.height);
            restart = true;
            break;

        case 0x270000:  // right
        case 77:
            pTree->domain.x -= 0.10f * std::min( pTree->domain.width, pTree->domain.height);
            restart = true;
            break;

        case 0x280000:  // down
        case 80:
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
        {
            // sort in order of gestation, increasing
            std::vector<int> indexes(pTree->transforms.size());
            std::iota(indexes.begin(), indexes.end(), 0);
            std::sort(indexes.begin(), indexes.end(), [&](int a, int b) { return pTree->transforms[a].gestation < pTree->transforms[b].gestation; });

            cout << "=== " << pTree->name << " transforms: " << endl;
            cout << "idx freq gest  key      color\n";
            for (auto i : indexes)
            {
                auto const &t = pTree->transforms[i];
                cout << std::setw(2) << i
                     << ":" << std::setw(5) << pTree->transformCounts[t.transformMatrixKey] 
                     << " " << std::setw(4) << std::setprecision(3) << t.gestation
                     << "  " << t.transformMatrixKey 
                     << "  " << t.colorTransform.description() << endl;
            }
            break;
        }

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

        case 20:    // Ctrl-T
        {
            // remove unexpressed genes
            int count = pTree->transforms.size();
            for (auto it = pTree->transforms.begin(); it != pTree->transforms.end(); )
            {
                if (pTree->transformCounts[it->transformMatrixKey] == 0)
                {
                    it = pTree->transforms.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            cout << "=== " << (count - pTree->transforms.size()) << " transforms removed.\n";
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

        case 'B':
        {
            pBreedTree = pTree->clone();
            cout << "** tree " << pBreedTree->name << " cloned to breed **\n";
            break;
        }

        case 'b':
        {
            if (pBreedTree == nullptr)
            {
                pBreedTree = pTree->clone();
                cout << "** tree "<<pBreedTree->name<<" cloned to breed **\n";
            }
            else
            {
                cout << "** Breeding current " << pTree->name << endl;
                cout << "** Breeding with " << pBreedTree->name << endl;
                pTree->combineWith(*pBreedTree, 0.1);
                cout << "** trees combined: "<<pTree->name<<" **\n";
                //restart = true;
            }
            break;
        }

		case 2:		// Ctrl+B: swap current with breeding cache
		{
			if (pBreedTree != nullptr)
			{
				std::swap(pBreedTree, pTree);
				restart = true;
				cout << "** Swapped with breeding cache: loaded " << pTree->name << endl;
			}
			else
			{
				cout << "** Nothing in breeding cache\n";
			}
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
            cout << "Could not open or find the image " << imagePath << endl;
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
        cout << " Usage: display_image ImageToLoadAndDisplay" << endl;
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