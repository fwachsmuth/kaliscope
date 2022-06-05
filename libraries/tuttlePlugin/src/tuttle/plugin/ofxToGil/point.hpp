#ifndef _TUTTLE_OFXTOGIL_POINT_HPP_
#define	_TUTTLE_OFXTOGIL_POINT_HPP_

#include <ofxCore.h>

#include <boost/gil/point.hpp>

namespace tuttle {
namespace plugin {

inline boost::gil::point<double> ofxToGil( const OfxPointD& p )
{
	return boost::gil::point<double>( p.x, p.y );
}

inline OfxPointD gilToOfx( const boost::gil::point<double>& p )
{
	OfxPointD r = { p.x, p.y };

	return r;
}

inline boost::gil::point<int> ofxToGil( const OfxPointI& p )
{
	return boost::gil::point<int>( p.x, p.y );
}

inline OfxPointI gilToOfx( const boost::gil::point<int>& p )
{
	OfxPointI r = { p.x, p.y };

	return r;
}

}
}


#endif

