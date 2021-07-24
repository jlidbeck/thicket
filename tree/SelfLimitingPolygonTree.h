#pragma once


#include "tree.h"
#include "util.h"
#include <vector>
#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <array>
#include <list>
#include <unordered_set>
#include <filesystem>


namespace fs = std::filesystem;


using std::cout;
using std::endl;

// Ignore warnings about double to float conversion
#pragma warning (disable : 4305)
#pragma warning (disable : 4244)


//  Limited-growth qtrees
//  These models limit growth by prohibiting self-intersection


#pragma region SelfLimitingPolygonTree

class SelfLimitingPolygonTree : public qtree
{
protected:
    // --- settings ---

    int polygonSides = 5;
    float starAngle = 0;
    cv::Scalar rootNodeColor = cv::Scalar(0.0, 0.0, 1.0, 1.0);

    std::vector<cv::Point2f> drawPolygon;

    fs::path fieldImagePath;

    // controls size of intersection field--in pixels per model unit, independent of display resolution.
    int fieldResolution = 40;

    // minimum size (relative to rootNode) for new nodes to be considered viable
    float minimumScale = 0.01f; 

    std::vector<ColorTransform> colorTransformPalette;

    // --- model ---

    // intersection field
    cv::Mat1b m_field;
    Matx33 m_fieldTransform;
    // temp drawing layer, same size as field, for drawing individual nodes and checking for intersection
    mutable cv::Mat1b m_fieldLayer;
    mutable cv::Rect m_fieldLayerBoundingRect;

public:

    SelfLimitingPolygonTree() { }

    virtual void to_json(json &j) const override
    {
        qtree::to_json(j);

        j["_class"] = "SelfLimitingPolygonTree";

        j["fieldResolution"] = fieldResolution;
        j["polygonSides"] = polygonSides;
        j["starAngle"] = starAngle;

        if(!drawPolygon.empty())
            ::to_json(j["drawPolygon"], drawPolygon);

        if (!fieldImagePath.empty())
            j["fieldImage"] = fieldImagePath.string();
        
        ::to_json(j["rootNode"]["color"], rootNodeColor);
    }

    virtual void from_json(json const &j) override
    {
        qtree::from_json(j);

        drawPolygon.clear();
        if(j.contains("drawPolygon"))
            ::from_json(j.at("drawPolygon"), drawPolygon);

        if (j.contains("fieldImage"))
            fieldImagePath = j.at("fieldImage").get<string>();

        fieldResolution = j.at("fieldResolution");
        polygonSides = (j.contains("polygonSides") ? j.at("polygonSides").get<int>() : 5);
        starAngle    = (j.contains("starAngle"   ) ? j.at("starAngle"   ).get<float>() : 0);
        
        if (j.contains("rootNode"))
        {
            ::from_json(j.at("rootNode").at("color"), rootNodeColor);
        }
    }

    virtual void setRandomSeed(int randomize) override
    {
        qtree::setRandomSeed(randomize);

        float maxRadius = 10.0;
        domain = cv::Rect_<float>(-maxRadius, -maxRadius, maxRadius * 2, maxRadius * 2);
        polygonSides = 5;
        starAngle = 36.0f;

        if (randomize)
        {
            maxRadius = (float)(5.0 + r(40.0));
            domain = cv::Rect_<float>(-maxRadius, -maxRadius, maxRadius * 2, maxRadius * 2);
            polygonSides = (randomize % 6) + 3;
            starAngle = ((randomize % 12) < 6 ? 60.0f : 0.0f);
            //gestationRandomness = r(200.0);
        }

        if (starAngle > 0.0f)
            util::polygon::createStar(polygon, polygonSides, starAngle);
        else
            util::polygon::createRegularPolygon(polygon, polygonSides);

        // create set of edge transforms to map edge 0 to all other edges,
        // child polygons will be potentially spawned whose 0 edge aligns with parent polygon's {i} edge
        transforms.clear();
        for (int i = 0; i < polygon.size(); ++i)
        {
            transforms.push_back(createEdgeTransform(i, 0));
        }

        randomizeTransforms(3);
    }

