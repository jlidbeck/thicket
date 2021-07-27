#pragma once

#include "ColorTransform.h"
#include "util.h"
#include "simple_svg.hpp"
#include <opencv2/core/core.hpp>
#include <vector>
#include <queue>
#include <filesystem>
#include <random>
#include <nlohmann/json.hpp>
#include <iostream>
#include <unordered_map>
#include <memory>


using json = nlohmann::basic_json<>;

namespace fs = std::filesystem;


using std::cout;
using std::endl;

class qcanvas;

using std::vector;
using std::string;
using std::priority_queue;

using cv::Point2f;
typedef cv::Matx<float, 3, 3> Matx33;
typedef cv::Matx<float, 4, 1> Matx41;
typedef cv::Matx<float, 4, 4> Matx44;


class qcanvas
{
    Matx33          m_globalTransform;
    cv::Mat         m_image;
    svg::Document   m_svgDocument;

public:

    qcanvas() {
    }

    cv::Mat const & getImage() const { return m_image; }

    svg::Document const & getSVG() const { return m_svgDocument; }

    void create(cv::Mat im)
    {
        m_image = im;

        // Match SVG dimensions to pixel size
        m_svgDocument = svg::Document(svg::Layout(svg::Dimensions(im.cols, im.rows), svg::Layout::Origin::TopLeft)); // no flip, no scale
    }

    void clear()
    {
        // clear image to black
        m_image = 0;

        // clear vector data
        m_svgDocument = svg::Document(svg::Layout(svg::Dimensions(m_image.cols, m_image.rows), svg::Layout::Origin::TopLeft)); // no flip, no scale

        // draw image border
        svg::Polygon border(svg::Stroke(1, svg::Color(55, 55, 55)));
        border << svg::Point(0, 0) << svg::Point(m_image.cols, 0)
            << svg::Point(m_image.cols, m_image.rows) << svg::Point(0, m_image.rows);
        m_svgDocument << border;

        m_svgDocument << svg::Text(svg::Point(5, m_image.rows - 5), "Simple SVG", svg::Color(55, 55, 55), svg::Font(10, "Franklin Gothic"));
    }

    // sets global transform map to map provided domain to m_image, centered, vertically flipped
    void setTransformToFit(cv::Rect_<float> const &domain, float buffer)
    {
        if (m_image.empty()) throw std::exception("Image is empty");

        m_globalTransform = util::transform3x3::centerAndFit(
            domain, 
            cv::Rect_<float>(0.0f, 0.0f, (float)m_image.cols, (float)m_image.rows), 
            buffer, 
            true);
    }

    cv::Point2f canvasToModel(cv::Point2f pt)
    {
        Matx33 m = m_globalTransform.inv(cv::DecompTypes::DECOMP_LU);
        auto t = m * cv::Point3f(pt.x, pt.y, 1.0f);
        return cv::Point2f(t.x, t.y);
    }

    void fillPoly(std::vector<cv::Point2f> const &polygon, Matx33 const &transform, cv::Scalar color, int lineThickness, cv::Scalar lineColor)
    {
        Matx33 m = m_globalTransform * transform;

        vector<cv::Point2f> v;
        cv::transform(polygon, v, m.get_minor<2, 3>(0, 0));
        vector<vector<cv::Point> > pts(1);
        for (auto const& p : v)
            pts[0].push_back(p * 16);

        if (lineThickness > 0)
        {
            cv::fillPoly(m_image, pts, color, cv::LineTypes::LINE_8, 4);
            cv::polylines(m_image, pts, true, lineColor, lineThickness, cv::LineTypes::LINE_AA, 4);
        }
        else
        {
            cv::fillPoly(m_image, pts, color, cv::LineTypes::LINE_AA, 4);
        }



        svg::Polygon svgPolygon(
            svg::Fill(svg::Color(color[2], color[1], color[0])), 
            svg::Stroke()   // no outline
        );
        for (auto const& p : v)
            svgPolygon << svg::Point(p.x, p.y);

        m_svgDocument << svgPolygon;


    }

};


