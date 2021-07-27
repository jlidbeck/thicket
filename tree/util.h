#pragma once

#include <nlohmann/json.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/affine.hpp>
#include <vector>
#include <string>


// Operators for OpenCV types

//  rect * float => rect:
//  scales the points (and dimensions) of a rectangle
template<typename _Tp>
cv::Rect_<_Tp> operator*(cv::Rect_<_Tp> const& rc, _Tp scale) 
{
    return cv::Rect_<_Tp>(scale * rc.x, scale * rc.y, scale * rc.width, scale * rc.height);
}

//  rect *= float => rect:
//  scales the points (and dimensions) of a rectangle
template<typename _Tp>
cv::Rect_<_Tp> operator*=(cv::Rect_<_Tp> & rc, _Tp scale) 
{
    rc.x *= scale; 
    rc.y *= scale; 
    rc.width *= scale; 
    rc.height *= scale;
    return rc;
}


namespace util
{

    namespace transform3x3
    {
        template<typename _Tp>
        cv::Matx<_Tp, 3, 3> getTranslate(_Tp translateX = 0, _Tp translateY = 0)
        {
            cv::Matx<_Tp, 3, 3> m = cv::Matx<_Tp, 3, 3>::eye();
            m(0, 2) = translateX;
            m(1, 2) = translateY;
            return m;
        }


        template<typename _Tp>
        cv::Mat_<_Tp> getRotationMatrix2D(cv::Point_<_Tp> center, double angle, double scale, _Tp translateX = 0, _Tp translateY = 0)
        {
            cv::Mat_<_Tp> m = cv::Mat_<_Tp>::eye(3, 3);
            m(cv::Range(0, 2), cv::Range::all()) = cv::getRotationMatrix2D(center, angle, scale);
            m.at<_Tp>(0, 2) += translateX;
            m.at<_Tp>(1, 2) += translateY;
            return m;
        }

        template<typename _Tp>
        cv::Mat_<_Tp> getFlipScaleOffset(double scale, double offX, double offY)
        {
            return (cv::Mat_<_Tp>(3, 3) <<
                scale, 0, offX,
                0, -scale, offY,
                0, 0, 1);
        }

        template<typename _Tp>
        cv::Mat_<_Tp> getRotateFlipScaleOffset(double angle, double scale, double offX, double offY)
        {
            cv::Mat_<_Tp> m = cv::Mat_<_Tp>::eye(3, 3);
            m(cv::Range(0, 2), cv::Range::all()) = cv::getRotationMatrix2D(cv::Point_<_Tp>(1.0, 0.0), angle, scale);

            m.at<_Tp>(1, 1) = -m.at<_Tp>(1, 1);
            m.at<_Tp>(0, 2) = offX;
            m.at<_Tp>(1, 2) = offY;
            return m;
        }

        // Matx

        //  2D rotation about the origin
        template<typename _Tp>
        cv::Matx<_Tp, 3, 3> getRotate(_Tp angle)
        {
            _Tp c = cos(angle);
            _Tp s = sin(angle);
            return cv::Matx<_Tp, 3, 3>( c,-s, 0,
                                        s, c, 0,
                                        0, 0, 1 );
        }

        //  Create matrix to apply scaling
        template<typename _Tp>
        cv::Matx<_Tp, 3, 3> getScale(_Tp scaleX, _Tp scaleY)
        {
            return cv::Matx<_Tp, 3, 3>(
                scaleX, 0, 0,
                0, scaleY, 0,
                0, 0,      1 );
        }

        //  Create matrix to apply first scaling, then translation
        template<typename _Tp>
        cv::Matx<_Tp, 3, 3> getScaleTranslate(_Tp scale, _Tp translateX, _Tp translateY)
        {
            return cv::Matx<_Tp, 3, 3>( scale, 0, translateX,
                                        0, scale, translateY,
                                        0,     0, 1 );
        }

        //  Create transform matrix to map two source points to two target points using *rotation* and scaling.
        //  Generates an orthogonal 3x3 transform matrix that maps src0 to dest0 and src1 to dest1, 
        //  with a positive or zero determinant.
        template<typename _Tp>
        cv::Matx<_Tp, 3, 3> getEdgeMap(cv::Point_<_Tp> src0, cv::Point_<_Tp> src1, cv::Point_<_Tp> dest0, cv::Point_<_Tp> dest1)
        {
            cv::Point_<_Tp> srcv = src1 - src0;
            cv::Point_<_Tp> destv = dest1 - dest0;
            auto srcnorm = srcv.ddot(srcv);
            _Tp sc = (_Tp)(srcv.ddot( destv) / srcnorm);
            _Tp ss = (_Tp)(srcv.cross(destv) / srcnorm);
            auto mScaleRotate = cv::Matx<_Tp, 3, 3>(
                sc, -ss, 0,
                ss,  sc, 0,
                0,   0,  1 );
            return getTranslate(dest0.x, dest0.y)
                * mScaleRotate
                * getTranslate(-src0.x, -src0.y);
        }


