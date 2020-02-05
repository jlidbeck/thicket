#include "treedemo.h"


namespace fs = std::filesystem;


using std::cout;
using std::cin;
using std::endl;


TreeDemo::~TreeDemo()
{
    std::lock_guard lock(m_mutex);
    endWorkerTask();
}


bool TreeDemo::isValid() const
{
    std::lock_guard lock(m_mutex);

    return (pTree != nullptr && m_currentRun.valid() && !m_currentRun._Is_ready());
}

bool TreeDemo::isWorkerTaskRunning() const
{ 
    std::lock_guard lock(m_mutex);

    return (m_currentRun.valid() && !m_currentRun._Is_ready());
}

void TreeDemo::endWorkerTask()
{
    //std::lock_guard lock(m_mutex);

    if (m_currentRun.valid())
    {
        printf("### Waiting for worker task ###\n");

        m_cancel = true;

        m_currentRun.wait();
        std::future<void> invalid;
        std::swap(m_currentRun, invalid);

        m_cancel = false;
    }
}


void TreeDemo::startWorkerTask()
{
    //std::lock_guard lock(m_mutex);

    if (isWorkerTaskRunning())
    {
        // already running
        return;
    }

    printf("### Starting worker task ###\n");

    m_currentRun = std::async(std::launch::async, [&] {

        while (!pTree->nodeQueue.empty())
        {
            if (m_cancel)
                return;

            int nodesProcessed = processNodes();

            // update display
            sendProgressUpdate();

            if (m_stepping) 
                break;

        }   // while(no key pressed and not complete)

        sendProgressUpdate();
    });

}


// sets stepping mode and performs one step
bool TreeDemo::beginStepMode()
{
    if (pTree == nullptr)
        return false;

    endWorkerTask();

    m_stepping = true;
    m_maxNodesProcessedPerFrame = 1;

    if (pTree->nodeQueue.empty())
        restart();
    int nodesProcessed = processNodes();

    return true;
}

bool TreeDemo::endStepMode()
{
    if (pTree == nullptr)
        return false;

    m_stepping = false;
    m_maxNodesProcessedPerFrame = 64;

    if (pTree->nodeQueue.empty())
        restart();

    // is worker thread complete? if so, kick it off
    startWorkerTask();

    return true;
}

void TreeDemo::sendProgressUpdate()
{
    if (!!m_progressCallback)
    {
        //std::lock_guard lock(m_mutex);
        m_quit |= (0 != m_progressCallback(1, m_totalNodesProcessed));
    }
    else
    {
        showReport(1.0);
    }
}

