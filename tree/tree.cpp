#include "tree.h"
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
    void setScaleToFit(cv::Rect_<float> const &rect)
    {
        if (image.empty()) throw std::exception("Image is empty");

		globalTransform = util::transform3x3::centerAndFit(rect, cv::Rect_<float>(0, 0, image.cols, image.rows), 0.2f, true);
    }

};


qnode::qnode(double beginTime_) 
    {
        beginTime = beginTime_;

        m_accum = m_accum.eye();
        generation = 0;
    }


#define Half12  0.94387431268

//  (1/2)^(1/n)
//  growth factor with a halflife of {n}
float halfRoot(int n)
{
    return pow(0.5f, 1.0f / (float)n);
}


#pragma region qnode tree

#define NUM_PRESETS 8

void qtree::create(int settings)
{
    m_boundingRect = cv::Rect_<float>(0, 0, 0, 0);

    rootNode.m_color = Matx41(0.5, 0.5, 0.5, 1);
    rootNode.m_accum = Matx33::eye();

    switch (settings % NUM_PRESETS)
    {
    case 0:
        // H hourglass
        polygon = { {
            { .1f,-1}, { .08,0}, { .1, 1},
            {-.1, 1}, {-.08,0}, {-.1,-1}
            } };

        transforms = { {
                qtransform(90, halfRoot(2), cv::Point2f(0,  0.9)),
                qtransform(-90, halfRoot(2), cv::Point2f(0, -0.9))
            } };

        rootNode.m_accum = util::transform3x3::getRotationMatrix2D(cv::Point2f(), 90, 1);

        m_boundingRect = cv::Rect_<float>(-2, -2, 4, 4);

        break;

    case 1:
        polygon = { {
                // L
                {0.0f,0},{0.62,0},{0.62f,0.3f},{0.3f,0.3f},{0.3f,1},{0,1}
            } };

        transforms = { {
                qtransform(90, halfRoot(2), cv::Point2f(0,  0.9)),
                qtransform(-90, halfRoot(2), cv::Point2f(0, -0.9))
            } };

        rootNode.m_accum = util::transform3x3::getRotationMatrix2D(cv::Point2f(), 90, 1);
        rootNode.m_accum = util::transform3x3::getRotationMatrix2D(cv::Point2f(4.0f, 12.0f), 150.0, 7.0, 15.0f, 27.0f);
        break;

    case 2: // L reptiles
        polygon = { {
                {0,0},{2,0},{2,0.8},{0.8,0.8},{0.8,2},{0,2}
            } };

        transforms = { {
                qtransform(0, 0.5, cv::Point2f(0, 0)),
                qtransform(0, 0.5, cv::Point2f(0.5, 0.5)),
                qtransform(0, 0.5, cv::Point2f(1, 1)),
                qtransform(-90, 0.5, cv::Point2f(2, 0)),
                qtransform(90, 0.5, cv::Point2f(0, 2))
            } };

        rootNode.m_accum = util::transform3x3::getTranslate(-1, -1);
        break;

    case 3: // 1:r3:2 right triangle reptile
    {
        float r3 = sqrt(3.0f);

        polygon = { {
                {0.0f,0},{r3,0},{0,1}
            } };

        transforms = { {
                //          00  01   tx     10    11  ty
                qtransform(0.0f, r3 / 3, 0.0f,  r3 / 3,0.0f,0.0f,  util::colorSink(Matx41(9,0,3,1)*0.111f, 0.1f)),
                qtransform(-.5f,-r3 / 6, r3 / 2,  r3 / 6,-.5f, .5f,  util::colorSink(Matx41(3,0,1,1)*0.111f, 0.9f)),
                qtransform(.5f,-r3 / 6, r3 / 2, -r3 / 6,-.5f, .5f,  util::colorSink(Matx41(3,0,9,1)*0.111f, 0.1f))
            } };

    }
    break;

    case 4: // 1:2:r5 right triangle reptile
        polygon = { {
                {0,0},{2,0},{0,1}
            } };

        transforms = { {
                //          00  01  tx   10  11  ty
                qtransform(-.2,-.4, .4, -.4, .2, .8,  util::colorSink(Matx41(0,0,9,1)*0.111f, 0.1f)),
                qtransform(.4,-.2, .2, -.2,-.4, .4,  util::colorSink(Matx41(0,4,9,1)*0.111f, 0.1f)),
                qtransform(.4, .2, .2, -.2, .4, .4,  util::colorSink(Matx41(9,9,9,1)*0.111f, 0.1f)),
                qtransform(-.4,-.2,1.2,  .2,-.4, .4,  util::colorSink(Matx41(0,9,4,1)*0.111f, 0.1f)),
                qtransform(.4,-.2,1.2, -.2,-.4, .4,  util::colorSink(Matx41(0,0,0,1)*0.111f, 0.1f))
            } };

        break;

    case 5: // 5-stars
    {
        polygon.clear();
        float astep = 6.283f / 10.0f;
        float angle = 0.0f;
        for (int i = 0; i < 10; i += 2)
        {
            polygon.push_back(Point2f(sin(angle), cos(angle)));
            angle += astep;
            polygon.push_back(0.1f * Point2f(sin(angle), cos(angle)));
            angle += astep;
        }

        transforms = { {
                qtransform(util::transform3x3::getRotate(1 * astep) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(0,2,9,1)*0.111f, 0.7f)),
                qtransform(util::transform3x3::getRotate(3 * astep) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(0,4,9,1)*0.111f, 0.7f)),
                qtransform(util::transform3x3::getRotate(5 * astep) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(0,9,9,1)*0.111f, 0.7f)),
                qtransform(util::transform3x3::getRotate(7 * astep) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(6,0,9,1)*0.111f, 0.7f)),
                qtransform(util::transform3x3::getRotate(9 * astep) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(9,4,0,1)*0.111f, 0.7f))
            } };

        rootNode.m_color = Matx41(0.5f, 0.75f, 1, 1);

        m_boundingRect = cv::Rect_<float>(-2, -2, 4, 4);

        break;
    }

    case 6: // 6-stars
    {
        polygon.clear();
        float r32 = sqrt(3.0f)*0.5;
        float c[12] = { 1.0, r32, 0.5, 0.0,-0.5,-r32,
                        -1.0,-r32,-0.5, 0.0, 0.5, r32 };
        for (int i = 0; i < 12; i += 2)
        {
            polygon.push_back(Point2f(c[i], c[(i + 3) % 12]));
            polygon.push_back(0.1f * Point2f(c[(i + 1) % 12], c[(i + 4) % 12]));
        }

        float angle = 6.283f / 6.0f;
        transforms = { {
                qtransform(util::transform3x3::getRotate(0 * angle) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(0,2,9,1)*0.111f, 0.7f)),
                qtransform(util::transform3x3::getRotate(1 * angle) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(0,4,9,1)*0.111f, 0.7f)),
                qtransform(util::transform3x3::getRotate(2 * angle) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(0,9,9,1)*0.111f, 0.7f)),
                qtransform(util::transform3x3::getRotate(3 * angle) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(6,0,9,1)*0.111f, 0.7f)),
                qtransform(util::transform3x3::getRotate(4 * angle) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(9,4,0,1)*0.111f, 0.7f)),
                qtransform(util::transform3x3::getRotate(5 * angle) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(2,0,9,1)*0.111f, 0.7f))
            } };

        rootNode.m_color = Matx41(1, 1, 1, 1);

        m_boundingRect = cv::Rect_<float>(-2, -2, 4, 4);

        break;
    }

    case 7: // irregular quad
    {
        polygon = { {
                {0,0},{1,0},{1.0f,0.9f},{0.1f,0.8f}
            } };

        transforms = { {
                qtransform(util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[2], polygon[1]), util::colorSink(Matx41(0,0,9,1)*0.111f, 0.2f)),
                qtransform(util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[3], polygon[2]), util::colorSink(Matx41(0,9,0,1)*0.111f, 0.2f)),
                qtransform(util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[0], polygon[3]), util::colorSink(Matx41(9,0,0,1)*0.111f, 0.2f))
            } };

        rootNode.m_color = Matx41(1, 1, 1, 1);

        m_boundingRect = cv::Rect_<float>(-3, -3, 7, 7);

        break;
    }

    }   // switch (settings % NUM_PRESETS)

    offspringTemporalRandomness = 1000;

    if (settings >= NUM_PRESETS)
    {
        for (auto &t : transforms)
        {
            t.colorTransform = util::colorSink(util::randomColor(), util::r());
        }

        offspringTemporalRandomness = 1 + rand() % 500;
    }

    cout << "Settings changed: " << transforms.size() << " transforms, offspringTemporalRandomness: " << offspringTemporalRandomness << endl;

    // clear and initialize the queue with the seed

    util::clear(nodeQueue);
    nodeQueue.push(rootNode);
}