        //  Create transform matrix to map two source points to two target points using *reflection* and scaling.
        //  Generates an orthogonal 3x3 transform matrix that maps src0 to dest0 and src1 to dest1, 
        //  with a negative or zero determinant.
        template<typename _Tp>
        cv::Matx<_Tp, 3, 3> getMirroredEdgeMap(cv::Point_<_Tp> src0, cv::Point_<_Tp> src1, cv::Point_<_Tp> dest0, cv::Point_<_Tp> dest1)
        {
            cv::Point_<_Tp> srcv = src1 - src0;
            cv::Point_<_Tp> destv = dest1 - dest0;
            auto srcnorm = srcv.dot(srcv);
            auto sc = (srcv.x*destv.x - srcv.y*destv.y) / srcnorm;
            auto ss = (srcv.x*destv.y + srcv.y*destv.x) / srcnorm;
            auto mScaleMirror = cv::Matx<_Tp, 3, 3>(
                sc,  ss, 0,
                ss, -sc, 0,
                0,   0,  1 );
            return getTranslate(dest0.x, dest0.y)
                * mScaleMirror
                * getTranslate(-src0.x, -src0.y);
        }


        //  Create 2D transform matrix to map {src} rectangle to fit within {dest},
        //  keeping the scale as large as possible while maintaining aspect ratio.
        //  buffer: relative to {src} coords, minimum extra padding around all sides of {src}.
        //  flipVertical: if set, {src} orientation is flipped.
		template<typename _Tp>
		cv::Matx<_Tp, 3, 3> centerAndFit(cv::Rect_<_Tp> const& src, cv::Rect_<_Tp> const &dest, _Tp buffer, bool flipVertical)
		{
			_Tp scale = std::min(dest.width / src.width, dest.height / src.height);
            scale /= (1.0f + buffer);

			// scale, flip vertical, offset

			// 1. translate {src} to centered at origin
			// 2. scale {src}
			// 3. translate {src} to be centered on {dest}
			auto a = util::transform3x3::getScaleTranslate(1.0f, -(src.x + src.width / 2.0f), -(src.y + src.height / 2.0f));
            auto b = util::transform3x3::getScale(scale, flipVertical ? -scale : scale);
            auto c = util::transform3x3::getScaleTranslate(1.0f, dest.x + dest.width / 2.0f, dest.y + dest.height / 2.0f);
            //auto d = b*a;
            //d = c * d;
            return c*b*a;
		}
	}

    template<typename _Tp, int m, int n>
    bool approximatelyEqual(cv::Matx<_Tp, m, n> const &m1, cv::Matx<_Tp, m, n> const &m2, _Tp epsilon=0.0001)
    {
        double min, max;
        cv::minMaxLoc(cv::abs(cv::Mat(m1 - m2)), &min, &max);
        return (max < epsilon);
    }

    template<typename _Tp>
    cv::Rect_<_Tp> getBoundingRect(std::vector<cv::Point_<_Tp> > const &pts)
    {
        _Tp l = pts[0].x, t = pts[0].y, r = pts[0].x, b = pts[0].y;
        for (auto const &pt : pts)
        {
            if (pt.x < l) l = pt.x;
            else if (pt.x > r) r = pt.x;
            if (pt.y < t) t = pt.y;
            else if (pt.y > b) b = pt.y;
        }
        return cv::Rect_<_Tp>(l, t, r - l, b - t);
    }


    //  Expands {rect} to contain all points in {pts}.
    template<typename _Tp>
    cv::Rect_<_Tp> getBoundingRect(cv::Rect_<_Tp> rect, std::vector<cv::Point_<_Tp> > const &pts)
    {
        _Tp r = rect.br().x, b = rect.br().y;
        for (auto const &pt : pts)
        {
            if (pt.x < rect.x) rect.x = pt.x;
            else if (pt.x > r) r = pt.x;
            if (pt.y < rect.y) rect.y = pt.y;
            else if (pt.y > b) b = pt.y;
        }
        rect.width = r - rect.x;
        rect.height = b - rect.y;
        return rect;
    }



    //  Assumes provided Scalar is in BGR order and scaled 0..255; returns string as "RRGGBB"
    inline std::string toRgbHexString(cv::Scalar bgr)
    {
        // temp patch
        if ((bgr(0) > 0 || bgr(1) > 0 || bgr(2) > 0) && (bgr(0) <= 1) && (bgr(1) <= 1) && (bgr(2) <= 1))
        {
            bgr *= 255.0;
        }

        char str[8];
        sprintf_s(str, "%02x%02x%02x",
            (uchar)(0.5 + bgr(2)),
            (uchar)(0.5 + bgr(1)),
            (uchar)(0.5 + bgr(0))
        );
        return str;
    }

