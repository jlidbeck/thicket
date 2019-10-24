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


using std::cout;
using std::endl;
using std::vector;
using std::priority_queue;

using cv::Point2f;
typedef cv::Matx<float, 3, 3> Matx33;
typedef cv::Matx<float, 4, 1> Matx41;
typedef cv::Matx<float, 4, 4> Matx44;


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

    // sets global transform map to show -range .. range in each dimension
    void setScaleToFit(int range)
    {
        if (image.empty()) throw std::exception("Image is empty");

        float halfw = (float)(image.cols / 2);
        float halfh = (float)(image.rows / 2);
        float scale = std::min(halfw / range, halfh / range);

        // scale, flip vertical, offset
        globalTransform = Matx33(
            scale,  0,     halfw,
            0,     -scale, halfh,
            0,      0,     1);
    }

    // sets global transform map to show -range .. range in each dimension
    void setScaleToFit(cv::Rect_<float> const &rect)
    {
        if (image.empty()) throw std::exception("Image is empty");

		globalTransform = util::transform3x3::centerAndFit(rect, cv::Rect_<float>(0, 0, image.cols, image.rows));
    }

};


class qtransform
{
public:
    Matx33 m_transformMatrix;
    Matx44 m_colorTransform;

public:

    qtransform(Matx33 const &cvMat = Matx33::eye(), Matx44 const &cvColor = Matx44::eye())
    {
        m_transformMatrix = cvMat;
        m_colorTransform = cvColor;
    }

    template<typename _Tp>
    qtransform(_Tp m00, _Tp m01, _Tp mtx, _Tp m10, _Tp m11, _Tp mty, Matx44 const &colorTransform)
    {
        m_transformMatrix = Matx33(m00, m01, mtx, m10, m11, mty, 0, 0, 1);
        m_colorTransform = colorTransform;
        //Matx44(
        //0.1, 0.9, 0,   0,
        //0,   0.1, 0.9, 0,
        //0.9, 0,   0.1, 0,
        //0,   0,   0,   1);
    }

    qtransform(double angle, double scale, cv::Point2f translate)
    {
        m_transformMatrix = util::transform3x3::getRotationMatrix2D(Point2f(), angle, scale, translate.x, translate.y);
        m_colorTransform = m_colorTransform.eye();
        m_colorTransform(2, 2) = 0.94;
        m_colorTransform(1, 1) = 0.96;
    }
};


class qnode
{
public:
    double beginTime;
    int generation;
    Matx33 m_accum;
    Matx41 m_color = Matx41(1,0,0,1);

    qnode(double beginTime_ = 0) 
    {
        beginTime = beginTime_;

        m_accum = m_accum.eye();
        generation = 0;
    }

    inline float det() const { return (m_accum(0, 0) * m_accum(0, 0) + m_accum(1, 0) * m_accum(1, 0)); }

    bool operator!() const { return (det() < 1e-6); }

    void draw(qcanvas &canvas, vector<cv::Point2f> const &points)
    {
        Matx33 m = canvas.globalTransform * m_accum;

        vector<cv::Point2f> v;
        cv::transform(points, v, m.get_minor<2, 3>(0,0));
        vector<vector<cv::Point> > pts(1);
        for (auto const& p : v)
            pts[0].push_back(p*16);

        cv::Scalar color(255.0 * m_color(0), 255.0 * m_color(1), 255.0 * m_color(2), 255.0 * m_color(3));
        cv::Scalar lineColor = cv::Scalar(255, 255, 255, 255);
        cv::fillPoly(canvas.image, pts, color, cv::LineTypes::LINE_AA, 4);
        //cv::polylines(canvas.image, pts, true, lineColor, 1, cv::LineTypes::LINE_8);
    }

