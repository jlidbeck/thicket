#include "treedemo.h"
#include <opencv2/highgui/highgui.hpp>
#include <thread>
#include <conio.h>
#include "SelfLimitingPolygonTree.h"
#include "ExactRationalAngleTree.h"

#define WIN32_LEAN_AND_MEAN      // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <WinUser.h>


TreeDemo g_treeDemo;

#pragma region OpenCV HighGUI callbacks

static void onMouse(int event, int x, int y, int, void*)
{
    if (event != cv::MouseEventTypes::EVENT_LBUTTONDOWN)
        return;

    cv::Point seed = cv::Point(x, y);
    auto pt = g_treeDemo.canvas.canvasToModel(seed);

    auto pTree = g_treeDemo.getTree();

    std::vector<qnode> nodes;

    // display info on node
    pTree->getNodesIntersecting(cv::Rect2f(pt, cv::Size2f(0, 0)), nodes);
    for (auto& node : nodes)
    {
        vector<string> lineage;
        pTree->getLineage(node, lineage);
        cout << "Node[" << node.id << "]:";
        for (auto& tname : lineage)
            cout << " " << tname;
        cout << endl;
    }
    return;

    // delete block
    pTree->getNodesIntersecting(cv::Rect2f(pt, cv::Size2f(11, 8)), nodes);
    for (auto &node : nodes)
    {
        pTree->removeNode(node.id);
    }
    pTree->redrawAll(g_treeDemo.canvas);
    cv::imshow("Memtest", g_treeDemo.canvas.getImage()); // Show our image inside it.
}

#pragma endregion


//  Process one node (if stepping), or until completion, or a key is pressed
//void consoleStep()
//{
//    int key = -1;
//
//    while (!the.m_quit && !the.pTree->nodeQueue.empty())
//    {
//        // process 1-64 nodes, uninterrupted
//        int nodesProcessed = the.processNodes();
//
//        // update display
//        imshow("Memtest", the.canvas.image); // Show our image inside it.
//        key = cv::waitKey(1);   // allows redraw
//
//        the.showReport(1.0);
//
//        if (the.m_stepping || ::_kbhit())
//        {
//            key = ::_getch();
//            if (key == 0 || key == 0xE0)    // arrow, function keys sent as 2 sequential codes
//                key = ::_getch();
//
//            the.processKey(key);
//
//            break;
//        }
//    }   // while(no key pressed and not complete)
//
//}

void redrawCallback()
{
    imshow("Memtest", g_treeDemo.canvas.getImage()); // Show our image inside it.
    auto key = cv::waitKey(1);   // allows redraw
}


//  Unsure how to interpret the param values...
LRESULT CALLBACK KeyboardProc(
    _In_ int    code,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    if (code < 0)
    {
        return ::CallNextHookEx(0, code, wParam, lParam);
    }

    if (g_treeDemo.processKey(lParam & 0xFFFF))
    {
        return 0;
    }

    printf("KeyboardProc(%d, %lld, %llx)\n", code, wParam, lParam);

    return ::CallNextHookEx(0, code, wParam, lParam);
}


int main(int argc, char** argv)
{
    runTests();
    ExactRationalAngleTree tree6;

    if (argc != 2)
    {
        cout << " Usage: display_image ImageToLoadAndDisplay" << endl;
        return -1;
    }

    HWND hwnd = ::GetConsoleWindow();
    HINSTANCE hmod = (HINSTANCE)::GetWindowLongPtr(hwnd, -6);
    DWORD dwThreadId=0;
    
    // not using keyboard hook
    //::SetWindowsHookEx(WH_KEYBOARD_LL, &KeyboardProc, hmod, dwThreadId);

    cv::namedWindow("Memtest", cv::WindowFlags::WINDOW_AUTOSIZE); // Create a window for display.

    cv::setMouseCallback("Memtest", onMouse, 0);

    g_treeDemo.restart(true);

    //  Main console program loop
    while (!g_treeDemo.isQuitting())
    {
        redrawCallback();
            //consoleStep();

        while (g_treeDemo.isWorkerTaskRunning() && !::_kbhit())
        {
            cv::imshow("Memtest", g_treeDemo.canvas.getImage()); // Show our image inside it.
            cv::waitKey(1);

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.03s);
        }

        if (g_treeDemo.getTree()->nodeQueue.empty())
        {
            g_treeDemo.showReport(0.0);
            cout << "Run complete.\n";

            //the.showCommands();
        }

        //int key = cv::waitKey(-1);
        int key = ::_getch();
        if (key == 0 || key == 0xE0)    // arrow, function keys sent as 2 sequential codes
            key = ::_getch();
        g_treeDemo.processKey(key);

    }

    //try {
        //the.load(argv[1]);
    //consoleRun();
    //}
    //catch (cv::Exception ex)
    //{
    //    cerr << "cv exception: " << ex.what();
    //    return -1;
    //}

    return 0;
}
