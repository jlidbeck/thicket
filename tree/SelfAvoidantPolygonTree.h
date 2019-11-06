#pragma once


#include "tree.h"
#include "util.h"
#include <random>
#include <vector>
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <array>


using std::cout;
using std::endl;


//  Limited-growth qtrees
//  These models limit growth by prohibiting self-intersection


class SelfAvoidantPolygonTree : public qtree
{
protected:
    // settings
    int maxRadius = 100;
    int polygonSides = 5;

    int fieldResolution = 10;   // pixels per unit

    // intersection field
    cv::Mat1b m_field;
    mutable cv::Mat1b m_fieldLayer;
    Matx33 m_fieldTransform;


public:

    SelfAvoidantPolygonTree() { }

    virtual void randomizeSettings(int randomize)
    {
        maxRadius = 100;
        polygonSides = 5;
        offspringTemporalRandomness = 1000;

        if (randomize)
        {
            maxRadius = 5 + rand() % 100;
            polygonSides = (randomize % 6) + 3;

            offspringTemporalRandomness = 1 + rand() % 200;
        }
    }

    virtual void create() override
    {
        // initialize intersection field
        m_field.create(maxRadius*2*fieldResolution, maxRadius*2*fieldResolution);
        m_field = 0;
        m_field.copyTo(m_fieldLayer);
        m_fieldTransform = util::transform3x3::getScaleTranslate(fieldResolution, maxRadius*fieldResolution, maxRadius*fieldResolution);

        // create regular polygon
        polygon.clear();
        float angle = -6.283 / (polygonSides * 2);
        for (int i = 0; i < polygonSides; i++)
        {
            polygon.push_back(Point2f(sin(angle), cos(angle)));
            angle += 6.283 / polygonSides;
        }

        // add edge transforms to map edge 0 to all other edges
        transforms.clear();
        for (int i = 1; i < polygonSides; ++i)
        {
            Matx41 hsv((float)i/(float)polygonSides, 1.0, 0.5), bgr(1.0, 0.5, 0.0);
            //cv::cvtColor(hsv, bgr, cv::ColorConversionCodes::COLOR_HSV2BGR);
            bgr = util::randomColor();
            transforms.push_back(
                qtransform(
                    util::transform3x3::getEdgeMap(polygon[polygonSides - 1], polygon[0], polygon[i], polygon[i - 1]),
                    util::colorSink(bgr, 0.7f))
            );
        }


        cout << "Settings changed: " << transforms.size() << " transforms, offspringTemporalRandomness: " << offspringTemporalRandomness << endl;

        // clear and initialize the queue with the seed

        qnode rootNode;
        createRootNode(rootNode);

        util::clear(nodeQueue);
        nodeQueue.push(rootNode);
    }

    virtual void createRootNode(qnode & rootNode)
    {
        rootNode.beginTime = 0;
        rootNode.generation = 0;
        rootNode.globalTransform = rootNode.globalTransform.eye();
        rootNode.color = cv::Scalar(1, 1, 1, 1);
    }


    virtual cv::Rect_<float> getBoundingRect() const override
    {
        return cv::Rect_<float>(-maxRadius, -maxRadius, 2 * maxRadius, 2 * maxRadius);
    }

    virtual bool isViable(qnode const &node) const override
    {
        if (!node) 
            return false;

        if (fabs(node.det()) < 0.01f)
            return false;

        // since the node polygon is centered at its local origin, its global
		// centroid is simply the translation from its transform matrix
        cv::Matx<float, 3, 1> center(node.globalTransform(0, 2), node.globalTransform(1, 2), 1.0f);

        if (center(0) < -maxRadius || center(0)>maxRadius || center(1) < -maxRadius || center(1)>maxRadius) 
            return false;

        // quick check field pixel at transformed node center
        auto fieldPt = m_fieldTransform * center;
        if (m_field.at<uchar>(fieldPt(1), fieldPt(0)))
            return false;

        drawField(node);
        thread_local cv::Mat andmat;
        cv::bitwise_and(m_field, m_fieldLayer, andmat);
        return(!cv::countNonZero(andmat));
    }

