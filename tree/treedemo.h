#pragma once

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


class TreeDemo
{
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


public:
    // console-driven run loop
    void consoleRun();
    void consoleStep();

    bool beginStepMode();
    bool endStepMode();

    void restart(bool randomize=false);

    void processCommands();
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
