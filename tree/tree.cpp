#include "HexGridTree.h"
#include "GridTree.h"
#include "util.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <filesystem>
#include <random>
#include <vector>

namespace fs = std::filesystem;


using std::cout;
using std::endl;


class qcanvas
{
public:
    Matx33 globalTransform;
    cv::Mat image;

    qcanvas() {
    }

    void create(cv::Mat im) 
    {
        image = im;
    }

    // sets global transform map to map provided domain to image, centered, vertically flipped
    void setScaleToFit(cv::Rect_<float> const &rect, float buffer)
    {
        if (image.empty()) throw std::exception("Image is empty");

		globalTransform = util::transform3x3::centerAndFit(rect, cv::Rect_<float>(0, 0, image.cols, image.rows), buffer, true);
    }

};


qnode::qnode(double beginTime_) 
{
    beginTime = beginTime_;
    generation = 0;
    color = cv::Scalar(1, 1, 1, 1);
    globalTransform = globalTransform.eye();
}


#pragma region qnode tree


//  process one node
bool qtree::process()
{
    auto currentNode = nodeQueue.top();
    nodeQueue.pop();

    if (!isViable(currentNode))
        return false;

    addNode(currentNode);

    // create a child node for each available transform.
    // all child nodes are added to the queue, even if not viable.
    for (auto const &t : transforms)
    {
        qnode child;
        beget(currentNode, t, child);
        nodeQueue.push(child);
    }

    return true;
}

//  Generate a child node from parent
void qtree::beget(qnode const & parent, qtransform const & t, qnode & child)
{
    child.generation = parent.generation + 1;
    child.beginTime = parent.beginTime + t.gestation + util::r() * offspringTemporalRandomness;

    child.globalTransform = parent.globalTransform * t.transformMatrix;

    auto m = t.colorTransform * parent.color;
    child.color = m;
}

void qtree::drawNode(qcanvas &canvas, qnode const &node)
{
    if (!isViable(node))
        return;

    Matx33 m = canvas.globalTransform * node.globalTransform;

    vector<cv::Point2f> v;
    cv::transform(polygon, v, m.get_minor<2, 3>(0, 0));
    vector<vector<cv::Point> > pts(1);
    for (auto const& p : v)
        pts[0].push_back(p * 16);

    cv::Scalar color = util::toColor(node.color);
    cv::Scalar lineColor = cv::Scalar(255, 255, 255, 255);
    cv::fillPoly(canvas.image, pts, color, cv::LineTypes::LINE_AA, 4);
    //cv::polylines(canvas.image, pts, true, lineColor, 1, cv::LineTypes::LINE_AA, 4);
}


#pragma endregion



class The
{
public:
    cv::Mat loadedImage;
    qcanvas canvas;

    //ReptileTree tree;
    //GridTree tree;
    //SelfAvoidantPolygonTree tree;
    //ScaledPolygonTree tree;
    TrapezoidTree tree;

    int minNodesProcessedPerFrame = 1;
    int maxNodesProcessedPerFrame = 64;

    int presetIndex = 7;
    float imagePadding = 0.1f;

    void run()
    {
        tree.create(presetIndex);

        cv::Mat3b img(cv::Size(1200, 1200));
        img = 0;

        canvas.create(img);
        canvas.setScaleToFit(tree.getBoundingRect(), imagePadding);

        //qtransform  t1( 30, halfRoot(6), cv::Point2f(0, 90)),
        //            t2(-30, halfRoot(6), cv::Point2f(0, 60));

        //qtransform  t1( 90, halfRoot(2), cv::Point2f(0,  0.9)),
        //            t2(-90, halfRoot(2), cv::Point2f(0, -0.9));

        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> dis(0, canvas.image.cols - 50);
        //cv::Rect rect(0, 0, 50, 50);

        while (1)
        {
            double time = 0;

            while (!tree.nodeQueue.empty())
            {
                ++time;

                int nodesProcessed = 0;
                while (!tree.nodeQueue.empty() 
                        && nodesProcessed < maxNodesProcessedPerFrame
                        //&& tree.nodeQueue.top().det() >= cutoff 
                        && (nodesProcessed < minNodesProcessedPerFrame || tree.nodeQueue.top().beginTime <= time)
                    )
                {
                    auto currentNode = tree.nodeQueue.top();
                    if (!tree.isViable(currentNode))
                    {
                        tree.nodeQueue.pop();
                        continue;
                    }

                    tree.drawNode(canvas, currentNode);

                    nodesProcessed++;
                    tree.process();
                    time = currentNode.beginTime + 1.0;
                }

                // adjust scaling to fit.. can't do this yet b/c tree can't be redrawn
                //canvas.setScaleToFit(tree.getBoundingRect());

                // fade
                //canvas.image *= 0.99;

                // update display
                imshow("Memtest", canvas.image); // Show our image inside it.

                int key = cv::waitKey(1);
                if (!processKey(key))
                    return;
            }

            cout << "Run complete. 's' to save, 'r' to randomize, ESC to quit.\n";
            int key = cv::waitKey(0);
            if (!processKey(key))
                return;

        }
    }

    bool processKey(int key)
    {
        switch (key)
        {
        case -1:
            return true; // no key pressed

        case 's':
        {
            fs::path imagePath = "temp0001.png";
            char filename[24];
            for (int count = 1; fs::exists(imagePath); ++count)
            {
                sprintf_s(filename, "temp%04d.png", count);
                imagePath = filename;
            }

            cv::imwrite(imagePath.string(), canvas.image);
            std::cout << "Image saved: " << imagePath << std::endl;
            return true;
        }

        case 'r':
        {
            canvas.image = 0;
            presetIndex = (++presetIndex);
            tree.create(presetIndex);
            canvas.setScaleToFit(tree.getBoundingRect(), imagePadding);
            return true;
        }

        case '.':
        {
            maxNodesProcessedPerFrame = 1;
            break;
        }

        case ' ':
        {
            // restart with current settings
            maxNodesProcessedPerFrame = 64;
            canvas.image = 0;
            //presetIndex = (presetIndex + ReptileTree::NUM_PRESETS) % (ReptileTree::NUM_PRESETS * 2);
            tree.create(presetIndex);
            canvas.setScaleToFit(tree.getBoundingRect(), imagePadding);
            return true;
        }

        case 27:    // ESC
        default:
            return false;
        }

        return false;
    }

    int load(fs::path imagePath)
    {
        imagePath = fs::absolute(imagePath);
        loadedImage = cv::imread(imagePath.string(), cv::ImreadModes::IMREAD_COLOR); // Read the file
        if (!loadedImage.data) // Check for invalid input
        {
            std::cout << "Could not open or find the image " << imagePath << std::endl;
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

    the.canvas.image = the.loadedImage;
}


int main( int argc, char** argv )
{
    if( argc != 2)
    {
        std::cout <<" Usage: display_image ImageToLoadAndDisplay" << std::endl;
        return -1;
    }


    cv::namedWindow("Memtest", cv::WindowFlags::WINDOW_AUTOSIZE); // Create a window for display.

    cv::setMouseCallback("image", onMouse, 0);


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