    //  Randomizes existing transforms: color, gestation
    virtual void randomizeTransforms(int flags) override
    {
        if (flags & 1)
        {
            colorTransformPalette.clear();
            // HLS space color transforms
            //colorTransformPalette.push_back(util::scaleAndTranslate(1.0, 0.99, 1.0, 180.0*r()*r()-90.0, 0.0, 0.0));    // darken and hue-shift
            //colorTransformPalette.push_back(ColorTransform::hlsSink( 20.0, 0.5, 1.0, 0.25));   // toward orange
            //colorTransformPalette.push_back(ColorTransform::hlsSink(200.0, 0.5, 1.0, 0.25));   // toward blue
            double lightness = 0.5;
            double sat = 1.0;
            colorTransformPalette.push_back(ColorTransform::hlsSink(r(360.0), lightness, sat, r(0.5)));
            colorTransformPalette.push_back(ColorTransform::hlsSink(r(360.0), lightness, sat, r(0.5)));
            colorTransformPalette.push_back(ColorTransform::hlsSink(r(360.0), lightness, sat, r(0.5)));
            colorTransformPalette.push_back(ColorTransform::hlsSink(r(360.0), lightness, sat, r(0.5)));
            colorTransformPalette.push_back(ColorTransform::hlsSink(r(360.0), 0.5 + r(0.5), sat, r(0.5)));
            colorTransformPalette.push_back(ColorTransform::hueShift(r(2.0) - 1.0));			// slow hueshift
            colorTransformPalette.push_back(ColorTransform::hueShift(r(2.0) - 1.0));			// slow hueshift
            colorTransformPalette.push_back(ColorTransform::hueShift(r(360.0)));            // wild hueshift
            colorTransformPalette.push_back(ColorTransform::hueShift(r(360.0)));            // wild hueshift
            colorTransformPalette.push_back(ColorTransform::hlsTransform(std::vector<float>{ 1.0,0.0, -1.0,1.0, 1.0,0.0 }));            // dark/light alternating L=1-L (no effect on fully sat colors)
            colorTransformPalette.push_back(ColorTransform::hlsTransform(std::vector<float>{ 1.0,0.0, -1.0,0.6, 1.0,0.0 }));            // dark/light alternating 
            //colorTransformPalette.push_back(ColorTransform::hlsTransform(r(360.0), 0.9, 1.0));            // darken
            if (r(2))
            {
                colorTransformPalette.push_back(ColorTransform::hlsSink(r(360.0), 0.0, 0.0, r(0.5)));       // desaturating darken--grayer, muted shadows
                colorTransformPalette.push_back(ColorTransform::hlsSink(r(360.0), 1.0, 0.0, r(0.5)));       // desaturating whiten
            }
            else
            {
                colorTransformPalette.push_back(ColorTransform::hlsSink(r(360.0), 0.0, 1.0, r(0.5)));       // sat darken
                colorTransformPalette.push_back(ColorTransform::hlsSink(r(360.0), 1.0, 1.0, r(0.5)));       // sat whiten
            }
        }

        for (auto & t : transforms)
        {
            if (flags & 1)
            {
                int idx = r((int)colorTransformPalette.size());
                t.colorTransform = colorTransformPalette[idx];
            }

            if (flags & 2)
            {
                t.gestation = 1.0 + r(10.0);
            }
        }

        if (flags & 4)
        {
            // modify polygon that's drawn: remove up to (N-3) vertices
            drawPolygon = polygon;
            for (int i = r((int)polygon.size() - 3); i >= 0; --i)
            {
                drawPolygon.erase(drawPolygon.begin() + r((int)drawPolygon.size()));
            }
        }

    }

