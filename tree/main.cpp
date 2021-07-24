#include "treedemo.h"
#include <opencv2/highgui/highgui.hpp>
#include <thread>
#include <conio.h>


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
    the.pTree->getNodesIntersecting(cv::Rect2f(pt, cv::Size2f(0, 0)), nodes);
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
    cv::imshow("Memtest", the.canvas.getImage()); // Show our image inside it.
}

#pragma endregion


void redrawCallback()
{
    imshow("Memtest", the.canvas.getImage()); // Show our image inside it.
    auto key = cv::waitKey(1);   // allows redraw
}


int main(int argc, char** argv)
{
    //if (argc != 2)
    //{
    //    cout << "Usage: " << argv[0] << " <Image FIle>" << endl;
    //    return -1;
    //}

    cv::namedWindow("Memtest", cv::WindowFlags::WINDOW_AUTOSIZE); // Create a window for display.

    cv::setMouseCallback("Memtest", onMouse, 0);

    the.restart(true);

    //  Main console program loop
    while (!the.isQuitting())
    {
        redrawCallback();

        while (the.isWorkerTaskRunning() && !::_kbhit())
        {
            cv::imshow("Memtest", the.canvas.getImage()); // Show our image inside it.
            cv::waitKey(1);

            using namespace std::chrono_literals;
            std::this_thread::sleep_for(0.03s);
        }

        if (the.pTree->nodeQueue.empty())
        {
            the.showReport(0.0);
            cout << "Run complete.\n";

            //the.showCommands();
        }

        //int key = cv::waitKey(-1);
        int key = ::_getch();
        if (key == 0 || key == 0xE0)    // arrow, function keys sent as 2 sequential codes
            key = ::_getch();
        the.processKey(key);

    }

    return 0;
}
