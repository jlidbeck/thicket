#include "treedemo.h"


namespace fs = std::filesystem;


using std::cout;
using std::cin;
using std::endl;


TreeDemo::~TreeDemo()
{
    endWorkerTask();
}


bool TreeDemo::isValid() const
{
    std::lock_guard lock(m_mutex);

    return (m_pTree != nullptr && m_currentRun.valid() && !m_currentRun._Is_ready());
}

bool TreeDemo::isWorkerTaskRunning() const
{ 
    std::lock_guard lock(m_mutex);

    return (m_currentRun.valid() && !m_currentRun._Is_ready());
}

void TreeDemo::endWorkerTask()
{
    std::lock_guard lock(m_mutex);

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

        while (!m_pTree->nodeQueue.empty())
        {
            if (m_cancel)
                return;

            int nodesProcessed = processNodes();

            // update display
            sendProgressUpdate();

            if (m_stepping) 
                break;

        }   // while(no key pressed and not complete)

        auto sz = "COMPLETE: " + std::to_string(*m_pTree);

        sendProgressUpdate();
    });

}


// sets stepping mode and performs one step
bool TreeDemo::beginStepMode()
{
    if (m_pTree == nullptr)
        return false;

    endWorkerTask();

    m_stepping = true;
    m_maxNodesProcessedPerFrame = 1;

    if (m_pTree->nodeQueue.empty())
        restart();
    int nodesProcessed = processNodes();

    return true;
}