    virtual void create() override
    {
        // initialize intersection field
        auto rc = getBoundingRect();
        auto fieldSize = rc.size() * (float)fieldResolution;
        m_field.create(fieldSize);
        m_field = 0;
        m_field.copyTo(m_fieldLayer);

        if (!fieldImagePath.empty())
        {
            cv::Mat fieldImage = cv::imread(fieldImagePath.string());
            cv::cvtColor(fieldImage, fieldImage, cv::ColorConversionCodes::COLOR_BGR2GRAY);
            cv::resize(fieldImage, m_field, m_field.size(), 0, 0, cv::InterpolationFlags::INTER_NEAREST);
        }

        //int x = fieldSize.width / 4;
        //int y = fieldSize.height / 4;
        //auto white = cv::Scalar(255.0, 255.0, 255.0);
        //cv::line(m_field, cv::Point(x, fieldSize.height), cv::Point(x, y), white, 1, cv::LineTypes::LINE_8);
        //cv::line(m_field, cv::Point(2*x, 0), cv::Point(2*x, 3*y), white, 1, cv::LineTypes::LINE_8);
        //cv::line(m_field, cv::Point(3*x, fieldSize.height), cv::Point(3*x, y), white, 1, cv::LineTypes::LINE_8);

        // maps min corner of the bounding rect to (0,0) of the field
        m_fieldTransform = util::transform3x3::getScaleTranslate((double)fieldResolution, (double)-rc.x*fieldResolution, (double)-rc.y*fieldResolution);

        // clear and initialize the queue with the seed

        qnode rootNode;
        createRootNode(rootNode);

        util::clear(nodeQueue);
        nodeQueue.push(rootNode);

        m_nodeList.clear();
    }

    virtual void createRootNode(qnode & rootNode)
    {
        rootNode.id = 0;
        rootNode.parentId = 0;
        rootNode.beginTime = 0;
        rootNode.color = rootNodeColor;

        // center root node at origin
        auto centroid = util::polygon::centroid(polygon);
        rootNode.globalTransform = util::transform3x3::getScaleTranslate(1.0f, -centroid.x, -centroid.y);
    }

    virtual void beget(qnode const & parent, qtransform const & t, qnode & child) override
    {
        qtree::beget(parent, t, child);

        // apply affine transform in HLS space
        child.color = t.colorTransform.apply(parent.color);
    }


    virtual bool isViable(qnode const &node) const override
    {
        if (!node) 
            return false;

        if (fabs(node.det()) < minimumScale*minimumScale)
            return false;

        if (!drawField(node))
            return false;   // out of image bounds

        thread_local cv::Mat andmat;
        cv::bitwise_and(m_field(m_fieldLayerBoundingRect), m_fieldLayer(m_fieldLayerBoundingRect), andmat);
        return(!cv::countNonZero(andmat));
    }

    //  draw node on field stage layer to prepare for collision detection
    //  full collision detection is not done here, but this function returns false if out-of-bounds or other
    //  quick detection means that this node is not viable.
    bool drawField(qnode const &node) const
    {
        vector<cv::Point2f> v;

        // first, transform node polygon to model coordinates
        cv::transform(polygon, v, node.globalTransform.get_minor<2, 3>(0, 0));

        // test each vertex against maxRadius
        for (auto const& p : v)
            if(!isPointInBounds(p))
                return false;

        // transform model polygon to field coords
        Matx33 m = m_fieldTransform * node.globalTransform;
        cv::transform(polygon, v, m.get_minor<2, 3>(0, 0));
        // convert to int-coordinate struct for cv::polylines
        vector<vector<cv::Point> > pts(1);
        for (auto const& p : v)
            pts[0].push_back(p);

        // (double) check that coords are within field
        m_fieldLayerBoundingRect = cv::boundingRect(pts[0]);
        if ((cv::Rect(0, 0, m_field.cols, m_field.rows) & m_fieldLayerBoundingRect) != m_fieldLayerBoundingRect)
            return false;

        // clear a region of our scratch layer and draw node on it
        m_fieldLayer(m_fieldLayerBoundingRect) = 0;
        cv::fillPoly(m_fieldLayer, pts, cv::Scalar(255), cv::LineTypes::LINE_8);
        // reduce by drawing outline in black:
        // this is a bit of a hack to get around the problem of OpenCV always drawing a pixel-wide boundary even when only a fill is specified
        cv::polylines(m_fieldLayer, pts, true, cv::Scalar(0), 1, cv::LineTypes::LINE_8);
        // double-draw and soften the line--purely for aesthetics, since the field layer is exported as well
        cv::polylines(m_fieldLayer, pts, true, cv::Scalar(0), 1, cv::LineTypes::LINE_AA);

        return true;
    }

