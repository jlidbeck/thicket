#include "util.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <filesystem>
#include <random>
#include <queue>
#include <vector>

namespace fs = std::filesystem;


using namespace cv;
using namespace std;


class qcanvas
{
public:
    cv::Mat1f globalTransform;
    cv::Mat image;

    qcanvas(cv::Mat image) {
        create(image);
    }

    qcanvas() {
    }

    void create(cv::Mat im) {
        image = im;

        int range = 2;  // show -range .. range in each dimension
        float scale = std::min(image.cols / 2 / range, image.rows / 2 / range);

        // scale, flip vertical, offset
        globalTransform = cv::Mat1f::eye(3, 3);
        globalTransform = (Mat1f(3, 3) <<
            scale, 0, image.cols / 2,
            0, -scale, image.rows / 2,
            0, 0, 1);
    }
};


class qnode
{
public:
    int begin;
    int generation;
    cv::Mat1f m_accum;

    qnode(int time = 0) {
        begin = time;

        m_accum = cv::Mat1f::eye(3, 3);
        generation = 0;
    }

    inline float det() const { return (m_accum.at<float>(0, 0) * m_accum.at<float>(0, 0) + m_accum.at<float>(1, 0) * m_accum.at<float>(1, 0)); }

    bool operator!() const { return (det() < 1e-6); }

    void draw(qcanvas &canvas)
    {
        cv::Mat1f m = canvas.globalTransform * m_accum;

        vector<cv::Point2f> v = { { 
            { .1,-1}, { .08,0}, { .1, 1}, 
            {-.1, 1}, {-.08,0}, {-.1,-1}
            } };
        cv::transform(v, v, m(cv::Range(0, 2), cv::Range::all()));
        vector<vector<cv::Point> > pts(1);
        for (auto const& p : v)
            pts[0].push_back(p);

        cv::fillPoly(canvas.image, pts, cv::Scalar_<uint8_t>(255, 192, 128, 255), cv::LineTypes::LINE_8, 0);
        //cv::fillPoly(image, v, cv::Scalar_<uint8_t>(255, 255, 255, 255), -1, 0);
    }

    struct OldestFirst
    {
        bool operator()(qnode const& a, qnode const& b)
        {
            return (a.begin > b.begin);
        }
    };

    struct BiggestFirst
    {
        bool operator()(qnode const& a, qnode const& b)
        {
            return (b.det() > a.det());
        }
    };
};


class qtransform
{
public:
    cv::Mat1f m;

    qtransform(double angle, double scale, cv::Point2f translate)
    {
        m = transform3x3::getRotationMatrix2D(Point2f(), angle, scale, translate.x, translate.y);
    }

    void transform(qnode const& in, qnode& out)
    {
        out.generation = in.generation + 1;
        out.m_accum = m * in.m_accum;
    }
};

#define Half12  0.94387431268

//  (1/2)^(1/n)
//  growth factor with a halflife of {n}
float halfRoot(int n)
{
    return pow(0.5f, 1.0f / (float)n);
}

class The
{
public:
    Mat loadedImage;
    qcanvas canvas;

    priority_queue<qnode, vector<qnode>, qnode::BiggestFirst> treeQueue;

    int minNodesProcessedPerFrame = 100;
    int maxNodesProcessedPerFrame = 1024;

    void run()
    {
        canvas.create(canvas.image);

        qnode rootNode;
        //rootNode.m_accum = transform3x3::getRotationMatrix2D(cv::Point2f(), 90, 1);
        treeQueue.push(rootNode);

        //qtransform  t1( 30, halfRoot(6), cv::Point2f(0, 90)),
        //            t2(-30, halfRoot(6), cv::Point2f(0, 60));

        qtransform  t1( 90, halfRoot(2), cv::Point2f(0,  0.9)),
                    t2(-90, halfRoot(2), cv::Point2f(0, -0.9));

        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> dis(0, canvas.image.cols - 50);
        //cv::Rect rect(0, 0, 50, 50);

        int time = 0;

        while(!treeQueue.empty())
        {
            ++time;

            auto cutoff = treeQueue.top().det();

            int nodesProcessed = 0;
            while (!treeQueue.empty() && //nodesProcessed < minNodesProcessedPerFrame)
                                         //treeQueue.top().det() >= cutoff && nodesProcessed < maxNodesProcessedPerFrame)
                                         treeQueue.top().generation*10<time && nodesProcessed < maxNodesProcessedPerFrame)
            {
                auto currentNode = treeQueue.top();
                treeQueue.pop();

                qnode child1(time), child2(time);
                t1.transform(currentNode, child1);
                t2.transform(currentNode, child2);

                if (!!child1) treeQueue.push(child1);
                if (!!child2) treeQueue.push(child2);

                //rect.x = dis(gen);
                //rect.y = dis(gen);
                //cv::rectangle(image, rect, cv::Scalar_<uint8_t>(255, 192, 128, 255), -1, cv::LineTypes::LINE_8, 0);
                //cv::rectangle(image, rect, cv::Scalar_<uint8_t>(255, 255, 255, 255), 1, cv::LineTypes::LINE_8, 0);

                ++nodesProcessed;

                currentNode.draw(canvas);
            }

            canvas.image *= 0.9;

            // update display
            imshow("Memtest", canvas.image); // Show our image inside it.


            if (waitKey(1) != -1)
                return;
        }
    }

    int load(fs::path imagePath)
    {
        imagePath = fs::absolute(imagePath);
        loadedImage = imread(imagePath.string(), IMREAD_COLOR); // Read the file
        if (!loadedImage.data) // Check for invalid input
        {
            cout << "Could not open or find the image " << imagePath << std::endl;
            return -1;
        }

        //canvas.image = loadedImage;

        cv::Mat3b img(cv::Size(1200, 900));
        canvas.create(img);

        return 0;
    }

} the;

static void onMouse(int event, int x, int y, int, void*)
{
    if (event != EVENT_LBUTTONDOWN)
        return;
    Point seed = Point(x, y);

    the.canvas.image = the.loadedImage;
}


int main( int argc, char** argv )
{
    if( argc != 2)
    {
        cout <<" Usage: display_image ImageToLoadAndDisplay" << endl;
        return -1;
    }


    namedWindow("Memtest", WINDOW_AUTOSIZE); // Create a window for display.

    setMouseCallback("image", onMouse, 0);


    //try {
        the.load(argv[1]);
        the.run();
    //}
    //catch (cv::Exception ex)
    //{
    //    cerr << "cv exception: " << ex.what();
    //    return -1;
    //}

    return 0;
}