cv::Rect_<float> qtree::getBoundingRect() const
{
    if (m_boundingRect.width == 0)
    {
        vector<cv::Point2f> v;
        rootNode.getPolyPoints(polygon, v);
        return util::getBoundingRect(v);
    }

    return m_boundingRect;
}

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
    child.beginTime = parent.beginTime + 1 + rand() % offspringTemporalRandomness;

    child.m_accum = parent.m_accum * t.transformMatrix;

    auto m = t.colorTransform * parent.m_color;
    child.m_color = m.col(0);
}

void qtree::drawNode(qcanvas &canvas, qnode const &node)
{
    if (!isViable(node))
        return;

    Matx33 m = canvas.globalTransform * node.m_accum;

    vector<cv::Point2f> v;
    cv::transform(polygon, v, m.get_minor<2, 3>(0, 0));
    vector<vector<cv::Point> > pts(1);
    for (auto const& p : v)
        pts[0].push_back(p * 16);

    cv::Scalar color = util::toColor(node.m_color);
    cv::Scalar lineColor = cv::Scalar(255, 255, 255, 255);
    cv::fillPoly(canvas.image, pts, color, cv::LineTypes::LINE_AA, 4);
    //cv::polylines(canvas.image, pts, true, lineColor, 1, cv::LineTypes::LINE_8);
}