    void undrawNode(qnode &node)
    {
        drawField(node);
        cv::bitwise_not(m_fieldLayer(m_fieldLayerBoundingRect), m_fieldLayer(m_fieldLayerBoundingRect));
        cv::bitwise_and(m_field(m_fieldLayerBoundingRect), m_fieldLayer(m_fieldLayerBoundingRect), m_field(m_fieldLayerBoundingRect));
    }

    std::list<qnode> m_nodeList;
    std::unordered_set<int> m_markedForDeletion;

    virtual void addNode(qnode &currentNode) override
    {
        qtree::addNode(currentNode);

        m_nodeList.push_back(currentNode);

        // update field image: composite new node
        cv::bitwise_or(m_field(m_fieldLayerBoundingRect), m_fieldLayer(m_fieldLayerBoundingRect), m_field(m_fieldLayerBoundingRect));
    }

    std::list<qnode>::const_iterator findNode(int id) const
    {
        for (auto it = m_nodeList.begin(); it!=m_nodeList.end(); ++it)
        {
            if (it->id == id)
            {
                return it;
            }
        }
        return m_nodeList.end();
    }

    std::list<qnode>::iterator findNode(int id)
    {
        for (auto it = m_nodeList.begin(); it != m_nodeList.end(); ++it)
        {
            if (it->id == id)
            {
                return it;
            }
        }
        return m_nodeList.end();
    }

    void getLineage(qnode const & node, std::vector<string> & lineage) const override
    {
        lineage.clear();
        lineage.push_back(node.sourceTransform);
        int id = node.parentId;
        while (id != 0)
        {
            auto it = findNode(id);
            if (it == m_nodeList.end())
            {
                cout << "Parent id not found:>" << id << endl;
                return;
            }
            lineage.push_back(it->sourceTransform);
            id = it->parentId;
        }
    }

    virtual void getNodesIntersecting(cv::Rect2f const &rect, std::vector<qnode> &nodes) const override
    {
        for (auto & node : m_nodeList)
        {
            std::vector<cv::Point2f> pts;
            getPolyPoints(node, pts);

            if (cv::pointPolygonTest(pts, rect.tl(), false) >= 0.0)
            {
                nodes.push_back(node);
            }
            else
            {
                // this is not a proper intersection
                for (auto const& pt : pts)
                {
                    if (rect.contains(pt))
                    {
                        nodes.push_back(node);
                        break;
                    }
                }
            }
        }
    }

    virtual int removeNode(int id) override
    {
        auto it = m_nodeList.begin();
        if(id>=0)
            it = findNode(id);

        if (it == m_nodeList.end())
        {
            // not found
            return 0;
        }
        cout << " -- removing " << (it->id) << " from " << (it->parentId) << " remaining: " << m_nodeList.size() << endl;
        undrawNode(*it);
        it = m_nodeList.erase(it);
        return 1;
        /*
        m_markedForDeletion.clear();
        m_markedForDeletion.insert(id);

        markDescendantsForDeletion();

        int removed = 0;
        for (auto it = m_nodeList.begin(); it != m_nodeList.end(); )
        {
            if (m_markedForDeletion.find(it->id) != m_markedForDeletion.end())
            {
                cout << " -- removing " << (it->id) << " from " << (it->parentId) << endl;
                undrawNode(*it);

                it = m_nodeList.erase(it);
                ++removed;
            }
            else
            {
                ++it;
            }
        }

        m_markedForDeletion.clear();

        //sprout();

        return removed;//*/
    }