    //  Assumes provided string is "RRGGBB"; returns Scalar in BGR order
    inline cv::Scalar fromRgbHexString(char const * rgbString)
    {
        uint32_t rgb;
        sscanf_s(rgbString, "%x", &rgb);
        //temp patch
        if (rgb && ((rgb & 0x01010101) == rgb))
        {
            rgb *= 0xFF;
        }
        return cv::Scalar(rgb & 0xFF, (rgb >> 8) & 0xFF, (rgb >> 16) & 0xFF);
    }


    template<typename _Tp>
    cv::Matx<_Tp, 4, 4> scaleAndTranslate(_Tp sx, _Tp sy, _Tp sz, _Tp tx, _Tp ty, _Tp tz)
    {
        return cv::Matx<_Tp, 4, 4>(
            sx, 0, 0, tx,
            0, sy, 0, ty,
            0, 0, sz, tz,
            0, 0, 0,  1);
    }


    //  Create a 3D transform matrix whose fixed point is {color}.
    //  As long as 0<{a}<=1, points transformed by this matrix move toward {color}.
    template<typename _Tp>
    cv::Matx<_Tp, 4, 4> colorSink(cv::Matx<_Tp, 4, 1> const &color, _Tp a)
    {
        return cv::Matx<_Tp, 4, 4>(
            1 - a, 0, 0, a*color(0),
            0, 1 - a, 0, a*color(1),
            0, 0, 1 - a, a*color(2),
            0, 0, 0, 1);
    }

    //  Create a 3D transform matrix whose fixed point is {color}.
    //  As long as 0<{a}<=1, points transformed by this matrix move toward {color}.
    //  Since Scalar is derived from Matx<_tp, 4, 1>, this extra polymorphism shouldn't be necessary
    template<typename _Tp>
    cv::Matx<_Tp, 4, 4> colorSink(cv::Scalar_<_Tp> const &color, _Tp a)
    {
        return cv::Matx<_Tp, 4, 4>(
            1 - a, 0, 0, a*color(0),
            0, 1 - a, 0, a*color(1),
            0, 0, 1 - a, a*color(2),
            0, 0, 0, 1);
    }

    //  Create a 3D transform matrix whose fixed point is (b, g, r).
    //  As long as 0<{a}<=1, points transformed by this matrix move toward the fixed point.
    template<typename _Tp>
    cv::Matx<_Tp, 4, 4> colorSink(_Tp b, _Tp g, _Tp r, _Tp a)
    {
        return cv::Matx<_Tp, 4, 4>(
            1 - a, 0, 0, a*b,
            0, 1 - a, 0, a*g,
            0, 0, 1 - a, a*r,
            0, 0, 0, 1);
    }

    template<typename _Tp>
    cv::Matx<_Tp, 4, 4> hsvTransform(_Tp hueShift, _Tp satScale, _Tp valScale)
    {
        return cv::Matx<_Tp, 4, 4>(
            1, 0,        0,         hueShift,
            0, satScale, 0,         0,
            0, 0,        valScale,  0,
            0, 0,        0,         1);
    }


#pragma region HSV

    // h: 0.0-360.0; s: 0.0-1.0; v: 0.0-1.0

    inline cv::Scalar cvtColor(cv::Scalar const &v, cv::ColorConversionCodes cvtCode)
    {
        cv::Mat3f mat(1, 1, cv::Vec3f((float)v(0), (float)v(1), (float)v(2)));
        cv::cvtColor(mat, mat, cvtCode);
        auto const &p = mat(0, 0);
        return cv::Scalar(p(0), p(1), p(2), 1.0);
    }

    template<typename _Tp>
    inline cv::Scalar hsv2bgr(_Tp h, _Tp s, _Tp v)
    {
        cv::Mat3f mat(1, 1, cv::Vec3f((float)h, (float)s, (float)v));
        cv::cvtColor(mat, mat, cv::ColorConversionCodes::COLOR_HSV2BGR);
        return (mat(0, 0));
    }

    template<typename _Tp>
    inline cv::Scalar bgr2hsv(_Tp b, _Tp g, _Tp r)
    {
        cv::Mat3f mat(1, 1, cv::Vec3f((float)b, (float)g, (float)r));
        cv::cvtColor(mat, mat, cv::ColorConversionCodes::COLOR_BGR2HSV);
        return (mat(0, 0));
    }

#pragma endregion

    template<class _Class>
    void clear(_Class &container)
    {
        container = _Class();
    }