    //  draw node on field layer to detect collisions
    void drawField(qnode const &node) const
    {
        Matx33 m = m_fieldTransform * node.globalTransform;

        vector<cv::Point2f> v;
        cv::transform(polygon, v, m.get_minor<2, 3>(0, 0));
        vector<vector<cv::Point> > pts(1);
        for (auto const& p : v)
            pts[0].push_back(p);

        m_fieldLayer = 0;
        cv::fillPoly(m_fieldLayer, pts, cv::Scalar(255), cv::LineTypes::LINE_8);
        // reduce by drawing outline in black, since OpenCV fillPoly draws extra pixels along edge
        cv::polylines(m_fieldLayer, pts, true, cv::Scalar(0), 1);
    }

    // update field image: composite new node
    virtual void addNode(qnode &currentNode) override
    {
        cv::bitwise_or(m_field, m_fieldLayer, m_field);
    }

};


class ScaledPolygonTree : public SelfAvoidantPolygonTree
{
    double m_ratio;
    bool m_ambidextrous;

public:
    virtual void randomizeSettings(int randomize) override
    {
        SelfAvoidantPolygonTree::randomizeSettings(randomize);

        fieldResolution = 200;
        maxRadius = 3.5;
        
        // ratio: child size / parent size
        std::array<float, 5> ratioPresets = { {
            (sqrt(5.0f) - 1.0f) / 2.0f,        // phi
            0.5f,
            1.0f / 3.0f,
            1.0f / sqrt(2.0f),
            (sqrt(3.0f) - 1.0f) / 2.0f
        } };

        m_ratio = ratioPresets[randomize%ratioPresets.size()];
        m_ambidextrous = (randomize % 2);
    }

    virtual void create() override
    {
        SelfAvoidantPolygonTree::create();

        // override edge transforms
        transforms.clear();
        for (int i = 1; i < polygon.size(); ++i)
        {
            auto edgePt1 = m_ratio * polygon[i - 1] + (1.0f - m_ratio)*polygon[i    ];
            transforms.push_back(
                qtransform(
                    util::transform3x3::getEdgeMap(polygon[polygon.size() - 1], polygon[0], polygon[i], edgePt1),
                    util::colorSink(util::randomColor(), 0.5))
            );

            if (m_ambidextrous)
            {
                auto edgePt2 = m_ratio * polygon[i    ] + (1.0f - m_ratio)*polygon[i - 1];
                transforms.push_back(
                    qtransform(
                        util::transform3x3::getEdgeMap(polygon[polygon.size() - 1], polygon[0], edgePt2, polygon[i - 1]),
                        util::colorSink(util::randomColor(), 0.5))
                );
            }
        }

        cout << "Settings changed: polygon:"<<polygon.size()<<" transforms:" << transforms.size() 
            << " offspringTemporalRandomness: " << offspringTemporalRandomness << endl;
    }

};


class TrapezoidTree : public SelfAvoidantPolygonTree
{
public:
    virtual void randomizeSettings(int randomize) override
    {
        SelfAvoidantPolygonTree::randomizeSettings(randomize);

        fieldResolution = 200;
        maxRadius = 10;
        offspringTemporalRandomness = 10;
    }

    virtual void create() override
    {
        SelfAvoidantPolygonTree::create();

        // override polygon
        polygon = { { { -0.5f, -0.5f}, {0.5f, -0.5f}, {0.4f, 0.4f}, {-0.5f, 0.45f} } };

        //auto m = util::transform3x3::getMirroredEdgeMap(cv::Point2f( 0.0f,0.0f ), cv::Point2f( 1.0f,0.0f ), cv::Point2f( 0.0f,0.0f ), cv::Point2f( 1.0f,0.0f ));

        int steps = 24;
        float angle = 6.283f / steps;
        float r0 = 0.5f;
        float r1 = 1.0f;
        float growthFactor = pow(r1 / r0, 2.0f / steps);
        polygon = { { {r0,0}, {r1,0}, {r1*growthFactor*cos(angle),r1*growthFactor*sin(angle)}, {r0*growthFactor*cos(angle),r0*growthFactor*sin(angle)} } };

        // override edge transforms
        transforms.clear();
        //for (int i = 1; i < polygon.size(); ++i)
        //{
        //    transforms.push_back(
        //        qtransform(
        //            util::transform3x3::getMirroredEdgeMap(polygon[0], polygon[1], polygon[i], polygon[(i+1)%polygon.size()]),
        //            util::colorSink(util::randomColor(), 0.5f))
        //    );
        //}

        transforms.push_back(
            qtransform(
                util::transform3x3::getMirroredEdgeMap(polygon[0], polygon[1], polygon[1], polygon[2]),
                util::colorSink(util::randomColor(), 0.5))
        );
        transforms.push_back(
            qtransform(
                util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[3], polygon[2]),
                util::colorSink(util::randomColor(), 0.5))
        );
        transforms.push_back(
            qtransform(
                util::transform3x3::getMirroredEdgeMap(polygon[0], polygon[1], polygon[3], polygon[0]),
                util::colorSink(util::randomColor(), 0.5))
        );

