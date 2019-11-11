#pragma once

#include "util.h"
#include <opencv2/core/core.hpp>
#include <vector>
#include <queue>
#include <filesystem>
#include <random>
#include <nlohmann/json.hpp>

using json = nlohmann::basic_json<>;

namespace fs = std::filesystem;


class qcanvas;

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

    // sets global transform map to map provided domain to image, centered, vertically flipped
    void setScaleToFit(cv::Rect_<float> const &rect, float buffer)
    {
        if (image.empty()) throw std::exception("Image is empty");

        globalTransform = util::transform3x3::centerAndFit(rect, cv::Rect_<float>(0.0f, 0.0f, (float)image.cols, (float)image.rows), buffer, true);
    }

};


class qtransform
{
public:
    Matx33 transformMatrix;
    Matx44 colorTransform;
    double gestation;

public:

    qtransform(Matx33 const &transformMatrix_ = Matx33::eye(), Matx44 const &colorTransform_ = Matx44::eye(), double gestation_ = 1.0)
    {
        transformMatrix = transformMatrix_;
        colorTransform = colorTransform_;
        gestation = gestation_;
    }

    template<typename _Tp>
    qtransform(_Tp m00, _Tp m01, _Tp mtx, _Tp m10, _Tp m11, _Tp mty, Matx44 const &colorTransform_)
    {
        transformMatrix = Matx33(m00, m01, mtx, m10, m11, mty, 0, 0, 1);
        colorTransform = colorTransform_;
        gestation = 1.0;
    }

    qtransform(double angle, double scale, cv::Point2f translate)
    {
        transformMatrix = util::transform3x3::getRotationMatrix2D(Point2f(), angle, scale, translate.x, translate.y);
        colorTransform = colorTransform.eye();
        colorTransform(2, 2) = 0.94f;
        colorTransform(1, 1) = 0.96f;
        gestation = 1.0;
    }
};

namespace util
{
    //using json = nlohmann::json;

    template<typename _Tp>
    nlohmann::json to_json(std::vector<cv::Point_<_Tp> > const &polygon)
    {
        nlohmann::json jm = nlohmann::json::array();
        for (auto &pt : polygon)
        {
            jm.push_back(pt.x);
            jm.push_back(pt.y);
        }
        return jm;
    }

    template<typename _Tp, int m, int n>
    nlohmann::json to_json(cv::Matx<_Tp, m, n> const &mat)
    {
        nlohmann::json jm = nlohmann::json::array();
        for (int i = 0; i < m; ++i)
            for (int j = 0; j < n; ++j)
                jm[i].push_back((float)mat(i, j));
                //sz += std::to_string(mat(i, j)) + (j < n - 1 ? ", " : i < n - 1 ? ",\n  " : "\n  ]");
        return jm;
    }

    template<typename _Tp>
    nlohmann::json to_json(qtransform const &t)
    {
        nlohmann::json j = {
            { "transform", to_json(t.transformMatrix) },
            { "color", to_json(t.colorTransform) },
            { "gestation", t.gestation }
        };
        return j;
    }

    template<typename _Tp>
    bool from_json(nlohmann::json const &j, std::vector<cv::Point_<_Tp> > &polygon)
    {
        if (!j.is_array()) return false;

        polygon.clear();
        polygon.reserve(j.size());

        for (int i = 0; i < j.size(); i += 2)
            polygon.push_back(cv::Point2f(j[i], j[i + 1]));

        return true;
    }

}


class qnode
{
public:
    double      beginTime;
    int         generation;
    Matx33      globalTransform;
    cv::Scalar  color = cv::Scalar(1, 0, 0, 1);

    qnode(double beginTime_ = 0)
    {
        beginTime = beginTime_;
        generation = 0;
        color = cv::Scalar(1, 1, 1, 1);
        globalTransform = globalTransform.eye();
    }

    inline float det() const { return (globalTransform(0, 0) * globalTransform(1, 1) - globalTransform(0, 1) * globalTransform(1, 0)); }

    inline bool operator!() const { return !( fabs(det()) > 1e-5 ); }

    void getPolyPoints(std::vector<cv::Point2f> const &points, std::vector<cv::Point2f> &transformedPoints) const
    {
        cv::transform(points, transformedPoints, globalTransform.get_minor<2, 3>(0, 0));
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


class qtree
{
public:
    // settings
    double maxRadius = 100.0;
    int randomSeed = 0;

    // same polygon for all nodes
    std::vector<cv::Point2f> polygon;
    std::vector<qtransform> transforms;
    double offspringTemporalRandomness = 100.0;
    
    // model
    std::mt19937 prng; //Standard mersenne_twister_engine with default seed
    std::priority_queue<qnode, std::deque<qnode>, qnode::EarliestFirst> nodeQueue;

public:
    qtree() {}

    virtual void setRandomSeed(int seed)
    {
        randomSeed = seed;
        prng.seed(seed);
    }

    virtual json getSettings() const
    {
        json j;
        j["randomSeed"] = randomSeed;
        j["maxRadius"] = (maxRadius);
        j["polygon"] = util::to_json(polygon);
        for (auto &t : transforms)
            j["transforms"].push_back(util::to_json<float>(t));
        j["offspringTemporalRandomness"] = offspringTemporalRandomness;
        return j;
    }

    virtual bool settingsFromJson(json const &j)
    {
        randomSeed = j["randomSeed"];
        maxRadius = j["maxRadius"];
        util::from_json(j["polygon"], polygon);
        offspringTemporalRandomness = j["offspringTemporalRandomness"];
        return true;
    }

    virtual void create() = 0;

    // process the next node in the queue
    virtual bool process();

    // override to indicate that a child node should not be added
    virtual bool isViable(qnode const & node) const { return true; }

    // invoked when a viable node is pulled from the queue. override to update drawing, data structures, etc.
    virtual void addNode(qnode & node) { }

    // generate a child node from a parent
    virtual void beget(qnode const & parent, qtransform const & t, qnode & child);

    virtual cv::Rect_<float> getBoundingRect() const
    {
        float r = (float)maxRadius;
        return cv::Rect_<float>(-r, -r, 2 * r, 2 * r);
    }

    virtual void drawNode(qcanvas &canvas, qnode const &node);

    virtual void saveImage(fs::path imagePath) { };

    // util
    
#pragma region PRNG

    inline double r()
    {
        std::uniform_real_distribution<double> dist{ 0.0, 1.0 };
        return dist(prng);
    }

    inline double r(double maxVal)
    {
        std::uniform_real_distribution<double> dist{ 0.0, maxVal };
        return dist(prng);
    }

    inline int r(int maxVal)
    {
        std::uniform_int_distribution<int> dist{ 0, maxVal };
        return dist(prng);
    }

    inline cv::Scalar randomColor()
    {
        return util::hsv2bgr(r(360.0), 1.0, 0.5);
    }

#pragma endregion

};