    namespace polygon
    {
        template<typename _Tp>
        cv::Point_<_Tp> centroid(std::vector<cv::Point_<_Tp> > &polygon)
        {
            cv::Point_<_Tp> sum;
            for (auto const &pt : polygon)
                sum += pt;
            return sum / (_Tp)polygon.size();
        }

        template<typename _Tp>
        cv::Point_<_Tp> headingStep(_Tp angleDegrees)
        {
            _Tp a = (_Tp)(CV_PI * angleDegrees / 180.0);

            return cv::Point_<_Tp>(cos(a), sin(a));
        }

        //  creates regular polygon with vertices at unit distance from the origin
        template<typename _Tp>
        void createRegularCenteredPolygon(std::vector<cv::Point_<_Tp> > &polygon, int polygonSides)
        {
            polygon.clear();
            for (int i = 0; i < polygonSides; i++)
            {
                float angle = ((float)i - 0.5f) * CV_2PI / polygonSides;
                polygon.push_back(cv::Point2f(sin(angle), cos(angle)));
            }
        }

        //  creates regular polygon with side length 1 and first edge from (0,0) to (1,0)
        template<typename _Tp>
        void createRegularPolygon(std::vector<cv::Point_<_Tp> > &polygon, int polygonSides)
        {
            polygon.clear();
            cv::Point_<_Tp> pt(0, 0);
            for (int i = 0; i < polygonSides; i++)
            {
                polygon.push_back(pt);
                pt += headingStep((_Tp)(i * 360.0 / polygonSides));
            }
        }

        //  creates regular star with {n} points of angle {a} and side length 1
        template<typename _Tp>
        void createStar(std::vector<cv::Point_<_Tp> > &polygon, int n, float a)
        {
            polygon.clear();
            cv::Point2f pt(0, 0);
            _Tp hdg = 0.0f;
            a = 180.0f - a;
            for (int i = 0; i < n; ++i)
            {
                polygon.push_back(pt += headingStep(hdg += a));
                polygon.push_back(pt += headingStep(hdg += (360.0f / n - a)));
            }
        }

        //  creates regular 5-pointed star with side length 1
        template<typename _Tp>
        void createStar(std::vector<cv::Point_<_Tp> > &polygon)
        {
            createStar(polygon, 5, 36.0f);
        }
    }

}   // namespace util


#pragma region Serialization

using json = nlohmann::basic_json<>;

template<typename _Tp>
void to_json(json &j, cv::Scalar_<_Tp> const &color)
{
    j = util::toRgbHexString(255.0 * color);
}

template<typename _Tp>
void from_json(json const &j, cv::Scalar_<_Tp> &color)
{
    if (j.is_array())
    {
        throw(std::exception("todo"));
    }
    if (j.is_string())
    {
        color = util::fromRgbHexString(j.get<std::string>().c_str()) / 255.0;
    }
    else
    {
        throw(std::exception("not convertible to color"));
    }
}

// serialize vector of points

template<typename _Tp>
void to_json(json &j, std::vector<cv::Point_<_Tp> > const &polygon)
{
    j = nlohmann::json::array();
    for (auto &pt : polygon)
    {
        j.push_back(pt.x);
        j.push_back(pt.y);
    }
}

template<typename _Tp>
void from_json(json const &j, std::vector<cv::Point_<_Tp> > &polygon)
{
    if (!j.is_array())
        throw(std::exception("Not JSON array type"));

    polygon.clear();
    polygon.reserve(j.size());

    for (int i = 0; i < j.size(); i += 2)
        polygon.push_back(cv::Point2f(j[i], j[i + 1]));
}

// serialize M x N matrix

template<typename _Tp, int m, int n>
void to_json(json &j, cv::Matx<_Tp, m, n> const &mat)
{
    j = nlohmann::json::array();
    for (int r = 0; r < m; ++r)
        for (int c = 0; c < n; ++c)
            j[r].push_back((float)mat(r, c));
    //sz += std::to_string(mat(r, j)) + (j < n - 1 ? ", " : r < n - 1 ? ",\n  " : "\n  ]");
}

template<typename _Tp, int m, int n>
void from_json(json const &j, cv::Matx<_Tp, m, n> &mat)
{
    for (int r = 0; r < m; ++r)
        for (int c = 0; c < n; ++c)
            mat(r, c) = j[r][c];
    //sz += std::to_string(mat(r, j)) + (j < n - 1 ? ", " : r < n - 1 ? ",\n  " : "\n  ]");
}

template<typename _Tp, int m, int n>
void from_json(json const &j, std::vector<cv::Matx<_Tp, m, n> > &mats)
{
    mats.resize(j.size());
    for (int i = 0; i < j.size(); ++i)
        from_json(j[i], mats[i]);
}

#pragma endregion

