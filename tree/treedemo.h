#pragma once

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
public:
    std::shared_ptr<qtree> pTree;

    qcanvas canvas;

private:
    mutable std::mutex m_mutex;

    cv::Size m_renderSizePreview  = cv::Size( 200,  200);
    cv::Size m_renderSizeHD       = cv::Size(2000, 1500);
    cv::Size m_renderSize         = m_renderSizePreview;

    ThornTree const m_defaultTree;

    std::vector<std::shared_ptr<qtree> > m_breeders;

    cv::Mat m_loadedImage;

    int m_minNodesProcessedPerFrame = 1;
    int m_maxNodesProcessedPerFrame = 64;
    bool m_stepping = false;

    int m_presetIndex = 0;
    float m_imagePadding = 0.1f;

    // current model
    double m_modelTime;
    // current model run stats
    int m_totalNodesProcessed;
    std::chrono::steady_clock::time_point m_startTime;    // wall-clock time
    double m_lastReportTime;                              // wall-clock time

    // commands
    bool m_restart = true;
    bool m_randomize = true;
    bool m_quit = false;

    // current file pointer. should usually point to existing file "tree%04d"
    int m_currentFileIndex = -1;

    std::future<void> m_currentRun;
    bool m_cancel = false;

    void processCommands();

    void endWorkerTask();
    void startWorkerTask();

    void sendProgressUpdate();

public:
    bool isWorkerTaskRunning() const;
    bool isQuitting() const { return m_quit; }

    std::function<int(int, int)> m_progressCallback;

    bool beginStepMode();
    bool endStepMode();
    bool isStepping() const { return m_stepping; }

    void restart(bool randomize=false);

    int processNodes();

    bool processKey(int key);

    int save();
    int openNext();
    int openPrevious();
    int openFile(int idx);
    int load(fs::path imagePath);
    void findNextUnusedFileIndex();
    void findPreviousFile();
    void findNextFile();

    void showCommands();
    void showReport(double debounceSeconds);
};