int TreeDemo::processNodes()
{
    int nodesProcessed = 0;
    while (!pTree->nodeQueue.empty()
        && nodesProcessed < m_maxNodesProcessedPerFrame
        //&& pTree->nodeQueue.top().det() >= cutoff 
        && (nodesProcessed < m_minNodesProcessedPerFrame || pTree->nodeQueue.top().beginTime <= m_modelTime)
        )
    {
        //std::lock_guard lock(m_mutex);

        auto currentNode = pTree->nodeQueue.top();
        if (!pTree->isViable(currentNode))
        {
            pTree->nodeQueue.pop();
            continue;
        }

        pTree->drawNode(canvas, currentNode);

        nodesProcessed++;
        pTree->process();
        m_modelTime = currentNode.beginTime + 1.0;
    }

    m_totalNodesProcessed += nodesProcessed;

    sendProgressUpdate();

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
    //std::lock_guard lock(m_mutex);

    if (m_restart)
    {
        endWorkerTask();

        cout << "--- Starting run --- " << endl;

        if (!pTree)
        {
            pTree = m_defaultTree.clone();
            cout << "--- Set default tree\n";
        }

        if (m_randomize)
        {
            pTree->setRandomSeed(++m_presetIndex);
            //pTree->name = std::string("R") + std::to_string(m_presetIndex);
            m_randomize = false;
            cout << "--- Randomized [" << m_presetIndex << +"]\n";
        }
        pTree->create();
        pTree->transformCounts.clear();

        //json settingsJson;
        //pTree->to_json(settingsJson);
        //cout << settingsJson << endl;

        cout << "--- Starting run: " << pTree->name << ": " << pTree->transforms.size() << " transforms" << endl;

        canvas.create(cv::Mat3b(m_renderSize));
        canvas.clear();
        canvas.setTransformToFit(pTree->getBoundingRect(), m_imagePadding);


        m_startTime = std::chrono::steady_clock::now();
        m_lastReportTime = 0;

        m_modelTime = 0;
        m_totalNodesProcessed = 0;

        m_restart = false;

        sendProgressUpdate();
    }

    if (!m_stepping)
    {
        startWorkerTask();
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
    double curTime = std::chrono::duration_cast<std::chrono::duration<double>>(t - m_startTime).count();
    if (curTime - m_lastReportTime < debounceSeconds)
    {
        return;
    }

    m_lastReportTime = curTime;
    cout << std::setw(8) << curTime << ": " << m_totalNodesProcessed << " nodes processed (" << ((double)m_totalNodesProcessed) / curTime << "/s)" << endl;
}

int TreeDemo::openSettingsFile(int idx)
{
    char filename[50];
    sprintf_s(filename, "tree%04d.settings.json", idx);
    return openSettingsFile(fs::path(filename));
}

int TreeDemo::openSettingsFile(fs::path path)
{
    //  If path indicates a PNG, look for the corresponding settings file
    auto x = path.extension().native();
    if (path.extension().native().compare(fs::path(L".png").native()) == 0)
    {
        path.replace_extension("settings.json");
    }

    cout << "Opening " << path << "...\n";

    try 
    {
        std::ifstream infile(path);

        if (!infile)
        {
            cout << "Unable to open " << fs::absolute(path) << endl;
            return -1;
        }

        json j;
        infile >> j;
        pTree = qtree::createTreeFromJson(j);

        cout << "Settings read from: " << path << endl;

        if (pTree->name.empty())
        {
            //  set name attribute to default: filename without .settings.json
            while (path.has_extension())
            {
                path = path.stem();
            }
            pTree->name = path.generic_string();
        }

        restart();

        return 0;
    }
    catch (std::exception &ex)
    {
        cout << "Failed to read settings from " << fs::absolute(path) << "\n" << ex.what() << endl;
    }
    return -1;
}

const int MAX = 1000;

//  returns the largest-numbered file less than endIndex, or
//  -1 if no saved files are found
int TreeDemo::findPreviousFile(int endIndex) const
{
    fs::path jsonPath;

    int lastIdx = -1;
    for (auto& p : fs::directory_iterator("."))
    {
        //std::cout << p.path() << '\n';
        int idx;
        if (sscanf_s(p.path().filename().string().c_str(), "tree%d.settings.json", &idx) == 1)
        {
            if (idx > lastIdx && idx < endIndex)
                lastIdx = idx;
        }
    }
        
    return lastIdx;
}

//  returns the next-highest-numbered existing file greater than startIndex,
//  or -1 if no higher-numbered files are found
int TreeDemo::findNextFile(int startIndex) const
{
    fs::path jsonPath;
    char filename[40];
    for (int i = startIndex+1; i < MAX; ++i)
    {
        sprintf_s(filename, "tree%04d.settings.json", i);
        if (fs::exists(filename))
        {
            return i;
        }
    }

    return -1;
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
        endWorkerTask();
        m_quit = true;
        return true;

    case '?':
        showCommands();
        return true;

    case 's':
        save();
        return true;

    case 'o':
    case 0x710000:  // F2
    {
        if (m_currentFileIndex < 0)
        {
            m_currentFileIndex = findMostRecentFileIndex();
        }

        if (m_currentFileIndex < 0)
        {
            cout << "No files in current directory.\n";
            return true;
        }

        int idx = -1;
        cout << "Current file index is " << m_currentFileIndex << ".\nEnter index: ";
        if (std::cin >> idx)
        {
            if (idx < 0)
                idx = m_currentFileIndex;
            openSettingsFile(idx);
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
        m_renderSize = (m_renderSize == m_renderSizePreview ? m_renderSizeHD : m_renderSizePreview);
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
    case 0x790000:  // F10
    {
        beginStepMode();
        return true;
    }

    case ',':
    case 188:
    case 0x740000:  // F5
    {
        endStepMode();
        return true;
    }

    case ' ':
    {
        endStepMode();
        restart();
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
    //        cv::imshow("Memtest", canvas.m_image); // Show our image inside it.
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
        m_breeders.push_back(pTree->clone());
        cout << "** tree " << m_breeders.back()->name << " cloned to breed pool [" << m_breeders.size() - 1 << "] **\n";
        return true;
    }

    case 'b':
    {
        if (m_breeders.empty())
        {
            m_breeders.push_back(pTree->clone());
            cout << "** tree "<< m_breeders.back()->name<<" cloned to breed pool [" << m_breeders.size()-1 << "] **\n";
        }
        else
        {
            auto other = m_breeders.back();
            cout << "** Breeding current " << pTree->name << endl;
            cout << "** Breeding with " << other->name << endl;
            pTree->combineWith(*other, 0.1);
            cout << "** trees combined: "<<pTree->name<<" **\n";
            //restart = true;
        }
        return true;
    }

	case 2:		// Ctrl+B: swap current with breeding cache
	{
        if (m_breeders.empty())
		{
			cout << "** Nothing in breeding pool\n";
		}
        else
        {
            // should we end the worker task first?
			std::swap(m_breeders.back(), pTree);
			restart();
			cout << "** Swapped with breeding cache: loaded " << pTree->name << endl;
		}
		return true;
	}

    case 0x100000:  // shift
    case 0x110000:  // ctrl
    case 0x5b0000:  // Windows key
        return true;

    default:
        printf("? %d %x\n", key, key);
    }

    return false;
}

int TreeDemo::save()
{
    std::lock_guard lock(m_mutex);

    gotoNextUnusedFileIndex();

    cout << "Saving m_image and settings: " << m_currentFileIndex << endl;

    char name[24];
    sprintf_s(name, "tree%04d", m_currentFileIndex);

    // Write raster image to PNG file

    fs::path imagePath = std::string(name) + ".png";
    cv::imwrite(imagePath.string(), canvas.getImage());

    // Write vector image to SVG file

    auto svgPath = imagePath.replace_extension("svg");
    canvas.getSVG().save(svgPath.string());

    if (pTree->name.empty())
    {
        pTree->name = name;
    }

    // allow extending classes to customize the save
    pTree->saveImage(imagePath, canvas);

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
    if (m_currentFileIndex <= 0)
        m_currentFileIndex = findMostRecentFileIndex();
    else
        m_currentFileIndex = findPreviousFile(m_currentFileIndex);

    if (m_currentFileIndex < 0)
    {
        printf("No files found in current directory\n");
        return -1;
    }
    return openSettingsFile(m_currentFileIndex);
}

int TreeDemo::openNext()
{
    m_currentFileIndex = findNextFile(m_currentFileIndex);
    if (m_currentFileIndex < 0)
    {
        printf("No files found in current directory\n");
        return -1;
    }
    return openSettingsFile(m_currentFileIndex);
}

int TreeDemo::load(fs::path imagePath)
{
    imagePath = fs::absolute(imagePath);
    m_loadedImage = cv::imread(imagePath.string(), cv::ImreadModes::IMREAD_COLOR); // Read the file
    if (!m_loadedImage.data) // Check for invalid input
    {
        cout << "Could not open or find the image " << imagePath << endl;
        return -1;
    }

    //canvas.m_image = loadedImage;

    return 0;
}