        //transforms[0].gestation = 1111.1;
        transforms[0].gestation = 6.2;
        transforms[1].gestation = 1.0;
        transforms[2].gestation = 7.3;

        //transforms[0].colorTransform = util::colorSink(1.0f,1.0f,1.0f, 0.5f);
        //transforms[0].colorTransform = util::colorSink(0.0f,0.5f,0.0f, 0.8f);
        //transforms[1].colorTransform = util::colorSink(0.5f,1.0f,1.0f, 0.3f);
        //transforms[2].colorTransform = util::colorSink(0.9f,0.5f,0.0f, 0.8f);



        cout << "Settings changed: TrapezoidTree:" << polygon.size() << " transforms:" << transforms.size()
            << " offspringTemporalRandomness: " << offspringTemporalRandomness << endl;
    }

    virtual void createRootNode(qnode & rootNode) override
    {
        rootNode.color = cv::Scalar(0.2, 0.5, 0, 1);
    }

};


cv::Point2f headingStep(float angleDegrees)
{
    float a = 3.14159265f*angleDegrees / 180.0f;
    
    return cv::Point2f(cos(a), sin(a));
}

class ThornTree : public SelfAvoidantPolygonTree
{
public:
    virtual void randomizeSettings(int randomize) override
    {
        SelfAvoidantPolygonTree::randomizeSettings(randomize);

        fieldResolution = 200;
        maxRadius = 20;
        offspringTemporalRandomness = 100;
    }

    virtual void create() override
    {
        SelfAvoidantPolygonTree::create();

        // override polygon
        polygon.clear();
        cv::Point2f pt(0, 0);
        polygon.push_back(pt);
        polygon.push_back(pt += headingStep(0));
        polygon.push_back(pt += headingStep(120));
        polygon.push_back(pt += headingStep(105));
        polygon.push_back(pt += headingStep(90));
        polygon.push_back(pt += headingStep(75));
        polygon.push_back(pt += headingStep(240));
        polygon.push_back(pt += headingStep(255));
        polygon.push_back(pt += headingStep(270));

        // override edge transforms
        transforms.clear();
        auto ct1 = util::colorSink(util::hsv2bgr(66.0, 1.0, 0.5), 0.3);
        auto ct2 = util::colorSink(util::hsv2bgr(192.0, 1.0, 0.5), 0.3);
        for (int i = 0; i < polygon.size(); ++i)
        {
            for (int j = 0; j < polygon.size(); ++j)
            {
                transforms.push_back(
                    qtransform(
                        util::transform3x3::getEdgeMap(polygon[i], polygon[(i + 1) % polygon.size()], polygon[(j + 1) % polygon.size()], polygon[j]),
                        ct1)
                );
                transforms.push_back(
                    qtransform(
                        util::transform3x3::getMirroredEdgeMap(polygon[i], polygon[(i + 1) % polygon.size()], polygon[j], polygon[(j + 1) % polygon.size()]),
                        ct2)
                );
            }
        }

        //transforms[0].gestation = 1111.1;

        //transforms[0].colorTransform = util::colorSink(1.0f,1.0f,1.0f, 0.5f);
        //transforms[0].colorTransform = util::colorSink(0.0f,0.5f,0.0f, 0.8f);
        //transforms[1].colorTransform = util::colorSink(0.5f,1.0f,1.0f, 0.3f);
        //transforms[2].colorTransform = util::colorSink(0.9f,0.5f,0.0f, 0.8f);



        cout << "Settings changed: ThornTree:" << polygon.size() << " transforms:" << transforms.size()
            << " offspringTemporalRandomness: " << offspringTemporalRandomness << endl;
    }

    virtual void createRootNode(qnode & rootNode) override
    {
        rootNode.color = cv::Scalar(1,1,1, 1);
    }

};
