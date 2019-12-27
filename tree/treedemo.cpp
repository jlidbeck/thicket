#include "treedemo.h"


namespace fs = std::filesystem;


using std::cout;
using std::cin;
using std::endl;


cv::Size renderSizePreview  = cv::Size(200, 200);
cv::Size renderSizeHD       = cv::Size(2000, 1500);


void TreeDemo::consoleRun()
{
    processCommands();

    while (!m_quit)
    {
        consoleStep();

        if (pTree->nodeQueue.empty())
        {
            showReport(0.0);
            cout << "Run complete.\n";

            showCommands();
            //int key = cv::waitKey(-1);
            int key = ::_getch();
            if (key == 0 || key == 0xE0)    // arrow, function keys sent as 2 sequential codes
                key = ::_getch();
            processKey(key);
        }

    }
}

void TreeDemo::consoleStep()
{
    int key = -1;

    while (!pTree->nodeQueue.empty())
    {
        if (!m_stepping)
        {
            int nodesProcessed = processNodes();
        }

        // update display
        imshow("Memtest", canvas.image); // Show our image inside it.
        key = cv::waitKey(1);   // allows redraw

        showReport(1.0);

        if (m_stepping || ::_kbhit())
        {
            key = ::_getch();
            if (key == 0 || key == 0xE0)    // arrow, function keys sent as 2 sequential codes
                key = ::_getch();

            break;
        }
    }   // while(no key pressed and not complete)

    processKey(key);
}

// sets stepping mode and performs one step
bool TreeDemo::beginStepMode()
{
    m_stepping = true;
    maxNodesProcessedPerFrame = 1;

    if (pTree->nodeQueue.empty())
        restart();
    int nodesProcessed = processNodes();

    return true;
}

bool TreeDemo::endStepMode()
{
    m_stepping = false;
    maxNodesProcessedPerFrame = 64;

    if (pTree->nodeQueue.empty())
        restart();

    return true;
}

int TreeDemo::processNodes()
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

    totalNodesProcessed += nodesProcessed;

    return nodesProcessed;
}

void TreeDemo::restart(bool randomize)
{
    m_restart = true;
    m_randomize = randomize;
    processCommands();
}

void TreeDemo::processCommands()
{
    if (m_restart)
    {
        cout << "--- Starting run --- " << endl;

        if (!pTree)
        {
            pTree = &defaultTree;
            cout << "--- Set default tree\n";
        }

        if (m_randomize)
        {
            pTree->setRandomSeed(++presetIndex);
            //pTree->name = std::string("R") + std::to_string(presetIndex);
            m_randomize = false;
            cout << "--- Randomized [" << presetIndex << +"]\n";
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

        m_restart = false;
    }
}



void TreeDemo::showCommands()
{
    cout << "| 'q' quit, 's' save, 'o',PgUp,PgDn open, 'C',' ' restart, '.'/',' step/continue, 'r' randomize, 'c' color, 'l' line color, 'p' polygon,\n"
        << "| domain adjustments: +/-/arrows/0/1/2, 't' transforms,\n"
        << "| breeding: ctrl-b swap, B stash, b breed, ESC to quit.\n";
}

void TreeDemo::showReport(double debounceSeconds)
{
    std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
    double curTime = std::chrono::duration_cast<std::chrono::duration<double>>(t - startTime).count();
    if (curTime - lastReportTime < debounceSeconds)
    {
        return;
    }

    lastReportTime = curTime;
    cout << std::setw(8) << curTime << ": " << totalNodesProcessed << " nodes processed (" << ((double)totalNodesProcessed) / curTime << "/s)" << endl;
}

int TreeDemo::openFile(int idx)
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

        restart();

        return 0;
    }
    catch (std::exception &ex)
    {
        cout << "Failed to open " << filename << ":\n" << ex.what() << endl;
    }
    return -1;
}

const int MAX = 1000;

//  finds most recent file and sets {currentFileIndex}.

void TreeDemo::findNextUnusedFileIndex()
{
    currentFileIndex = MAX;
    findPreviousFile();
    ++currentFileIndex;
}

//  currentFileIndex will be set to 0 if no saved files are found
void TreeDemo::findPreviousFile()
{
    if (currentFileIndex < 0)
        currentFileIndex = 0;

    fs::path jsonPath;
    char filename[40];

    int lastIdx = 0, prevIdx = 0;
    for (auto& p : fs::directory_iterator("."))
    {
        //std::cout << p.path() << '\n';
        int idx;
        if (sscanf_s(p.path().filename().string().c_str(), "tree%d.settings.json", &idx) == 1)
        {
            if (idx > lastIdx)
                lastIdx = idx;
            if (idx<currentFileIndex && idx>prevIdx)
                prevIdx = idx;
        }
    }
    if (prevIdx > 0) {
        currentFileIndex = prevIdx;
        return;
    }
    if (lastIdx > 0) {
        currentFileIndex = lastIdx;
        return;
    }

    currentFileIndex = -1;// nothing found
}

