#ifndef _TUTTLE_OFXTOGIL_COLOR_HPP_
#define	_TUTTLE_OFXTOGIL_COLOR_HPP_

#include <ofxPixels.h>

#include <boost/gil/pixel.hpp>
#include <boost/gil/rgba.hpp>

namespace tuttle {
namespace plugin {

inline boost::gil::pixel<double, boost::gil::rgba_layout_t> ofxToGil( const OfxRGBAColourD& c )
{
	return boost::gil::pixel<double, boost::gil::rgba_layout_t>(c.r, c.g, c.b, c.a);
}

}
}


#endif

