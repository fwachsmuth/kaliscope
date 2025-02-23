#include "ReaderPlugin.hpp"

#include <boost/filesystem/operations.hpp>
#include <filesystem.hpp>

namespace tuttle {
namespace plugin {

namespace bfs = boost::filesystem;

ReaderPlugin::ReaderPlugin( OfxImageEffectHandle handle )
	: OFX::ImageEffect( handle )
{
	_clipDst       = fetchClip( kOfxImageEffectOutputClipName );
	_paramFilepath = fetchStringParam( kTuttlePluginFilename );
	_paramBitDepth = fetchChoiceParam( kTuttlePluginBitDepth );
	_paramChannel  = fetchChoiceParam( kTuttlePluginChannel );
	updateSequence();
}

ReaderPlugin::~ReaderPlugin()
{}

void ReaderPlugin::changedParam( const OFX::InstanceChangedArgs& args, const std::string& paramName )
{
	if( paramName == kTuttlePluginFilename )
	{
		updateSequence();
	}
}

void ReaderPlugin::getClipPreferences( OFX::ClipPreferencesSetter& clipPreferences )
{
	// If pattern detected (frame varying on time)
	clipPreferences.setOutputFrameVarying( varyOnTime() );

	switch( getExplicitBitDepthConversion() )
	{
		case eParamReaderBitDepthByte:
		{
			clipPreferences.setClipBitDepth( *this->_clipDst, OFX::eBitDepthUByte );
			break;
		}
		case eParamReaderBitDepthShort:
		{
			clipPreferences.setClipBitDepth( *this->_clipDst, OFX::eBitDepthUShort );
			break;
		}
		case eParamReaderBitDepthAuto:
		case eParamReaderBitDepthFloat:
		{
			clipPreferences.setClipBitDepth( *this->_clipDst, OFX::eBitDepthFloat );
			break;
		}
	}
	switch( getExplicitChannelConversion() )
	{
		case eParamReaderChannelGray:
		{
			clipPreferences.setClipComponents( *this->_clipDst, OFX::ePixelComponentAlpha );
			break;
		}
		case eParamReaderChannelRGB:
		{
			if( OFX::getImageEffectHostDescription()->supportsPixelComponent( OFX::ePixelComponentRGB ) )
				clipPreferences.setClipComponents( *this->_clipDst, OFX::ePixelComponentRGB );
			else
				clipPreferences.setClipComponents( *this->_clipDst, OFX::ePixelComponentRGBA );
			break;
		}
		case eParamReaderChannelAuto:
		case eParamReaderChannelRGBA:
		{
			clipPreferences.setClipComponents( *this->_clipDst, OFX::ePixelComponentRGBA );
			break;
		}
	}

	clipPreferences.setPixelAspectRatio( *this->_clipDst, 1.0 );
}

bool ReaderPlugin::getTimeDomain( OfxRangeD& range )
{
	range.min = getFirstTime();
	range.max = getLastTime();
	TUTTLE_TLOG( TUTTLE_INFO, "[Reader plugin] Time Domain : " << range.min << " to " << range.max );
	return true;
}

void ReaderPlugin::render( const OFX::RenderArguments& args )
{
	std::string filename =  getAbsoluteFilenameAt( args.time );
	TUTTLE_LOG_INFO( "        >-- " << filename );
}

void ReaderPlugin::updateSequence()
{
	std::string path = _paramFilepath->getValue();
	_isSequence      = sequenceParser::browseSequence( _filePattern, path );
	_directory       = bfs::path( path ).parent_path();
	if( _directory.empty() ) // relative path
	{
		_directory = bfs::current_path();
	}
}

}
}