//  finds most recent file and sets {currentFileIndex}.
//  currentFileIndex will be set to 0 if no saved files are found
void TreeDemo::findNextFile()
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

bool TreeDemo::processKey(int key)
{
    // Top-level commands: work whether or not config is active

    switch (key)
    {
    case -1:
    case 0:
        return false; // no key pressed

    case 27:    // ESC
    case 'q':
        m_quit = true;
        return true;

    case '?':
        showCommands();
        return true;

    case 's':
        save();
        return true;

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
        return true;
    }

    case 'O':
    case 0x210000:      // PageUp (VK_PRIOR << 16)
    case 73:
    //case 33:
        openPrevious();
        return true;

    case 0x220000:      // PageDown (VK_NEXT << 16)
    case 81:
    //case 34:
        openNext();
        return true;

    case 'r':
    {
        // restart with new random settings
        restart(true);
        return true;
    }
    }

    //  Config-level commands: only active if config is loaded

    if (pTree == nullptr)
        return false;

    switch (key)
    {
    case 'h':           // HD/preview toggle
        renderSize = (renderSize == renderSizePreview ? renderSizeHD : renderSizePreview);
        restart();
        return true;

    case 'C':
    {
        pTree = pTree->clone();
        restart();
        cout << "Cloned\n";
        return true;
    }

    case '+':
    case 0x6B0000:      // VK_ADD << 16
        pTree->domain = pTree->domain * 2.0f;
        restart();
        return true;

    case '-':
    case 0x6D0000:      // VK_SUBTRACT << 16
        pTree->domain = pTree->domain * 0.5f;
        restart();
        return true;

    case '0':   // recenter model in domain
        pTree->domain.x = -0.5f * pTree->domain.width;
        pTree->domain.y = -0.5f * pTree->domain.height;
        restart();
        return true;

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
        restart();
        return true;
    }

    case '2':
        switch (pTree->domainShape)
        {
        case qtree::DomainShape::RECT: pTree->domainShape = qtree::DomainShape::ELLIPSE; restart(); return true;
        case qtree::DomainShape::ELLIPSE: pTree->domainShape = qtree::DomainShape::RECT; restart(); return true;
        }
        return true;
                
    case 0x250000:  // left (VK_LEFT << 16)
    //case 37:
    case 75:
        pTree->domain.x += 0.10f * std::min( pTree->domain.width, pTree->domain.height);
        restart();
        return true;

    case 0x260000:  // up
    //case 38:
    case 72:
        pTree->domain.y -= 0.10f * std::min( pTree->domain.width, pTree->domain.height);
        restart();
        return true;

    case 0x270000:  // right
    //case 39:
    case 77:
        pTree->domain.x -= 0.10f * std::min( pTree->domain.width, pTree->domain.height);
        restart();
        return true;

    case 0x280000:  // down
    //case 40:
    case 80:
        pTree->domain.y += 0.10f * std::min( pTree->domain.width, pTree->domain.height);
        restart();
        return true;

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
        restart();
        return true;

    case 'c':   // randomize colors
        pTree->randomizeTransforms(1);
        restart();
        return true;

    case 'p':   // randomize drawPolygon
        pTree->randomizeTransforms(4);
        restart();
        return true;

    case '.':
    case 190:
    {
        beginStepMode();
        return true;
    }

    case ',':
    case 188:
    {
        endStepMode();
        return true;
    }

    case ' ':
    {
        endStepMode();
        return true;
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
        return true;
    }

    case 'T':
    {
        int idx;
        cout << "Drop transform? ";
        if (cin >> idx)
        {
            pTree->transforms.erase(pTree->transforms.begin() + idx);
        }
        return true;
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
        return true;
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
        return true;
    }

    case 'B':
    {
        pBreedTree = pTree->clone();
        cout << "** tree " << pBreedTree->name << " cloned to breed **\n";
        return true;
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
        return true;
    }

	case 2:		// Ctrl+B: swap current with breeding cache
	{
		if (pBreedTree != nullptr)
		{
			std::swap(pBreedTree, pTree);
			restart();
			cout << "** Swapped with breeding cache: loaded " << pTree->name << endl;
		}
		else
		{
			cout << "** Nothing in breeding cache\n";
		}
		return true;
	}

    default:
        cout << "? " << key << endl;
    }

    return false;
}

int TreeDemo::save()
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

    return 0;
}

int TreeDemo::openPrevious()
{
    findPreviousFile();
    if (currentFileIndex < 0)
    {
        printf("No files found in current directory\n");
        return -1;
    }
    return openFile(currentFileIndex);
}

int TreeDemo::openNext()
{
    findNextFile();
    if (currentFileIndex < 0)
    {
        printf("No files found in current directory\n");
        return -1;
    }
    return openFile(currentFileIndex);
}

int TreeDemo::load(fs::path imagePath)
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

TreeDemo the;


#pragma region OpenCV HighGUI callbacks

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

#pragma endregion


//  Main console program loop

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
    the.consoleRun();
    //}
    //catch (cv::Exception ex)
    //{
    //    cerr << "cv exception: " << ex.what();
    //    return -1;
    //}

    return 0;
}