    //bool isDescendantOf(int child, int ancestor)
    //{
    //    auto it = findNode(child);
    //    while (it!=m_nodeList.end())
    //    {
    //        if(it->id==ancestor)
    //            return true;
    //        if (it->parentId == it->id)
    //            return false;
    //        it = findNode(it->parentId);
    //    }
    //    // ??
    //    return false;
    //}

    void markDescendantsForDeletion()
    {
        std::unordered_set<int> safeList;

        for (auto it = m_nodeList.begin(); it != m_nodeList.end(); ++it)
        {
            bool del = false;

            int idx = it->id;
            while (1)
            {
                if (safeList.find(idx) != safeList.end())
                {
                    //safe = true;
                    break;
                }
                if (m_markedForDeletion.find(idx) != m_markedForDeletion.end())
                {
                    del = true;
                    break;
                }
                auto jt = findNode(idx);
                if (jt->parentId == idx)
                    break;
                idx = jt->parentId;
            }

            if (del)
            {
                auto jt = it;
                while (1)
                {
                    m_markedForDeletion.insert(jt->id);
                    if (jt->parentId == jt->id)
                        break;
                    jt = findNode(jt->parentId);
                }
            }
            else
            {
                auto jt = it;
                while (1)
                {
                    safeList.insert(jt->id);
                    if (jt->parentId == jt->id)
                        break;
                    jt = findNode(jt->parentId);
                }
            }
        }
    }

    //bool isAncestorMarkedForDeletion(int id)
    //{
    //    auto it = findNode(id);
    //    while (it != m_nodeList.end())
    //    {
    //        if (m_markedForDeletion.find(it->id) != m_markedForDeletion.end())
    //            return true;
    //        if (it->parentId == it->id)
    //            return false;
    //        it = findNode(it->parentId);
    //    }
    //    // ??
    //    return false;
    //}

    virtual void regrowAll() override
    {
        for (auto & currentNode : m_nodeList)
        {
            // !TODO! this is duplicated code
            // create a child node for each available transform.
            // all child nodes are added to the queue, even if not viable.
            for (auto const &t : transforms)
            {
                qnode child;
                beget(currentNode, t, child);
                nodeQueue.push(child);
            }
        }
    }

    //  Node draw function for tree of nodes with all the same polygon
    virtual void redrawAll(qcanvas &canvas) override
    {
        canvas.clear();

        for (auto & node : m_nodeList)
        {
            drawNode(canvas, node);
        }
    }

};

REGISTER_QTREE_TYPE(SelfLimitingPolygonTree);

#pragma endregion

#pragma region ScaledPolygonTree

class ScaledPolygonTree : public SelfLimitingPolygonTree
{
    float m_ratio;
    bool m_ambidextrous;

public:
    virtual void setRandomSeed(int randomize) override
    {
        SelfLimitingPolygonTree::setRandomSeed(randomize);

        fieldResolution = 100;
        float maxRadius = 4.0;
        domain = cv::Rect_<float>(-maxRadius, -maxRadius, maxRadius * 2, maxRadius * 2);

        // ratio: child size / parent size
        std::array<float, 5> ratioPresets = { {
            (sqrt(5.0f) - 1.0f) / 2.0f,        // phi
            0.5f,
            1.0f / 3.0f,
            1.0f / sqrt(2.0f),
            (sqrt(3.0f) - 1.0f) / 2.0f
        } };

        m_ratio = ratioPresets[randomize%ratioPresets.size()];
        m_ambidextrous = r(2);// (randomize % 2);

        // override edge transforms
        transforms.clear();
        for (int i = 0; i < polygon.size(); ++i)
        {
            transforms.push_back(createEdgeTransform(i, polygon.size() - 1, false, 0.0, m_ratio));

            if (m_ambidextrous)
                transforms.push_back(createEdgeTransform(i, polygon.size() - 1, true, 1.0 - m_ratio, 1.0));

        }

        randomizeTransforms(3);
    }