#pragma endregion



class The
{
public:
    cv::Mat loadedImage;
    qcanvas canvas;

    qtree tree;

    int minNodesProcessedPerFrame = 1;
    int maxNodesProcessedPerFrame = 64;

    int presetIndex = 7;


    void run()
    {
        tree.create(presetIndex);

        cv::Mat3b img(cv::Size(900, 900));
        img = 0;

        canvas.create(img);
        canvas.setScaleToFit(tree.getBoundingRect());

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

                int nodesProcessed = 0, nodesAdded = 0;
                while (!tree.nodeQueue.empty() 
                        && nodesProcessed < maxNodesProcessedPerFrame
                        //&& tree.nodeQueue.top().det() >= cutoff 
                        && (nodesProcessed < minNodesProcessedPerFrame || tree.nodeQueue.top().beginTime <= time)
                    )
                {
                    auto currentNode = tree.nodeQueue.top();

                    tree.drawNode(canvas, currentNode);

                    nodesProcessed++;
                    nodesAdded += tree.process();
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
            presetIndex = (++presetIndex) % (NUM_PRESETS * 2);
            tree.create(presetIndex);
            canvas.setScaleToFit(tree.getBoundingRect());
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
            presetIndex = (presetIndex + NUM_PRESETS) % (NUM_PRESETS * 2);
            tree.create(presetIndex);
            canvas.setScaleToFit(tree.getBoundingRect());
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