#pragma once

#include "HatTree.h"
#include "SelfLimitingPolygonTree.h"
#include "GridTree.h"
#include "ReptileTree.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <chrono>
#include <future>
#include <mutex>
#include <condition_variable>


namespace fs = std::filesystem;


class TreeDemo
{
    std::shared_ptr<qtree> m_pTree;
public:

    qcanvas canvas;

private:
    mutable std::mutex m_mutex;

    RenderSettings m_renderSettings;

    HatTree const m_defaultTree;

    std::vector<std::shared_ptr<qtree> > m_breeders;

    cv::Mat m_loadedImage;

    int m_minNodesProcessedPerFrame = 1;
    int m_maxNodesProcessedPerFrame = 64;
    bool m_stepping = false;

    int m_presetIndex = 0;

    // current model
    double m_modelTime = 0;
    // current model run stats
    int m_totalNodesProcessed = 0;
    std::chrono::steady_clock::time_point m_startTime;    // wall-clock time
    double m_lastReportTime = 0;                              // wall-clock time

    // commands
    bool m_restart = true;
    bool m_randomize = true;
    bool m_quit = false;

    // current file pointer. should usually point to existing file "tree%04d"
    int m_currentFileIndex = -1;

    std::future<void> m_currentRun;
    bool m_cancel = false;

public:
    TreeDemo() {}
    ~TreeDemo();

    std::shared_ptr<qtree>  getTree() const { return m_pTree; }
    std::mutex&             getMutex()      { return m_mutex; }

protected:
    void processCommands();

public:
    void endWorkerTask();
    void startWorkerTask();

    void sendProgressUpdate();

public:
    bool isValid() const;
    bool isWorkerTaskRunning() const;
    bool isQuitting() const { return m_quit; }

    std::function<int(int, int)> m_progressCallback;

    bool beginStepMode();
    bool endStepMode();
    bool isStepping() const { return m_stepping; }

    void restart(bool randomize=false);

private:
    int processNodes();

public:
    bool processKey(int key);

    void save();
    void save(fs::path fileName) const;
    void saveSettings(fs::path settingsPath) const;
    void saveImage(fs::path imagePath) const;
    void saveSVG(fs::path svgPath) const;
    std::string getName() const;
    int openNext();
    int openPrevious();
    int openSettingsFile(int idx);
    int openSettingsFile(fs::path settingsPath);
    int load(fs::path imagePath);
    void gotoNextUnusedFileIndex() { m_currentFileIndex = findMostRecentFileIndex() + 1; }
    int findMostRecentFileIndex() const { return findPreviousFile(9999); }
    int findPreviousFile(int idx) const;
    int findNextFile(int idx) const;

    void showCommands();
    void showReport(double debounceSeconds);

    RenderSettings const& getRenderSettings() const { return m_renderSettings; }
    void setRenderSettings(RenderSettings const& rs) { m_renderSettings = rs; } // requires restart()

};
