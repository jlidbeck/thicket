#pragma once

#include <opencv2/core/core.hpp>
#include <vector>


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

        //  Create matrix to apply first scaling, then translation, then rotation about the origin
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

        //  Create transform to map two points using rotation and scaling
        template<typename _Tp>
        cv::Matx<_Tp, 3, 3> getEdgeMap(cv::Point_<_Tp> src0, cv::Point_<_Tp> src1, cv::Point_<_Tp> dest0, cv::Point_<_Tp> dest1)
        {
            cv::Point_<_Tp> srcv = src1 - src0;
            cv::Point_<_Tp> destv = dest1 - dest0;
            auto srcnorm = srcv.dot(srcv);
            auto sc = srcv.dot(destv) / srcnorm;
            auto ss = srcv.cross(destv) / srcnorm;
            auto scrot = cv::Matx<_Tp, 3, 3>(
                sc, -ss, 0,
                ss,  sc, 0,
                0,   0,  1 );
            return getTranslate(dest0.x, dest0.y)
                * scrot
                * getTranslate(-src0.x, -src0.y);
        }


		template<typename _Tp>
		cv::Matx<_Tp, 3, 3> centerAndFit(cv::Rect_<_Tp> const& src, cv::Rect_<_Tp> const &dest, _Tp buffer, bool flipVertical)
		{
			_Tp scale = std::min(dest.width / src.width, dest.height / src.height);
            scale /= (1.0f + buffer);

			// scale, flip vertical, offset

			// 1. translate rect to centered at origin
			// 2. scale rect
			// 3. translate rect to centered on image
			auto a = util::transform3x3::getScaleTranslate(1.0f, -(src.x + src.width / 2.0f), -(src.y + src.height / 2.0f));
            auto b = util::transform3x3::getScale(scale, flipVertical ? -scale : scale);
            auto c = util::transform3x3::getScaleTranslate(1.0f, dest.x + dest.width / 2.0f, dest.y + dest.height / 2.0f);
            //auto d = b*a;
            //d = c * d;
            return c*b*a;
		}
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


    template<typename _Tp>
    cv::Matx<_Tp, 4, 4> colorSink(cv::Matx<_Tp, 4, 1> const &color, _Tp a)
    {
        return cv::Matx<_Tp, 4, 4>(
            1 - a, 0, 0, a*color(0),
            0, 1 - a, 0, a*color(1),
            0, 0, 1 - a, a*color(2),
            0, 0, 0, 1);
    }

    inline cv::Scalar toColor(cv::Matx<float, 4, 1> const &m) { return cv::Scalar(255.0 * m(0), 255.0 * m(1), 255.0 * m(2), 255.0 * m(3)); }

    inline float r()
    {
        return rand() * (1.0f / RAND_MAX);
    }

    inline cv::Matx<float, 4, 1> randomColor()
    {
        float a = r(), b = r();
        switch (rand() % 6)
        {
        case 0: return cv::Matx<float, 4, 1>(a, b, 0, 1);
        case 1: return cv::Matx<float, 4, 1>(a, b, 1, 1);
        case 2: return cv::Matx<float, 4, 1>(b, 0, a, 1);
        case 3: return cv::Matx<float, 4, 1>(b, 1, a, 1);
        case 4: return cv::Matx<float, 4, 1>(0, a, b, 1);
        default:
        case 5: return cv::Matx<float, 4, 1>(1, a, b, 1);
        }
    }

    template<class _T>
    void clear(_T &container)
    {
        container = _T();
    }

}   // namespace util