    virtual void to_json(json &j) const override
    {
        SelfLimitingPolygonTree::to_json(j);

        j["_class"] = "ScaledPolygonTree";
        j["ratio"] = m_ratio;
        j["ambidextrous"] = m_ambidextrous;
    }

    virtual void from_json(json const &j) override
    {
        SelfLimitingPolygonTree::from_json(j);

        m_ratio = j["ratio"];
        m_ambidextrous = j["ambidextrous"];
    }

    virtual void create() override
    {
        SelfLimitingPolygonTree::create();


    }

};

REGISTER_QTREE_TYPE(ScaledPolygonTree);

#pragma endregion

#pragma region TrapezoidTree

class TrapezoidTree : public SelfLimitingPolygonTree
{
public:
    virtual void setRandomSeed(int randomize) override
    {
        SelfLimitingPolygonTree::setRandomSeed(randomize);

        fieldResolution = 200;
        float maxRadius = 10;
        domain = cv::Rect_<float>(-maxRadius, -maxRadius, maxRadius * 2, maxRadius * 2);
        gestationRandomness = 10;
        rootNodeColor = cv::Scalar(0.2, 0.5, 0, 1);
    }

    virtual void create() override
    {
        SelfLimitingPolygonTree::create();

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
        //            ColorTransform::hlsSink(randomColor(), 0.5f))
        //    );
        //}

        transforms.push_back(
            qtransform(
                util::transform3x3::getMirroredEdgeMap(polygon[0], polygon[1], polygon[1], polygon[2]),
                ColorTransform::rgbSink(randomColor(), 0.5))
        );
        transforms.push_back(
            qtransform(
                util::transform3x3::getEdgeMap(polygon[0], polygon[1], polygon[3], polygon[2]),
                ColorTransform::rgbSink(randomColor(), 0.5))
        );
        transforms.push_back(
            qtransform(
                util::transform3x3::getMirroredEdgeMap(polygon[0], polygon[1], polygon[3], polygon[0]),
                ColorTransform::rgbSink(randomColor(), 0.5))
        );

        //transforms[0].gestation = 1111.1;
        transforms[0].gestation = r(10.0);
        transforms[1].gestation = r(10.0);
        transforms[2].gestation = r(10.0);

        //transforms[0].colorTransform = ColorTransform::hlsSink(1.0f,1.0f,1.0f, 0.5f);
        //transforms[0].colorTransform = ColorTransform::hlsSink(0.0f,0.5f,0.0f, 0.8f);
        //transforms[1].colorTransform = ColorTransform::hlsSink(0.5f,1.0f,1.0f, 0.3f);
        //transforms[2].colorTransform = ColorTransform::hlsSink(0.9f,0.5f,0.0f, 0.8f);

    }

};

REGISTER_QTREE_TYPE(TrapezoidTree);

#pragma endregion

#pragma region ThornTree

