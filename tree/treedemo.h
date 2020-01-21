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
    mutable std::mutex demo_mutex;

public:
    cv::Mat loadedImage;
    qcanvas canvas;

    cv::Size renderSizePreview  = cv::Size(200, 200);
    cv::Size renderSizeHD       = cv::Size(2000, 1500);
    cv::Size renderSize         = renderSizePreview;

    ThornTree defaultTree;
    qtree *pTree = nullptr;
    qtree *pBreedTree = nullptr;

    int minNodesProcessedPerFrame = 1;
    int maxNodesProcessedPerFrame = 64;
    bool m_stepping = false;

    int presetIndex = 0;
    float imagePadding = 0.1f;

    // current model
    double modelTime;
    // current model run stats
    int totalNodesProcessed;
    std::chrono::steady_clock::time_point startTime;    // wall-clock time
    double lastReportTime;                              // wall-clock time

    // commands
    bool m_restart = true;
    bool m_randomize = true;
    bool m_quit = false;

    // current file pointer. should usually point to existing file "tree%04d"
    int currentFileIndex = -1;

private:
    std::future<void> s_run;
    bool s_cancel = false;

    void processCommands();

    void endWorkerTask();
    void startWorkerTask();

    void sendProgressUpdate();

public:
    bool isWorkerTaskRunning() const;

    std::condition_variable cv;
    std::function<int(int, int)> m_progressCallback;

    bool beginStepMode();
    bool endStepMode();

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
