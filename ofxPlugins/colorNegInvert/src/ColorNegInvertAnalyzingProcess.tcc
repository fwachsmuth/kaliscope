/* Copyright (C) 2015 Eloi DU BOIS - All Rights Reserved
 * The license for this file is available here:
 * https://github.com/edubois/kaliscope/blob/master/LICENSE
 */

#include "ColorNegInvertAlgorithm.hpp"
#include "ColorNegInvertPlugin.hpp"

#include <boost/gil.hpp>
#include <terry/colorspace/layout/all.hpp>

typedef boost::gil::scoped_channel_value<float, boost::gil::float_point_zero<float>, boost::gil::float_point_one<float>> bits32f;

namespace tuttle {
namespace plugin {
namespace colorNegInvert {

template<class View>
ColorNegInvertAnalyzingProcess<View>::ColorNegInvertAnalyzingProcess( ColorNegInvertPlugin &effect )
: ImageGilFilterProcessor<View>( effect, eImageOrientationIndependant )
, _plugin( effect )
, _redFilterColor( 0.0 )
, _greenFilterColor( 0.0 )
, _blueFilterColor( 0.0 )
{
}

template<class View>
void ColorNegInvertAnalyzingProcess<View>::setup( const OFX::RenderArguments& args )
{
    ImageGilFilterProcessor<View>::setup( args );
    _params = _plugin.getProcessParams( args.renderScale );
    _redFilterColor = 0.0;
    _greenFilterColor = 0.0;
    _blueFilterColor = 0.0;
}

/**
 * @brief Function called by rendering thread each time a process must be done.
 * @param[in] procWindowRoW  Processing window
 */
template<class View>
void ColorNegInvertAnalyzingProcess<View>::multiThreadProcessImages( const OfxRectI& procWindowRoW )
{
    using namespace boost::gil;
    OfxRectI procWindowOutput = this->translateRoWToOutputClipCoordinates( procWindowRoW );
    const OfxPointI procWindowSize = {
            procWindowRoW.x2 - procWindowRoW.x1,
            procWindowRoW.y2 - procWindowRoW.y1 };

    using namespace terry::color::layout;
    typedef pixel<bits32f, boost::gil::layout< terry::color::layout::yuv_t> > YUVWorkPixT;
    YUVWorkPixT wpix;
    rgb32f_pixel_t wfpix;

    double yMax = 0.0;
    for( int y = procWindowOutput.y1; y < procWindowOutput.y2; ++y )
    {
        typename View::x_iterator src_it = this->_srcView.x_at( procWindowOutput.x1, y );
        typename View::x_iterator dst_it = this->_dstView.x_at( procWindowOutput.x1, y );
        for( int x = procWindowOutput.x1; x < procWindowOutput.x2; ++x, ++src_it, ++dst_it )
        {
            color_convert( *src_it, wpix );
            const double y = get_color( wpix, y_t() );
            if ( y > yMax )
            {
                yMax = y;
                color_convert( *src_it, wfpix );
            }
            color_convert( *src_it, *dst_it );
        }
        if( this->progressForward( procWindowSize.x ) )
            return;
    }

    const double redFilterColor = get_color( wfpix, red_t() );
    const double greenFilterColor = get_color( wfpix, green_t() );
    const double blueFilterColor = get_color( wfpix, blue_t() );
    if ( redFilterColor > _redFilterColor )
    {
        _redFilterColor = redFilterColor;
    }
    if ( greenFilterColor > _greenFilterColor )
    {
        _greenFilterColor = greenFilterColor;
    }
    if ( blueFilterColor > _blueFilterColor )
    {
        _blueFilterColor = blueFilterColor;
    }
}

template<class View>
void ColorNegInvertAnalyzingProcess<View>::postProcess()
{
    this->progressEnd();
    _plugin.notifyRGBFilterColor( _redFilterColor, _greenFilterColor, _blueFilterColor );
}

}
}
}
