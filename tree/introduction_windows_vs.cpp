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


class qnode
{
public:
    int begin;
    int generation;
    cv::Mat1f m_accum;

    qnode(int time = 0) {
        begin = time;

        m_accum.create(3, 3);
        cv::setIdentity(m_accum);
        generation = 0;
    }

    inline float det() const { return (m_accum.at<float>(0, 0) * m_accum.at<float>(0, 0) + m_accum.at<float>(1, 0) * m_accum.at<float>(1, 0)); }

    bool operator!() const { return (det() < 0.01); }

    void draw(cv::Mat& image)
    {
        cv::Mat1f global = cv::Mat1f::eye(3, 3);
        global.at<float>(0, 2) = 500;
        global.at<float>(1, 2) = 500;
        cv::Mat1f m = global * m_accum;

        vector<cv::Point2f> v = { { {-20,0}, {20,0}, {20,100}, {-20,100} } };
        cv::transform(v, v, m(cv::Range(0, 2), cv::Range::all()));
        vector<vector<cv::Point> > pts(1);
        for (auto const& p : v)
            pts[0].push_back(p);

        cv::fillPoly(image, pts, cv::Scalar_<uint8_t>(255, 192, 128, 255), cv::LineTypes::LINE_8, 0);
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
        m = Mat1f::eye(3, 3);
        m(cv::Range(0, 2), cv::Range::all()) = cv::getRotationMatrix2D(Point2f(1.0, 0.0), angle, scale);
        m.at<float>(0, 2) = translate.x;
        m.at<float>(1, 2) = translate.y;
    }

    void transform(qnode const& in, qnode& out)
    {
        out.generation = in.generation + 1;
        out.m_accum = m * in.m_accum;
    }
};

class The
{
public:
    Mat loadedImage;
    Mat image;

    priority_queue<qnode, vector<qnode>, qnode::BiggestFirst> treeQueue;

    void run()
    {
        qnode rootNode;
        treeQueue.push(rootNode);

        qtransform  t1( 30, 0.94387431268, cv::Point2f(0, 0)),
                    t2(-30, 0.707, cv::Point2f(0, 120));

        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> dis(0, the.image.cols - 50);
        //cv::Rect rect(0, 0, 50, 50);

        int time = 0;

        while(!treeQueue.empty())
        {
            ++time;

            auto cutoff = treeQueue.top().det();

            while (!treeQueue.empty() && treeQueue.top().det() >= cutoff)
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


                currentNode.draw(image);
            }

            image *= 0.99;

            // update display
            imshow("Memtest", image); // Show our image inside it.


            if (waitKey(1) != -1)
                return;
        }
    }

    int load(fs::path imagePath)
    {
        imagePath = fs::absolute(imagePath);
        loadedImage = imread(imagePath.string(), IMREAD_COLOR); // Read the file
        image = loadedImage;

        if (!loadedImage.data) // Check for invalid input
        {
            cout << "Could not open or find the image " << imagePath << std::endl;
            return -1;
        }

        return 0;
    }

} the;

static void onMouse(int event, int x, int y, int, void*)
{
    if (event != EVENT_LBUTTONDOWN)
        return;
    Point seed = Point(x, y);

    the.image = the.loadedImage;
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