//  Tesselations based on the "Versatile" thorn-shaped 9-sided polygon
//  described by Penrose, Grunwald, ...
class ThornTree : public SelfLimitingPolygonTree
{
public:
    virtual void setRandomSeed(int randomize) override
    {
        SelfLimitingPolygonTree::setRandomSeed(randomize);

        fieldResolution = 20;
        float maxRadius = 50;
        domain = cv::Rect_<float>(-maxRadius, -maxRadius, maxRadius * 2, maxRadius * 2);
        gestationRandomness = 0;

        rootNodeColor = cv::Scalar(1.0, 1.0, 0.0, 1);

        // override polygon with thorn/versatile polygon
        polygon.clear();
        cv::Point2f pt(0, 0);
        polygon.push_back(pt);
        polygon.push_back(pt += util::polygon::headingStep(  0.0f));
        polygon.push_back(pt += util::polygon::headingStep(120.0f));
        polygon.push_back(pt += util::polygon::headingStep(105.0f));
        polygon.push_back(pt += util::polygon::headingStep( 90.0f));
        polygon.push_back(pt += util::polygon::headingStep( 75.0f));
        polygon.push_back(pt += util::polygon::headingStep(240.0f));
        polygon.push_back(pt += util::polygon::headingStep(255.0f));
        polygon.push_back(pt += util::polygon::headingStep(270.0f));

        drawPolygon = polygon;
        // modify polygon that's drawn
        //drawPolygon.erase(drawPolygon.begin() + 1, drawPolygon.begin() + 5);

        // override edge transforms
        transforms.clear();
        //auto ct1 = ColorTransform::hlsSink(util::hsv2bgr( 10.0, 1.0, 0.75), 0.3);
        //auto ct2 = ColorTransform::hlsSink(util::hsv2bgr(200.0, 1.0, 0.75), 0.3);

        for (int i = 0; i < polygon.size(); ++i)
        {
            for (int j = 0; j < polygon.size(); ++j)
            {
                if (r(20) == 0)
                    transforms.push_back(createEdgeTransform(i, j));

                if (r(20) == 0)
                    transforms.push_back(createEdgeTransform(i, j, true));
            }
        }

        randomizeTransforms(3);
    }

    virtual void to_json(json &j) const override
    {
        SelfLimitingPolygonTree::to_json(j);
        
        j["_class"] = "ThornTree";
    }

    virtual void from_json(json const &j) override
    {
        SelfLimitingPolygonTree::from_json(j);

        if (drawPolygon.empty())
        {
            drawPolygon = polygon;
        }
    }

    virtual void create() override
    {
        SelfLimitingPolygonTree::create();

    }

    // overriding to save intersection field mask as well
    virtual void saveImage(fs::path imagePath, qcanvas const& canvas) override
    {
        auto svgPath = imagePath.replace_extension("svg");

        // save the intersection field mask
        imagePath = imagePath.replace_extension("mask.png");
        cv::imwrite(imagePath.string(), m_field);

        //auto rc = getBoundingRect();
        //canvas.svgDocument.setDimensions(rc.width, rc.height);

        canvas.getSVG().save(svgPath.string());
    }

    void drawNode(qcanvas &canvas, qnode const &node) override
    {
        cv::Scalar color =
            //(node.det() < 0) ? 255.0 * (cv::Scalar(1.0, 1.0, 1.0, 1.0) - node.color) : 
            255.0 * node.color;

        // todo
        canvas.fillPoly(drawPolygon, node.globalTransform, 255.0 * node.color, lineThickness, lineColor);

        // modify polygon that's drawn
        //pts[0].resize(4);

        //float d = cv::norm(pts[0][0] - pts[0][1]);
        //auto ctr0 = (pts[0][0] + pts[0][1] + pts[0][2] + pts[0][8]) / 4;
        //cv::circle(canvas.image, ctr0, d*0.3f, color, -1, cv::LineTypes::FILLED, 4);
        //auto ctr1 = (pts[0][2] + pts[0][3] + pts[0][7] + pts[0][8]) / 4;
        //cv::circle(canvas.image, ctr1, d*0.2f, color, -1, cv::LineTypes::FILLED, 4);
        //auto ctr2 = (pts[0][3] + pts[0][4] + pts[0][6] + pts[0][7]) / 4;
        //cv::circle(canvas.image, ctr2, d*0.15f, color, -1, cv::LineTypes::FILLED, 4);
        //auto ctr3 = (pts[0][4] + pts[0][6]) / 2;
        //cv::circle(canvas.image, ctr3, d*0.1f, color, -1, cv::LineTypes::FILLED, 4);
    }


};

REGISTER_QTREE_TYPE(ThornTree);

#pragma endregion