class qtransform
{
public:
    string key;
    Matx33 transformMatrix;
    ColorTransform colorTransform;
    double gestation;

public:

    qtransform(Matx33 const &transformMatrix_ = Matx33::eye(), ColorTransform const &colorTransform_ = ColorTransform(), double gestation_ = 1.0)
    {
        transformMatrix = transformMatrix_;
        colorTransform = colorTransform_;
        gestation = gestation_;
    }

    qtransform(string key_, Matx33 const &transformMatrix_ = Matx33::eye(), ColorTransform const &colorTransform_ = ColorTransform(), double gestation_ = 1.0)
    {
        key = key_;
        transformMatrix = transformMatrix_;
        colorTransform = colorTransform_;
        gestation = gestation_;
    }

    //template<typename _Tp>
    //qtransform(_Tp m00, _Tp m01, _Tp mtx, _Tp m10, _Tp m11, _Tp mty, ColorTransform const &colorTransform_)
    //{
    //    transformMatrix = Matx33(m00, m01, mtx, m10, m11, mty, 0, 0, 1);
    //    colorTransform = colorTransform_;
    //    gestation = 1.0;
    //}

    qtransform(float m00, float m01, float mtx, float m10, float m11, float mty, ColorTransform const& colorTransform_)
    {
        transformMatrix = Matx33(m00, m01, mtx, m10, m11, mty, 0, 0, 1);
        colorTransform = colorTransform_;
        gestation = 1.0;
    }

    qtransform(double angle, double scale, cv::Point2f translate)
    {
        transformMatrix = util::transform3x3::getRotationMatrix2D(Point2f(), angle, scale, translate.x, translate.y);
        //colorTransform = colorTransform.eye();
        //colorTransform(2, 2) = 0.94f;
        //colorTransform(1, 1) = 0.96f;
        gestation = 1.0;
    }
};

inline void to_json(json &j, qtransform const &t)
{
    j = json {
        { "gestation", t.gestation }
    };

    to_json(j["color"], t.colorTransform);

    if (!t.key.empty())
        j["transformKey"] = t.key;

    to_json(j["transform"], t.transformMatrix);
}

inline void from_json(json const &j, qtransform &t)
{
    t.gestation = j.at("gestation");
    from_json(j.at("color"),     t.colorTransform);
    from_json(j.at("transform"), t.transformMatrix);
    t.key = (j.contains("transformKey") ? j.at("transformKey") : "");
}

inline void to_json(json &j, std::vector<qtransform> const &transforms)
{
    j = nlohmann::json::array();
    for (int i = 0; i < transforms.size(); ++i)
        to_json(j[i], transforms[i]);
}

inline void from_json(json const &j, std::vector<qtransform> &transforms)
{
    if (j == nullptr || j.empty())
    {
        throw std::exception("No transforms found: JSON is empty");
    }

    transforms.resize(j.size());
    for (int i = 0; i < j.size(); ++i)
    {
        from_json(j[i], transforms[i]);
        // add unique names if they're undefined
        if (transforms[i].key.empty())
        {
            transforms[i].key = string("T") + std::to_string(i);
        }
    }
}


class qnode
{
public:
    int         id              = 0;
    int         parentId        = 0;
    string      sourceTransform;
    double      beginTime       = 0.0;
    Matx33      globalTransform;
    cv::Scalar  color = cv::Scalar(1.0, 0.5, 0.0, 1.0);


    qnode(int id_=0, int parentId_=0, double beginTime_ = 0)
    {
        id = id_;
        parentId = parentId_;
        beginTime = beginTime_;
        color = cv::Scalar(1, 1, 1, 1);
        globalTransform = globalTransform.eye();
    }

    inline float det() const { return (globalTransform(0, 0) * globalTransform(1, 1) - globalTransform(0, 1) * globalTransform(1, 0)); }

