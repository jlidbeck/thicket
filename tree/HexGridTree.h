#pragma once


#include "tree.h"
#include "util.h"
#include <random>
#include <vector>
#include <iostream>
#include <opencv2/imgproc.hpp>


using std::cout;
using std::endl;



class SelfAvoidantPolygonTree : public qtree
{
protected:
    cv::Mat1b m_field;
    mutable cv::Mat1b m_fieldLayer;
    Matx33 m_fieldTransform;

    int fieldResolution = 10;
    int maxRadius = 100;

    int polygonSides = 5;

public:

    SelfAvoidantPolygonTree() { }

    virtual void setDefaultSettings(int randomize)
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

    virtual void create(int randomizeSettings) override
    {
        setDefaultSettings(randomizeSettings);

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

        rootNode.m_color = Matx41(1, 1, 1, 1);


        cout << "Settings changed: " << transforms.size() << " transforms, offspringTemporalRandomness: " << offspringTemporalRandomness << endl;

        // clear and initialize the queue with the seed

        util::clear(nodeQueue);
        nodeQueue.push(rootNode);
    }


    virtual cv::Rect_<float> getBoundingRect() const override
    {
            //vector<cv::Point2f> v;
            //rootNode.getPolyPoints(polygon, v);
            //return util::getBoundingRect(v);

        float r = maxRadius;
        return cv::Rect_<float>(-r, -r, 2 * r, 2 * r);
    }

    virtual bool isViable(qnode const &node) const override
    {
        if (!node) 
            return false;

        // since the node polygon is centered at its local origin, its global
		// centroid is simply the translation from its transform matrix
        cv::Matx<float, 3, 1> center(node.m_accum(0, 2), node.m_accum(1, 2), 1.0f);

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
        Matx33 m = m_fieldTransform * node.m_accum;

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


class BlockTree : public SelfAvoidantPolygonTree
{
public:
    virtual void setDefaultSettings(int randomize) override
    {
        SelfAvoidantPolygonTree::setDefaultSettings(randomize);

        fieldResolution = 200;
        maxRadius = 3.5;
    }

    virtual void create(int randomizeSettings) override
    {
        SelfAvoidantPolygonTree::create(randomizeSettings);


        // override edge transforms
        transforms.clear();
        const float phi = (sqrt(5.0f) - 1.0f) / 2.0f;
        for (int i = 1; i < polygon.size(); ++i)
        {
            auto edgePt1 = phi * polygon[i - 1] + (1.0f - phi)*polygon[i];
            auto edgePt2 = phi * polygon[i] + (1.0f - phi)*polygon[i - 1];
            transforms.push_back(
                qtransform(
                    util::transform3x3::getEdgeMap(polygon[polygon.size() - 1], polygon[0], polygon[i], edgePt1),
                    util::colorSink(util::randomColor(), 0.5f))
            );
            transforms.push_back(
                qtransform(
                    util::transform3x3::getEdgeMap(polygon[polygon.size() - 1], polygon[0], edgePt2, polygon[i - 1]),
                    util::colorSink(util::randomColor(), 0.5f))
            );
        }

        rootNode.m_color = Matx41(1, 1, 1, 1);


        cout << "Settings changed: polygon:"<<polygon.size()<<" transforms:" << transforms.size() 
            << " offspringTemporalRandomness: " << offspringTemporalRandomness << endl;
    }

};
