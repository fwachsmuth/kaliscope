/* Copyright (C) 2015 Eloi DU BOIS - All Rights Reserved
 * The license for this file is available here:
 * https://github.com/edubois/kaliscope/blob/master/LICENSE
 */

#define OFXPLUGIN_VERSION_MAJOR 1
#define OFXPLUGIN_VERSION_MINOR 1

#include "ColorNegInvertPluginFactory.hpp"
#include <tuttle/plugin/Plugin.hpp>

namespace OFX {
namespace Plugin {

void getPluginIDs( OFX::PluginFactoryArray& ids )
{
    mAppendPluginFactory( ids, tuttle::plugin::colorNegInvert::ColorNegInvertPluginFactory, "fr.tuttle.djarlabs.colorneginvert" );
}

}
}