    inline bool operator!() const { return !( fabs(det()) > 1e-5 ); }

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


//  This macro should be invoked for each qtree-extending class
//  It creates a global function pointer (which is never used)
//  and registers a constructor lambda for the given qtree-derived class
#define REGISTER_QTREE_TYPE(TYPE) \
    auto const g_constructor_ ## TYPE = qtree::registerConstructor(#TYPE, [](){ return std::shared_ptr<TYPE>( new TYPE() ); });

class qtree
{

public:
    // settings
    string name;
    cv::Rect_<float> domain = cv::Rect_<float>(-100.0, -100.0, 100.0 * 2, 100.0 * 2);

    enum class DomainShape {
        RECT,
        ELLIPSE
    } domainShape = DomainShape::RECT;
    int randomSeed = 0;

    // same polygon for all nodes
    std::vector<cv::Point2f> polygon;

    // set of parent-to-child node transforms, defining child characteristics relative to parent's
    std::vector<qtransform> transforms;
    
    double gestationRandomness = 0.0;

    // draw settings
    cv::Scalar lineColor = cv::Scalar(0);
    int lineThickness = 0;

    // model
    std::mt19937 prng; //Standard mersenne_twister_engine with default seed
    std::priority_queue<qnode, std::deque<qnode>, qnode::EarliestFirst> nodeQueue;
    int nextNodeId = 1;

    // stats
    std::unordered_map<string, int> transformCounts;

public:
    qtree() {}

    virtual void setRandomSeed(int seed)
    {
        randomSeed = seed;
        prng.seed(seed);
    }

    virtual void randomizeTransforms(int flags) {};

#pragma region Serialization

    //  Extending classes should override and invoke the base member as necessary
    virtual void to_json(json &j) const
    {
        //  extending classes need to override this value
        j["_class"] = "qtree";

        j["name"] = name;
        j["randomSeed"] = randomSeed;

        //j["maxRadius"] = maxRadius;
        auto rc = getBoundingRect();
        //j["bounds"]["radius"] = maxRadius;
        j["bounds"]["shape"] = (domainShape == DomainShape::ELLIPSE ? "ellipse" : "rect");
        j["bounds"]["xmin"] = rc.x;
        j["bounds"]["ymin"] = rc.y;
        j["bounds"]["width"] = rc.width;
        j["bounds"]["height"] = rc.height;

        ::to_json(j["polygon"], polygon);
        for (auto &t : transforms)
        {
            json jt;
            ::to_json(jt, t);
            j["transforms"].push_back(jt);
        }

        j["gestationRandomness"] = gestationRandomness;

        j["drawSettings"] = json{
            { "lineColor", util::toRgbHexString(lineColor) },
            { "lineThickness", lineThickness }
        };
    }

    //  Extending classes should override and invoke the base member as necessary
    virtual void from_json(json const &j)
    {
        // using at() rather than [] so we get a proper exception on a missing key
        // (rather than assert/abort with NDEBUG)

        name = (j.contains("name") ? j.at("name").get<string>() : "");

        randomSeed = (j.contains("randomSeed") ? j.at("randomSeed").get<int>() : 0);

        if (j.contains("maxRadius"))
        {
            // legacy
            float maxRadius = j.at("maxRadius");
            domain = cv::Rect_<float>(-maxRadius, -maxRadius, maxRadius * 2, maxRadius * 2);
            domainShape = DomainShape::ELLIPSE;
        }

        if (j.contains("bounds") && j["bounds"].contains("xmin"))
        {
            domain = cv::Rect_<float>(
                domain.x      = j["bounds"]["xmin"],
                domain.y      = j["bounds"]["ymin"],
                domain.width  = j["bounds"]["width"],
                domain.height = j["bounds"]["height"]
                );

            domainShape = (j["bounds"]["shape"] == string("ellipse") ? DomainShape::ELLIPSE : DomainShape::RECT);
        }

        if (j.contains("polygon"))
        {
            ::from_json(j.at("polygon"), polygon);
        }

        if (j.contains("transforms"))
        {
            ::from_json(j.at("transforms"), transforms);
        }

        gestationRandomness = (j.contains("gestationRandomness") ? j.at("gestationRandomness").get<double>() : 0.0);

        if (j.contains("drawSettings"))
        {
            lineColor = util::fromRgbHexString(j.at("drawSettings").at("lineColor").get<string>().c_str());
            lineThickness = j.at("drawSettings").at("lineThickness");
        }
    }