    struct EarliestFirst
    {
        bool operator()(qnode const& a, qnode const& b)
        {
            return (a.beginTime > b.beginTime);
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


#define Half12  0.94387431268

//  (1/2)^(1/n)
//  growth factor with a halflife of {n}
float halfRoot(int n)
{
    return pow(0.5f, 1.0f / (float)n);
}


#define NUM_PRESETS 6

class qtree
{
public:
    vector<cv::Point2f> polygon;
    vector<qtransform> transforms;
    int offspringTemporalRandomness = 100;

    qnode rootNode;

    priority_queue<qnode, vector<qnode>, qnode::EarliestFirst> nodeQueue;


    qtree()
    {
    }

    void create(int settings)
    {
        switch (settings % NUM_PRESETS)
        {
        case 0:
            // hourglass
            polygon = { {
                { .1,-1}, { .08,0}, { .1, 1}, 
                {-.1, 1}, {-.08,0}, {-.1,-1}
                } };

            transforms = { {
                    qtransform( 90, halfRoot(2), cv::Point2f(0,  0.9)),
                    qtransform(-90, halfRoot(2), cv::Point2f(0, -0.9))
                } };

            rootNode.m_accum = util::transform3x3::getRotationMatrix2D(cv::Point2f(), 90, 1);
            break;

        case 1:
            polygon = { {
                    // L
                    {0,0},{0.62,0},{0.62,0.3},{0.3,0.3},{0.3,1},{0,1}
                } };

            transforms = { {
                    qtransform(90, halfRoot(2), cv::Point2f(0,  0.9)),
                    qtransform(-90, halfRoot(2), cv::Point2f(0, -0.9))
                } };

            rootNode.m_accum = util::transform3x3::getRotationMatrix2D(cv::Point2f(), 90, 1);
            break;

        case 2: // L reptiles
            polygon = { {
                    {0,0},{2,0},{2,0.8},{0.8,0.8},{0.8,2},{0,2}
                } };
            
            transforms = { {
                    qtransform(  0, 0.5, cv::Point2f(0, 0)),
                    qtransform(  0, 0.5, cv::Point2f(0.5, 0.5)),
                    qtransform(  0, 0.5, cv::Point2f(1, 1)),
                    qtransform(-90, 0.5, cv::Point2f(2, 0)),
                    qtransform( 90, 0.5, cv::Point2f(0, 2))
                } };

            rootNode.m_accum = util::transform3x3::getTranslate(-1, -1);
            break;

        case 3: // 1:r3:2 right triangle reptile
        {
            float r3 = sqrt(3.0);

            polygon = { {
                    {0.0f,0},{r3,0},{0,1}
                } };

            transforms = { {
                    //          00  01   tx     10    11  ty
                    qtransform(0.0f, r3/3, 0.0f,  r3/3,0.0f,0.0f,  util::colorSink(Matx41(9,0,3,1)*0.111f, 0.1f)),
                    qtransform(-.5f,-r3/6, r3/2,  r3/6,-.5f, .5f,  util::colorSink(Matx41(3,0,1,1)*0.111f, 0.9f)),
                    qtransform( .5f,-r3/6, r3/2, -r3/6,-.5f, .5f,  util::colorSink(Matx41(3,0,9,1)*0.111f, 0.1f))
                } };

            rootNode.m_color = Matx41(0.5, 0.5, 0.5, 1);
            rootNode.m_accum = util::transform3x3::getTranslate(-1.0, -0.5);
        }
        break;

        case 4: // 1:2:r5 right triangle reptile
            polygon = { {
                    {0,0},{2,0},{0,1}
                } };

            transforms = { {
                    //          00  01  tx   10  11  ty
                    qtransform(-.2,-.4, .4, -.4, .2, .8,  util::colorSink(Matx41(0,0,9,1)*0.111f, 0.1f)),
                    qtransform( .4,-.2, .2, -.2,-.4, .4,  util::colorSink(Matx41(0,4,9,1)*0.111f, 0.1f)),
                    qtransform( .4, .2, .2, -.2, .4, .4,  util::colorSink(Matx41(9,9,9,1)*0.111f, 0.1f)),
                    qtransform(-.4,-.2,1.2,  .2,-.4, .4,  util::colorSink(Matx41(0,9,4,1)*0.111f, 0.1f)),
                    qtransform( .4,-.2,1.2, -.2,-.4, .4,  util::colorSink(Matx41(0,0,0,1)*0.111f, 0.1f))
                } };

            rootNode.m_color = Matx41(0.5, 0.5, 0.5, 1);
            rootNode.m_accum = util::transform3x3::getTranslate(-1.0, -0.5);
            break;

        case 5: // 6-star
        {
            polygon.clear();
            double r32 = sqrt(3.0)*0.5;
            float c[12] = {  1.0, r32, 0.5, 0.0,-0.5,-r32,
                            -1.0,-r32,-0.5, 0.0, 0.5, r32 };
            for (int i = 0; i < 12; i += 2)
            {
                polygon.push_back(     Point2f(c[i], c[(i + 3) % 12]));
                polygon.push_back(0.1f*Point2f(c[(i + 1) % 12], c[(i + 4) % 12]));
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

            rootNode.m_color = Matx41(1,1,1, 1);

            break;
        }

        }   // switch (settings % NUM_PRESETS)

        offspringTemporalRandomness = 100;

        if (settings >= NUM_PRESETS)
        {
            for (auto &t : transforms)
            {
                t.m_colorTransform = util::colorSink(Matx41(util::r(), util::r(), util::r(), 1), util::r());
            }

            offspringTemporalRandomness = rand() % 200;
        }

        cout << "Settings changed: " << transforms.size() << " transforms, offspringTemporalRandomness: " << offspringTemporalRandomness << endl;

        // clear and initialize the queue with the seed

        while (!nodeQueue.empty()) nodeQueue.pop();
        nodeQueue.push(rootNode);
    }


    virtual cv::Rect_<float> getBoundingRect() const
    {
        return util::getBoundingRect(polygon);
    }


    bool process()
    {
        auto currentNode = nodeQueue.top();
        nodeQueue.pop();

        double miniTimeStep = 1.0 / transforms.size();

        for (auto const &t : transforms)
        {
            qnode child;
            beget(currentNode, t, child);
            child.beginTime += miniTimeStep;

            if (!!child) nodeQueue.push(child);
        }

        return true;
    }

    void beget(qnode const & parent, qtransform const & t, qnode & child)
    {
        child.generation = parent.generation + 1;
        child.beginTime = parent.beginTime + 1 + rand() % offspringTemporalRandomness;

        child.m_accum = parent.m_accum * t.m_transformMatrix;

        auto m = t.m_colorTransform * parent.m_color;
        child.m_color = m.col(0);
    }

};







class The
{
public:
    cv::Mat loadedImage;
    qcanvas canvas;
    qtree tree;

    int minNodesProcessedPerFrame = 100;
    int maxNodesProcessedPerFrame = 64;

    void run()
    {
        tree.create(5);

        cv::Mat3b img(cv::Size(2000, 1000));
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
            int time = 0;

            while (!tree.nodeQueue.empty())
            {
                ++time;

                auto cutoff = tree.nodeQueue.top().det();

                int nodesProcessed = 0;
                while (!tree.nodeQueue.empty() && //nodesProcessed < minNodesProcessedPerFrame)
                                             //tree.nodeQueue.top().det() >= cutoff && nodesProcessed < maxNodesProcessedPerFrame)
                    tree.nodeQueue.top().beginTime <= time && nodesProcessed < maxNodesProcessedPerFrame)
                {
                    // todo: draw all nodes
                    auto currentNode = tree.nodeQueue.top();
                    currentNode.draw(canvas, tree.polygon);

                    if (!tree.process()) break;

                    ++nodesProcessed;
                }

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
            static int i = 0;
            canvas.image = 0;
            tree.create((++i) % (NUM_PRESETS*2));
            canvas.setScaleToFit(tree.getBoundingRect());
            return true;
        }

        case 27:    // ESC
        default:
            return false;
        }
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