#pragma once


#include "tree.h"
#include "util.h"
#include <vector>
#include <iostream>
#include <unordered_set>


using std::cout;
using std::endl;


#define Half12  0.94387431268

//  (1/2)^(1/n)
//  growth factor with a halflife of {n}
float halfRoot(int n)
{
    return pow(0.5f, 1.0f / (float)n);
}


class ReptileTree : public qtree
{
private:
    // settings

    qnode m_rootNode;

public:
    static const int NUM_PRESETS = 8;

public:

    virtual void setRandomSeed(int seed) override
    {
        qtree::setRandomSeed(seed);

        bool randomizeOnCreate = (randomSeed >= NUM_PRESETS);

        switch (randomSeed % NUM_PRESETS)
        {
        case 0:
            // H hourglass
            polygon = { {
                { .1f,-1}, { .08f,0}, { .1f, 1},
                {-.1f, 1}, {-.08f,0}, {-.1f,-1}
                } };

            transforms = { {
                    qtransform( 90.0, halfRoot(2), cv::Point2f(0,  0.9)),
                    qtransform(-90.0, halfRoot(2), cv::Point2f(0, -0.9))
                } };

            m_rootNode.globalTransform = util::transform3x3::getRotationMatrix2D(cv::Point2f(), 90, 1);

            break;

        case 1:
            polygon = { {
                    // L
                    {0.0f,0},{0.62f,0},{0.62f,0.3f},{0.3f,0.3f},{0.3f,1},{0,1}
                } };

            transforms = { {
                    qtransform( 90.0, halfRoot(2), cv::Point2f(0,  0.9f)),
                    qtransform(-90.0, halfRoot(2), cv::Point2f(0, -0.9f))
                } };

            m_rootNode.globalTransform = util::transform3x3::getRotationMatrix2D(cv::Point2f(), 90, 1);
            m_rootNode.globalTransform = util::transform3x3::getRotationMatrix2D(cv::Point2f(4.0f, 12.0f), 150.0, 7.0, 15.0f, 27.0f);
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

            m_rootNode.globalTransform = util::transform3x3::getTranslate(-1, -1);
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
                    qtransform{ util::transform3x3::getRotate(1 * astep) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(0,2,9,1)*0.111f, 0.7f) },
                    qtransform{ util::transform3x3::getRotate(3 * astep) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(0,4,9,1)*0.111f, 0.7f) },
                    qtransform{ util::transform3x3::getRotate(5 * astep) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(0,9,9,1)*0.111f, 0.7f) },
                    qtransform{ util::transform3x3::getRotate(7 * astep) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(6,0,9,1)*0.111f, 0.7f) },
                    qtransform{ util::transform3x3::getRotate(9 * astep) * util::transform3x3::getScaleTranslate(0.5f, 0.0f, 1.0f), util::colorSink(Matx41(9,4,0,1)*0.111f, 0.7f) }
                } };

            m_rootNode.color = cv::Scalar(0.5f, 0.75f, 1, 1);

            break;
        }

        case 6: // 6-stars
        {
            polygon.clear();
            float r32 = sqrt(3.0f)*0.5f;
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

            m_rootNode.color = cv::Scalar(1, 1, 1, 1);

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

            m_rootNode.color = cv::Scalar(1, 1, 1, 1);

            break;
        }

        }   // switch (settings % NUM_PRESETS)

        offspringTemporalRandomness = 0;

        if (randomizeOnCreate)
        {
            for (auto &t : transforms)
            {
                t.colorTransform = util::colorSink(randomColor(), r());
                t.gestation = 1 + r(500.0);
            }

            offspringTemporalRandomness = 1 + r(100.0);
        }
    }

    virtual void create() override
    {
        m_rootNode.color = cv::Scalar(0.5, 0.5, 0.5, 1);
        m_rootNode.globalTransform = Matx33::eye();

        // clear and initialize the queue with the seed

        util::clear(nodeQueue);
        nodeQueue.push(m_rootNode);
    }

    virtual void to_json(json &j) const override
    {
	    qtree::to_json(j);

        j["_class"] = "ReptileTree";
    }

    virtual void from_json(json const &j) override
    {
        try {
            qtree::from_json(j);
        }
        catch (std::exception &ex)
        {
            // let's ignore exceptions for now
            randomSeed = 42;
        }
    }

    virtual cv::Rect_<float> getBoundingRect() const override
    {
        switch (randomSeed % NUM_PRESETS)
        {
        case 0: return cv::Rect_<float>(-2, -2, 4, 4);
        case 5: return cv::Rect_<float>(-2, -2, 4, 4);
        case 6: return cv::Rect_<float>(-2, -2, 4, 4);
        case 7: return cv::Rect_<float>(-3, -3, 7, 7);
        }

        // true reptiles don't grow outside the initial bounds
        vector<cv::Point2f> v;
        m_rootNode.getPolyPoints(polygon, v);
        return util::getBoundingRect(v);
    }
};

REGISTER_QTREE_TYPE(ReptileTree);