    static std::map<string, std::function<std::shared_ptr<qtree> ()> >& factory()
    {
        static std::map<string, std::function<std::shared_ptr<qtree> ()> > factoryTable;
        return factoryTable;
    }

    //  Registers a typed constructor lambda fn
    static auto registerConstructor(string className, std::function<std::shared_ptr<qtree>()> const &fn)
    {
        factory()[className] = fn;
        return fn;
    }

    //  Factory method to create instances of registered qtree-extending classes
    static std::shared_ptr<qtree> createTreeFromJson(json const &j)
    {
        //  peek at the "_class" member of the json to see what class to instantiate

        if (!j.contains("_class"))
        {
            throw(std::exception("Invalid JSON or missing \"_class\" key."));
        }

        string className = j["_class"];
        if (factory().find(className) == factory().end())
        {
            string msg = string("Class not registered: '") + className + "'";
            throw(std::exception(msg.c_str()));
        }

        auto pfn = factory().at(className);
        std::shared_ptr<qtree> pPrototype;
        try
        {
            pPrototype = pfn();
            pPrototype->from_json(j);
        }
        catch (std::exception& ex)
        {
            std::cout << "Failed to create instance of '" << className << "' from JSON.\n";
            std::cout << ex.what() << std::endl;
            std::cout << j << std::endl;
        }
        return pPrototype;
    }

    std::shared_ptr<qtree> clone() const
    {
        json j;
        to_json(j);
        return qtree::createTreeFromJson(j);
    }

#pragma endregion

    virtual void create() = 0;

    // process the next node in the queue
    virtual bool process();

    // override to indicate that a child node should not be added
    virtual bool isViable(qnode const & node) const { return true; }

    // invoked when a viable node is pulled from the queue. override to update drawing, data structures, etc.
    virtual void addNode(qnode & node);

    virtual void getNodesIntersecting(cv::Rect2f const &rect, std::vector<qnode> &nodes) const {}

    // convenience fn: get node's polygon in model coordinates
    virtual void getPolyPoints(qnode const &node, std::vector<cv::Point2f> &transformedPoints) const
    {
        cv::transform(polygon, transformedPoints, node.globalTransform.get_minor<2, 3>(0, 0));
    }

    virtual int removeNode(int id) { return 0; }

    // generate a child node from a parent
    virtual void beget(qnode const & parent, qtransform const & t, qnode & child);

    // fills vector with transform IDs
    virtual void getLineage(qnode const & node, std::vector<string> & lineage) const { }

    // trigger all existing nodes to attempt to re-bud child nodes
    virtual void regrowAll() {}

    virtual cv::Rect_<float> getBoundingRect() const
    {
        return domain;
    }

    virtual bool isPointInBounds(cv::Point2f pt) const
    {
        switch (domainShape)
        {
        case DomainShape::ELLIPSE:
            return (pt.dot(pt) <= domain.width*domain.height/4.0f); // circular bounds

        case DomainShape::RECT:
        default:
            return(domain.contains(pt));    // rectangular bounds
        }
    }

    virtual void redrawAll(qcanvas &canvas) {}
    virtual void drawNode(qcanvas &canvas, qnode const &node);

    virtual void saveImage(fs::path imagePath, qcanvas const &canvas) { };