bool TreeDemo::endStepMode()
{
    if (m_pTree == nullptr)
        return false;

    m_stepping = false;
    m_maxNodesProcessedPerFrame = 64;

    if (m_pTree->nodeQueue.empty())
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
    while (!m_pTree->nodeQueue.empty()
        && nodesProcessed < m_maxNodesProcessedPerFrame
        //&& m_pTree->nodeQueue.top().det() >= cutoff 
        && (nodesProcessed < m_minNodesProcessedPerFrame || m_pTree->nodeQueue.top().beginTime <= m_modelTime)
        )
    {
        //std::lock_guard lock(m_mutex);

        auto currentNode = m_pTree->nodeQueue.top();
        if (!m_pTree->isViable(currentNode))
        {
            m_pTree->nodeQueue.pop();
            continue;
        }

        m_pTree->drawNode(canvas, currentNode);

        nodesProcessed++;
        m_pTree->process();
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

        if (!m_pTree)
        {
            m_pTree = m_defaultTree.clone();
            cout << "--- Set default tree\n";
        }

        auto sz = std::to_string(*m_pTree);

        if (m_randomize)
        {
            m_pTree->setRandomSeed(++m_presetIndex);
            //m_pTree->name = std::string("R") + std::to_string(m_presetIndex);
            m_randomize = false;
            cout << "--- Randomized [" << m_presetIndex << +"]\n";
            m_modified = true;
        }
        m_pTree->create();
        m_pTree->transformCounts.clear();

        //json settingsJson;
        //m_pTree->to_json(settingsJson);
        //cout << settingsJson << endl;

        cout << "--- Starting run: " << m_pTree->name << ": " << m_pTree->transforms.size() << " transforms" << endl;

        canvas.create(m_renderSettings);

        // todo: move this to renderer
        canvas.setTransformToFit(m_pTree->getBoundingRect(), m_renderSettings.imagePadding);


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
    endWorkerTask();

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
        m_pTree = qtree::createTreeFromJson(j);

        cout << "Settings read from: " << path << endl;
        cout << std::to_string(*m_pTree) << endl;

        if (m_pTree->name.empty())
        {
            //  set name attribute to default: filename without path or extension
            while (path.has_extension())
            {
                path = path.stem();
            }
            m_pTree->name = path.generic_string();
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

    if (m_pTree == nullptr)
        return false;

    switch (key)
    {
    case 'h':           // HD/preview toggle
        m_renderSettings.renderSize = 
            ( m_renderSettings.renderSize == m_renderSettings.renderSizePreview 
            ? m_renderSettings.renderSizeHD 
            : m_renderSettings.renderSizePreview );
        m_modified = true;
        restart();
        return true;

    case 'C':
    {
        m_pTree = m_pTree->clone();
        restart();
        cout << "Cloned\n";
        return true;
    }

    case '+':
    case 0x6B0000:      // VK_ADD << 16
        m_pTree->domain = m_pTree->domain * 2.0f;
        m_modified = true;
        restart();
        return true;

    case '-':
    case 0x6D0000:      // VK_SUBTRACT << 16
        m_pTree->domain = m_pTree->domain * 0.5f;
        m_modified = true;
        restart();
        return true;

    case '0':   // recenter model in domain
        m_pTree->domain.x = -0.5f * m_pTree->domain.width;
        m_pTree->domain.y = -0.5f * m_pTree->domain.height;
        restart();
        return true;

    case '1':
    {
        // cycle thru aspect ratios
        float a = m_pTree->domain.width / m_pTree->domain.height;
        if (a == 1.0f)
            m_pTree->domain.width *= 2.0f;
        else if (a == 2.0f)
            m_pTree->domain.width *= 2.0f;
        else if (a == 4.0f)
            m_pTree->domain = cv::Rect_<float>(m_pTree->domain.x, m_pTree->domain.y, m_pTree->domain.width / 4.0f, m_pTree->domain.height * 2.0f);
        else if (a == 0.5f)
            m_pTree->domain.height *= 2.0f;
        else if (a > 1.0f)
            m_pTree->domain.width = m_pTree->domain.height;
        else
            m_pTree->domain.height = m_pTree->domain.width;
        restart();
        return true;
    }

    case '2':
        switch (m_pTree->domainShape)
        {
        case qtree::DomainShape::RECT: m_pTree->domainShape = qtree::DomainShape::ELLIPSE; restart(); return true;
        case qtree::DomainShape::ELLIPSE: m_pTree->domainShape = qtree::DomainShape::RECT; restart(); return true;
        }
        return true;
                
    case 0x250000:  // left (VK_LEFT << 16)
    //case 37:
    case 75:
        m_pTree->domain.x += 0.10f * std::min( m_pTree->domain.width, m_pTree->domain.height);
        restart();
        return true;

    case 0x260000:  // up
    //case 38:
    case 72:
        m_pTree->domain.y -= 0.10f * std::min( m_pTree->domain.width, m_pTree->domain.height);
        restart();
        return true;

    case 0x270000:  // right
    //case 39:
    case 77:
        m_pTree->domain.x -= 0.10f * std::min( m_pTree->domain.width, m_pTree->domain.height);
        restart();
        return true;

    case 0x280000:  // down
    //case 40:
    case 80:
        m_pTree->domain.y += 0.10f * std::min( m_pTree->domain.width, m_pTree->domain.height);
        restart();
        return true;

    case 'l':   // cycle thru line options
        if (m_pTree->lineThickness == 0)
        {
            m_pTree->lineThickness = 1;
            m_pTree->lineColor = cv::Scalar(0, 0, 0);
        }
        else if (m_pTree->lineColor == cv::Scalar(0, 0, 0))
        {
            m_pTree->lineColor = cv::Scalar(255,255,255);
        }
        else
        {
            m_pTree->lineThickness = 0;
        }
        restart();
        return true;

    case 'c':   // randomize colors
        m_pTree->randomizeTransforms(1);
        restart();
        return true;

    case 'p':   // randomize drawPolygon
        m_pTree->randomizeTransforms(4);
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
        std::vector<int> indexes(m_pTree->transforms.size());
        std::iota(indexes.begin(), indexes.end(), 0);
        std::sort(indexes.begin(), indexes.end(), [&](int a, int b) { return m_pTree->transforms[a].gestation < m_pTree->transforms[b].gestation; });

        cout << "=== " << m_pTree->name << " transforms: " << endl;
        cout << "idx freq gest  key      color\n";
        for (auto i : indexes)
        {
            auto const &t = m_pTree->transforms[i];
            cout << std::setw(2) << i
                    << ":" << std::setw(5) << m_pTree->transformCounts[t.transformMatrixKey] 
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
        if ((cin >> idx) && idx>=0 && idx<m_pTree->transforms.size())
        {
            m_pTree->transforms.erase(m_pTree->transforms.begin() + idx);
        }
        return true;
    }

    case 20:    // Ctrl-T
    {
        // remove unexpressed genes
        size_t count = m_pTree->transforms.size();
        for (auto it = m_pTree->transforms.begin(); it != m_pTree->transforms.end(); )
        {
            if (m_pTree->transformCounts[it->transformMatrixKey] == 0)
            {
                it = m_pTree->transforms.erase(it);
            }
            else
            {
                ++it;
            }
        }
        cout << "=== " << (count - m_pTree->transforms.size()) << " transforms removed.\n";
        return true;
    }

    case 'x':
    //{
    //    int count = 0;

    //    std::vector<qnode> nodes;
    //    cv::Rect2f rc(2, 3, 10, 7);
    //    m_pTree->getNodesIntersecting(rc, nodes);
    //    for (auto &node : nodes)
    //    {
    //        std::vector<string> lineage;
    //        m_pTree->getLineage(node, lineage);
    //        cout << " x: [";
    //        for (auto it = lineage.rbegin(); it != lineage.rend(); ++it) cout << *it << " ";
    //        cout << "]\n";
    //        count += m_pTree->removeNode(node.id);
    //    }

    //    //for (int idx = 0; idx < 100000 && count<1; ++idx)
    //    //{
    //    //    count += m_pTree->removeNode(-1);
    //    //}

    //    if (count)
    //    {
    //        cout << "** remove(" << 42 << "): " << count << " nodes removed\n";
    //        m_pTree->redrawAll(canvas);
    //        cv::imshow("Memtest", canvas.m_image); // Show our image inside it.
    //        break;
    //    }
    //    break;
    //}

    //case 'X':
    {
        m_pTree->regrowAll();
        return true;
    }

    case 'B':
    {
        m_breeders.push_back(m_pTree->clone());
        cout << "** tree " << m_breeders.back()->name << " cloned to breed pool [" << m_breeders.size() - 1 << "] **\n";
        return true;
    }

    case 'b':
    {
        if (m_breeders.empty())
        {
            m_breeders.push_back(m_pTree->clone());
            cout << "** tree "<< m_breeders.back()->name<<" cloned to breed pool [" << m_breeders.size()-1 << "] **\n";
        }
        else
        {
            auto other = m_breeders.back();
            cout << "** Breeding current " << m_pTree->name << endl;
            cout << "** Breeding with " << other->name << endl;
            m_pTree->combineWith(*other, 0.1);
            cout << "** trees combined: "<<m_pTree->name<<" **\n";
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
			std::swap(m_breeders.back(), m_pTree);
			restart();
			cout << "** Swapped with breeding cache: loaded " << m_pTree->name << endl;
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

std::string TreeDemo::getName() const
{
    if (!m_pTree->name.empty())
    {
        return m_pTree->name;
    }

    char name[24];
    sprintf_s(name, "tree%04d", m_currentFileIndex);

    return name;
}

//  Saves settings and images.
//  Uses qtree::name if defined, otherwise creates a name.
void TreeDemo::save()
{
    std::lock_guard lock(m_mutex);

    gotoNextUnusedFileIndex();

    cout << "Saving image and settings: " << m_currentFileIndex << endl;

    std::string name = getName();
    save(name);

    if (m_pTree->name.empty())
    {
        m_pTree->name = name;
    }

}

//  Saves settings, image, and vector files, and any other files specific to the qtree.
//  The extension of {fileName} is ignored.
//  Multiple files are saved with .PNG, .SVG, and .settings.json extensions.
void TreeDemo::save(fs::path path) const
{
    //  save image, vector, and settings files as:
    //  {name}.png, {name}.svg, {name}.settings.json

    while (path.has_extension())
    {
        path.replace_extension();
    }

    saveImage(    path.replace_extension("png")           );
    saveSVG(      path.replace_extension("svg")           );
    saveSettings( path.replace_extension("settings.json") );
}

//  Save raster image to file
void TreeDemo::saveImage(fs::path imagePath) const
{
    cv::imwrite(imagePath.string(), canvas.getImage());

    // allow extending classes to customize the save
    m_pTree->saveImage(imagePath, canvas);
}

//  Save vector drawing to file
void TreeDemo::saveSVG(fs::path svgPath) const
{
    canvas.getSVG().save(svgPath.string());
}

//  Save settings as json file
void TreeDemo::saveSettings(fs::path settingsPath) const
{
    std::lock_guard lock(m_mutex);
    std::ofstream outfile(settingsPath);

    // throw exception on file errors
    outfile.exceptions(std::ofstream::badbit | std::ofstream::failbit);

    json j;
    m_pTree->to_json(j);
    outfile << std::setw(4) << j;
    outfile.flush();
    outfile.close();
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