    virtual void combineWith(qtree const &tree, double a)
    {
        if (polygon != tree.polygon)
        {
            throw(std::exception("Different polygon"));
        }

		std::cout << "combineWith: " << transforms.size() << " t x " << tree.transforms.size() << " t\n";

		auto unmatchedTransforms = transforms;
		std::vector<qtransform> newTransforms;

        for (auto const &othert : tree.transforms)
        {
            // already has equivalent?
            bool matched = false;
            for (auto it = unmatchedTransforms.cbegin(); it!=unmatchedTransforms.end(); ++it)
            {
				auto thist = *it;
                //if (thist.key == othert.key)
                if(util::approximatelyEqual(thist.transformMatrix, othert.transformMatrix))
                {
                    thist.gestation = (1.0 - a)*thist.gestation + a * othert.gestation;
                    thist.colorTransform.linterp(othert.colorTransform, a);
					newTransforms.push_back(thist);
					unmatchedTransforms.erase(it);
                    matched = true;
                    break;
                }
            }

            if (!matched)
            {
                // other transform not matched... interpolate as very delayed transform
				auto tt = othert;
				tt.gestation = tt.gestation / a;
                newTransforms.push_back(tt);
            }
        }

		for (auto const& ut : unmatchedTransforms)
		{
			// other transform not matched... interpolate as very delayed transform
			auto tt = ut;
			tt.gestation = tt.gestation / a;
			newTransforms.push_back(tt);
		}

		cout << "After matching: " << newTransforms.size() << " transforms\n";
		transforms = newTransforms;

		name = std::string("[") + name + "+" + tree.name + "," + std::to_string(a) + "]";
    }

    // util
    
#pragma region PRNG

    //  random double in [0, 1)
    inline double r()
    {
        std::uniform_real_distribution<double> dist{ 0.0, 1.0 };
        return dist(prng);
    }

    //  random double in [0, max)
    inline double r(double maxVal)
    {
        std::uniform_real_distribution<double> dist{ 0.0, maxVal };
        return dist(prng);
    }

    //  random int in [0, max)
    inline int r(int maxVal)
    {
        std::uniform_int_distribution<int> dist{ 0, maxVal-1 };
        return dist(prng);
    }

    inline cv::Scalar randomColor()
    {
        return util::hsv2bgr(r(360.0), 1.0, 0.5);
    }

#pragma endregion

    //  Returns a 2D transform (scale, rotate, reflect, and transform) that maps the source edge, 
    //  or a specified segment of the source edge, to the target edge.
    //  The matrix will have a nonnegative determinant unless mirror is specified.
    //  Note:
    //  Transform creates an external-to-external mapping, with the goal of placing tiles that align but do not overlap.
    //  For example, if the source and target edges are the same, this function will compute a 180-degree rotation (if mirror==false), 
    //  or a reflection over the line defined by the edge (if mirror==true).
    //  If 0 < ratio0 < ratio1 < 1, transform will be a size reduction, aligning the entire mapped source edge to a subsegment of the premapped target edge.
    qtransform createEdgeTransform(int parentEdge, int childEdge, bool mirror=false, float ratio0=0.0f, float ratio1=1.0f) const
    {
        auto parentEdgeString = string("E") + std::to_string(parentEdge);
        if (ratio0 != 0.0f || ratio1 != 1.0f)
            parentEdgeString += (string("[") + std::to_string(ratio0) + ":" + std::to_string(ratio1) + "]");
        auto childEdgeString  = string("E") + std::to_string(childEdge);

        auto p0 = polygon[parentEdge];
        auto p1 = polygon[(parentEdge + 1) % polygon.size()];
        auto midPt0 = (1.0f - ratio0) * p0 + ratio0 * p1;
        auto midPt1 = (1.0f - ratio1) * p0 + ratio1 * p1;
        auto c0 = polygon[childEdge];
        auto c1 = polygon[(childEdge + 1) % polygon.size()];

        if (mirror)
        {
            return qtransform(
                parentEdgeString + ":-" + childEdgeString,
                util::transform3x3::getMirroredEdgeMap(c0, c1, midPt0, midPt1)
            );
        }

        return qtransform(
            parentEdgeString + ":+" + childEdgeString,
            util::transform3x3::getEdgeMap(c0, c1, midPt1, midPt0)
        );

    }
};


