#ifndef _ofxsImageEffect_H_
#define _ofxsImageEffect_H_
/*
 * OFX Support Library, a library that skins the OFX plug-in API with C++ classes.
 * Copyright (C) 2004-2005 The Open Effects Association Ltd
 * Author Bruno Nicoletti bruno@thefoundry.co.uk
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * Neither the name The Open Effects Association Ltd, nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The Open Effects Association Ltd
 * 1 Wardour St
 * London W1D 6PA
 * England
 *
 *
 *
 */

/** @file This file contains core code that wraps OFX 'objects' with C++ classes.
 *
 * This file only holds code that is visible to a plugin implementation, and so hides much
 * of the direct OFX objects and any library side only functions.
 */
#include "ofxsParam.h"
#include "ofxsInteract.h"
#include "ofxsMessage.h"
#include "ofxParametricParam.h"
#include "extensions/nuke/camera.h"

#include <ofxProgress.h>
#include <ofxTimeLine.h>

#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

/** @brief Nasty macro used to define empty protected copy ctors and assign ops */
#define mDeclareProtectedAssignAndCC( CLASS ) \
    CLASS& operator=( const CLASS& ) { assert( false ); return *this; }    \
    CLASS( const CLASS & ) { assert( false ); }

namespace OFX {
namespace Private {

OfxStatus mainEntryStr( const char*          actionRaw,
                        const void*          handleRaw,
                        OfxPropertySetHandle inArgsRaw,
                        OfxPropertySetHandle outArgsRaw,
                        const char*          plugname );
}
}

/**
 * @brief The core 'OFX Support' namespace, used by plugin implementations. All code for these are defined in the common support libraries.
 */
namespace OFX {
/** forward class declarations */
struct tag_ofxStatus;

class ClipDescriptor;
class ImageEffectDescriptor;

class Image;
class Clip;
class ImageEffect;
class ImageMemory;

/** @brief Enumerates the contexts a plugin can be used in */
enum EContext
{
    eContextNone,
    eContextGenerator,
    eContextFilter,
    eContextTransition,
    eContextPaint,
    eContextGeneral,
    eContextRetimer,
    eContextReader,
    eContextWriter,
};

const std::string mapContextEnumToString( const EContext s );

/** @brief Enumerates the pixel depths supported */
enum EBitDepth
{
    eBitDepthCustom = -1, ///< some non standard bit depth
    eBitDepthNone = 0, ///< bit depth that indicates no data is present
    eBitDepthUByte = 1,
    eBitDepthUShort = 2,
    eBitDepthFloat = 3
};

const std::string mapBitDepthEnumToString( const EBitDepth e );

/** @brief Enumerates the component types supported */
enum EPixelComponent
{
    ePixelComponentNone,
    ePixelComponentRGBA,
    ePixelComponentRGB,
    ePixelComponentAlpha,
    ePixelComponentCustom ///< some non standard pixel type
};

std::string mapPixelComponentEnumToString( const EPixelComponent e );

/** @brief Enumerates the ways a fielded image can be extracted from a clip */
enum EFieldExtraction
{
    eFieldExtractBoth,   /**< @brief extract both fields */
    eFieldExtractSingle, /**< @brief extracts a single field, so you have a half height image */
    eFieldExtractDoubled /**< @brief extracts a single field, but doubles up the field, so you have a full height image */
};

/** @brief Enumerates the kind of render thread safety a plugin has */
enum ERenderSafety
{
    eRenderUnsafe,       /**< @brief can only render a single instance at any one time */
    eRenderInstanceSafe, /**< @brief can call a single render on an instance, but can render multiple instances simultaneously */
    eRenderFullySafe     /**< @brief can call render any number of times on an instance, and render multiple instances simultaneously */
};

/** @brief Enumerates the fields present in an image */
enum EField
{
    eFieldNone,   /**< @brief unfielded image */
    eFieldBoth,   /**< @brief fielded image with both fields present */
    eFieldLower,  /**< @brief only the spatially lower field is present */
    eFieldUpper   /**< @brief only the spatially upper field is present  */
};

std::string mapFieldEnumToString( const EField e );

enum EPreMultiplication
{
    eImageOpaque,          /**< @brief the image is opaque and so has no premultiplication state */
    eImagePreMultiplied,   /**< @brief the image is premultiplied by it's alpha */
    eImageUnPreMultiplied, /**< @brief the image is unpremultiplied */
};

std::string mapPreMultiplicationEnumToString( const EPreMultiplication e );


class PluginFactory
{
public:
    virtual ~PluginFactory() {}
    virtual void                 load()   {}
    virtual void                 unload() {}
    virtual void                 describe( OFX::ImageEffectDescriptor& desc )                               = 0;
    virtual void                 describeInContext( OFX::ImageEffectDescriptor& desc, EContext context ) = 0;
    virtual ImageEffect*         createInstance( OfxImageEffectHandle handle, EContext context )         = 0;
    virtual const std::string&   getID() const                                                              = 0;
    virtual const std::string&   getUID() const                                                             = 0;
    virtual unsigned int         getMajorVersion() const                                                    = 0;
    virtual unsigned int         getMinorVersion() const                                                    = 0;
    virtual OfxPluginEntryPoint* getMainEntry()                                                             = 0;
};

template<class FACTORY>
class FactoryMainEntryHelper
{
protected:
    const std::string& getHelperID() const           { return _id; }
    unsigned int       getHelperMajorVersion() const { return _maj; }
    unsigned int       getHelperMinorVersion() const { return _min; }

    FactoryMainEntryHelper( const std::string& id, const unsigned int maj, const unsigned int min )
    : _id( id )
    , _maj( maj )
    , _min( min )
    {
		std::ostringstream ss;
		ss << id << "." << maj << "." << min;
        _uid = ss.str();
    }

    const std::string& getHelperUID() const { return _uid; }
    static OfxStatus   mainEntry( const char* action, const void* handle, OfxPropertySetHandle in, OfxPropertySetHandle out )
    {
        return OFX::Private::mainEntryStr( action, handle, in, out, _uid.c_str() );
    }

    static std::string _uid;
    std::string _id;
    unsigned int _maj;
    unsigned int _min;
};

template<class T>
std::string OFX::FactoryMainEntryHelper<T>::_uid;

template<class FACTORY>
class PluginFactoryHelper : public FactoryMainEntryHelper<FACTORY>,
    public PluginFactory
{
public:
    PluginFactoryHelper( const std::string& id, unsigned int maj, unsigned int min ) : FactoryMainEntryHelper<FACTORY>( id, maj, min )
    {}
    virtual ~PluginFactoryHelper() {}
    OfxPluginEntryPoint* getMainEntry()          { return FactoryMainEntryHelper<FACTORY>::mainEntry; }
    const std::string&   getID() const           { return FactoryMainEntryHelper<FACTORY>::getHelperID(); }
    const std::string&   getUID() const          { return FactoryMainEntryHelper<FACTORY>::getHelperUID(); }
    unsigned int         getMajorVersion() const { return FactoryMainEntryHelper<FACTORY>::getHelperMajorVersion(); }
    unsigned int         getMinorVersion() const { return FactoryMainEntryHelper<FACTORY>::getHelperMinorVersion(); }
};

#define mDeclarePluginFactory( CLASS, LOADFUNCDEF, UNLOADFUNCDEF ) \
    class CLASS : public OFX::PluginFactoryHelper < CLASS > \
    { \
    public: \
        CLASS( const std::string & id, unsigned int verMaj, unsigned int verMin ) : OFX::PluginFactoryHelper < CLASS > ( id, verMaj, verMin ) {} \
        virtual ~CLASS() {} \
        virtual void load() LOADFUNCDEF ; \
        virtual void unload() UNLOADFUNCDEF ; \
        virtual void describe( OFX::ImageEffectDescriptor & desc ); \
        virtual void describeInContext( OFX::ImageEffectDescriptor & desc, OFX::EContext context ); \
        virtual OFX::ImageEffect* createInstance( OfxImageEffectHandle handle, OFX::EContext context ); \
    };

typedef std::vector<PluginFactory*> PluginFactoryArray;

/** @brief Fetch's a suite from the host and logs errors
 *
 * All the standard suites are fetched by the support code, you should use this
 * to fetch any extra non-standard suites.
 */
void* fetchSuite( const char* suiteName, int suiteVersion, bool optional = false );

////////////////////////////////////////////////////////////////////////////////
/** @brief A class that lists all the properties of a host */
struct ImageEffectHostDescription
{
public:
    std::string hostName;
    std::string hostLabel;
    bool hostIsBackground;
    bool supportsOverlays;
    bool supportsMultiResolution;
    bool supportsTiles;
    bool temporalClipAccess;
    bool supportsMultipleClipDepths;
    bool supportsMultipleClipPARs;
    bool supportsSetableFrameRate;
    bool supportsSetableFielding;
    bool supportsStringAnimation;
    bool supportsCustomInteract;
    bool supportsChoiceAnimation;
    bool supportsBooleanAnimation;
    bool supportsCustomAnimation;
    bool supportsParametricParameter;
    bool supportsCameraParameter;
    int maxParameters;
    int maxPages;
    int pageRowCount;
    int pageColumnCount;
    typedef std::vector<EPixelComponent> PixelComponentArray;
    PixelComponentArray _supportedComponents;
    typedef std::vector<EContext> ContextArray;
    ContextArray _supportedContexts;
    typedef std::vector<EBitDepth> BitDepthArray;
    BitDepthArray _supportedPixelDepths;
    bool supportsProgressSuite;
    bool supportsTimeLineSuite;

public:
    bool supportsPixelComponent( const OFX::EPixelComponent component ) const
    {
        return std::find( _supportedComponents.begin(), _supportedComponents.end(), component ) != _supportedComponents.end();
    }
    bool supportsBitDepth( const OFX::EBitDepth bitDepth ) const
    {
        return std::find( _supportedPixelDepths.begin(), _supportedPixelDepths.end(), bitDepth ) != _supportedPixelDepths.end();
    }
    bool supportsContext( const OFX::EContext context ) const
    {
        return std::find( _supportedContexts.begin(), _supportedContexts.end(), context ) != _supportedContexts.end();
    }
	
    /** @return default pixel depth supported by host application. */
    EBitDepth getDefaultPixelDepth() const
    {
        if( ! _supportedPixelDepths.empty() )
        {
            return _supportedPixelDepths[0];
        }
        else
        {
            OFXS_COUT_WARNING("The host doesn't define supported pixel depth. (size: " << _supportedPixelDepths.size() << ")" );
            return eBitDepthFloat;
        }
    }
	
    /** @return default pixel component supported by host application. */
    EPixelComponent getDefaultPixelComponent() const
    {
        if( ! _supportedComponents.empty() )
        {
            return _supportedComponents[0];
        }
        else
        {
            OFXS_COUT_WARNING("The host doesn't define supported pixel component. (size: " << _supportedComponents.size() << ")" );
            return ePixelComponentRGBA;
        }
    }
};

/// retrieve the host description
ImageEffectHostDescription* getImageEffectHostDescription();

////////////////////////////////////////////////////////////////////////////////
/** @brief Wraps up a clip */
class ClipDescriptor
{
protected:
    mDeclareProtectedAssignAndCC( ClipDescriptor );
    ClipDescriptor( void ) { assert( false ); }

protected:
    /** @brief name of the clip */
    std::string _clipName;

    /** @brief properties for this clip */
    PropertySet _clipProps;

protected:
    /** @brief hidden constructor */
    ClipDescriptor( const std::string& name, OfxPropertySetHandle props );

    friend class ImageEffectDescriptor;

public:
    const PropertySet& getPropertySet() const { return _clipProps; }

    PropertySet& getPropertySet() { return _clipProps; }

    /** @brief set the label properties */
    void setLabels( const std::string& label, const std::string& shortLabel, const std::string& longLabel );
    void setLabel( const std::string& label ) { setLabels( label, label, label ); }

    /** @brief set how fielded images are extracted from the clip defaults to eFieldExtractDoubled */
    void setFieldExtraction( EFieldExtraction v );

    /** @brief set which components are supported, defaults to none set, this must be called at least once! */
    void addSupportedComponent( EPixelComponent v );

    /** @brief set which components are supported. This version adds by the raw C-string label, allowing you to add
     * custom component types */
    void addSupportedComponent( const std::string& comp );

    /** @brief say whether we are going to do random temporal access on this clip, defaults to false */
    void setTemporalClipAccess( bool v );

    /** @brief say whether if the clip is optional, defaults to false */
    void setOptional( bool v );

    /** @brief say whether this clip supports tiling, defaults to true */
    void setSupportsTiles( bool v );

    /** @brief say whether this clip is a 'mask', so the host can know to replace with a roto or similar, defaults to false */
    void setIsMask( bool v );
};

////////////////////////////////////////////////////////////////////////////////
/** @brief Wraps up an effect descriptor, used in the describe actions */
class ImageEffectDescriptor : public ParamSetDescriptor
{
protected:
    mDeclareProtectedAssignAndCC( ImageEffectDescriptor );
    ImageEffectDescriptor( void ) { assert( false ); }

protected:
    /** @brief The effect handle */
    OfxImageEffectHandle _effectHandle;

    /** @brief properties for this clip */
    PropertySet _effectProps;

    /** @brief Set of all previously defined parameters, defined on demand */
    std::map<std::string, ClipDescriptor*> _definedClips;

    /** @brief Set of strings for clip preferences action (stored in here so the array persists and can be used in a property name)*/
    std::map<std::string, std::string> _clipComponentsPropNames;
    std::map<std::string, std::string> _clipDepthPropNames;
    std::map<std::string, std::string> _clipPARPropNames;
    std::map<std::string, std::string> _clipROIPropNames;
    std::map<std::string, std::string> _clipFrameRangePropNames;

    std::unique_ptr<EffectInteractWrap> _overlayDescriptor;

public:
    /** @brief ctor */
    ImageEffectDescriptor( OfxImageEffectHandle handle );

    /** @brief dtor */
    ~ImageEffectDescriptor();

    const PropertySet& getPropertySet() const { return _effectProps; }

    PropertySet& getPropertySet() { return _effectProps; }

    OfxImageEffectHandle getImageEffectHandle() { return _effectHandle; }

    /** @brief, set the label properties in a plugin */
    void setLabels( const std::string& label, const std::string& shortLabel, const std::string& longLabel );
    void setLabel( const std::string& label ) { setLabels(label, label, label); }

    void setDescription( const std::string& description );

    /** @brief Set the plugin grouping, defaults to "" */
    void setPluginGrouping( const std::string& group );

    /** @brief Add a context to those supported, defaults to none, must be called at least once */
    void addSupportedContext( EContext v );

    /** @brief Add a pixel depth to those supported, defaults to none, must be called at least once */
    void addSupportedBitDepth( EBitDepth v );

    /** @brief Add a file extension to those supported, defaults to none */
    void addSupportedExtension( const std::string& extension );
    void addSupportedExtensions( const std::vector<std::string>& extensions );

    void setPluginEvaluation( double evaluation );
    
    /** @brief Is the plugin single instance only ? defaults to false */
    void setSingleInstance( bool v );

    /** @brief Does the plugin expect the host to perform per frame SMP threading defaults to true */
    void setHostFrameThreading( bool v );

    /** @brief Does the plugin support multi resolution images, defaults to true */
    void setSupportsMultiResolution( bool v );

    /** @brief Does the plugin support image tiling, defaults to true */
    void setSupportsTiles( bool v );

    /** @brief Does the plugin perform temporal clip access, defaults to false */
    void setTemporalClipAccess( bool v );

    /** @brief Does the plugin want to have render called twice per frame in all circumanstances for fielded images ? defaults to true */
    void setRenderTwiceAlways( bool v );

    /** @brief Does the plugin support inputs and output clips of differing depths, defaults to false */
    void setSupportsMultipleClipDepths( bool v );

    /** @brief Does the plugin support inputs and output clips of pixel aspect ratios, defaults to false */
    void setSupportsMultipleClipPARs( bool v );

    /** @brief How thread safe is the plugin, defaults to eRenderInstanceSafe */
    void setRenderThreadSafety( ERenderSafety v );

    /** @brief If the slave  param changes the clip preferences need to be re-evaluated */
    void addClipPreferencesSlaveParam( ParamDescriptor& p );

    /** @brief Create a clip, only callable from describe in context
     *
     * The returned clip \em must not be deleted by the client code. This is all managed by the ImageEffectDescriptor itself.
     */
    ClipDescriptor* defineClip( const std::string& name );

    /** @brief Access to the string maps needed for runtime properties. Because the char array must persist after the call,
     * we need these to be stored in the descriptor, which is only deleted on unload.*/

    const std::map<std::string, std::string>& getClipComponentPropNames() const  { return _clipComponentsPropNames; }
    const std::map<std::string, std::string>& getClipDepthPropNames() const      { return _clipDepthPropNames; }
    const std::map<std::string, std::string>& getClipPARPropNames() const        { return _clipPARPropNames; }
    const std::map<std::string, std::string>& getClipROIPropNames() const        { return _clipROIPropNames; }
    const std::map<std::string, std::string>& getClipFrameRangePropNames() const { return _clipFrameRangePropNames; }

    /** @brief override this to create an interact for the effect */
    virtual void setOverlayInteractDescriptor(EffectInteractWrap* desc);
};

////////////////////////////////////////////////////////////////////////////////
/** @brief Wraps up an image */
class Image
{
protected:
    /** @brief the handle that holds this image */
    PropertySet _imageProps;

    /** @brief friend so we get access to ctor */
    friend class Clip;

    void* _pixelData;                   /**< @brief the base address of the image */
    EPixelComponent _pixelComponents;     /**< @brief get the components in the image */
    int _rowDistanceBytes;                    /**< @brief the number of bytes per scanline */

    int _pixelBytes;                  /**< @brief the number of bytes per pixel */
    EBitDepth _pixelDepth;                 /**< @brief get the pixel depth */
    EPreMultiplication _preMultiplication; /**< @brief premultiplication on the image */
    OfxRectI _regionOfDefinition;          /**< @brief the RoD in pixel coordinates, this may be more or less than the bounds! */
    OfxRectI _bounds;                      /**< @brief the bounds on the pixel data */
    double _pixelAspectRatio;            /**< @brief the pixel aspect ratio */
    EField _field;                        /**< @brief which field this represents */
    std::string _uniqueID;                   /**< @brief the unique ID of this image */
    OfxPointD _renderScale;                  /**< @brief any scaling factor applied to the image */

public:
    /** @brief ctor */
    Image( OfxPropertySetHandle props );

    /** @brief dtor */
    virtual ~Image();

    const PropertySet& getPropertySet() const { return _imageProps; }

    PropertySet& getPropertySet() { return _imageProps; }

    /** @brief get the pixel depth */
    EBitDepth getPixelDepth() const { return _pixelDepth; }

    /** @brief get the components in the image */
    EPixelComponent getPixelComponents() const { return _pixelComponents; }

    /** @brief get the string representing the pixel components */
    std::string getPixelComponentsProperty() const { return _imageProps.propGetString( kOfxImageEffectPropComponents ); }

    /** @brief premultiplication on the image */
    EPreMultiplication getPreMultiplication() const { return _preMultiplication; }

    /** @brief get the scale factor that has been applied to this image */
    OfxPointD getRenderScale() const { return _renderScale; }

    /** @brief get the scale factor that has been applied to this image */
    double getPixelAspectRatio() const { return _pixelAspectRatio; }

    /** @brief get the pixel data for this image */
    void* getPixelData() const { return _pixelData; }

    /** @brief get the region of definition (in pixel coordinates) of this image */
    OfxRectI getRegionOfDefinition() const { return _regionOfDefinition; }

    /** @brief get the bounds on the image data (in pixel coordinates) of this image */
    OfxRectI getBounds() const { return _bounds; }
	
    OfxPointI getBoundsSize() const { const OfxPointI res = { _bounds.x2 - _bounds.x1, _bounds.y2 - _bounds.y1 }; return res; }

	std::size_t getPixelBytes() const;
	
    /** @brief get the distance between 2 rows in bytes, may be negative */
    int getRowDistanceBytes() const { return _rowDistanceBytes; }
	
    /** @brief get the data row size in bytes, by definition >= 0 */
    std::size_t getBoundsRowDataBytes() const;
	
	std::size_t getBoundsNbPixels() const;
	
    std::size_t getBoundsImageDataBytes() const;
	
    bool isLinearBuffer() const { return (int)(getBoundsRowDataBytes()) == getRowDistanceBytes(); }

    /** @brief get the fielding of this image */
    EField getField() const { return _field; }

    /** @brief the unique ID of this image */
    std::string getUniqueIdentifier() const { return _uniqueID; }

    /** @brief return a pixel pointer
     *
     * x and y are in pixel coordinates
     *
     * If the components are custom, then this will return NULL as the support code
     * can't know the pixel size to do the work.
     */
    void* getPixelAddress( int x, int y );
};

////////////////////////////////////////////////////////////////////////////////
/** @brief Wraps up a clip instance */
class Clip
{
protected:
    mDeclareProtectedAssignAndCC( Clip );

    /** @brief name of the clip */
    std::string _clipName;

    /** @brief properties for this clip */
    PropertySet _clipProps;

    /** @brief handle for this clip */
    OfxImageClipHandle _clipHandle;

    /** @brief effect instance that owns this clip */
    ImageEffect* _effect;

    /** @brief hidden constructor */
    Clip( ImageEffect* effect, const std::string& name, OfxImageClipHandle handle, OfxPropertySetHandle props );

    /** @brief so one can be made */
    friend class ImageEffect;

public:
    /// get the underlying property set on this clip
    const PropertySet& getPropertySet() const { return _clipProps; }

    /// get the underlying property set on this clip
    PropertySet& getPropertySet() { return _clipProps; }

    /// get the OFX clip handle
    OfxImageClipHandle getHandle() { return _clipHandle; }

    /** @brief get the name */
    const std::string& name( void ) const { return _clipName; }

    /** @brief fetch the labels */
    void getLabels( std::string& label, std::string& shortLabel, std::string& longLabel ) const;

    /** @brief what is the pixel depth images will be given to us as */
    EBitDepth getPixelDepth( void ) const;

    /** @brief what is the components images will be given to us as */
    EPixelComponent getPixelComponents( void ) const;

    /** @brief get the string representing the pixel components */
    std::string getPixelComponentsProperty( void ) const { return _clipProps.propGetString( kOfxImageEffectPropComponents ); }

    /** @brief what is the actual pixel depth of the clip */
    EBitDepth getUnmappedPixelDepth( void ) const;

    /** @brief what is the component type of the clip */
    EPixelComponent getUnmappedPixelComponents( void ) const;

    /** @brief get the string representing the pixel components */
    std::string getUnmappedPixelComponentsProperty( void ) const { return _clipProps.propGetString( kOfxImageClipPropUnmappedComponents ); }

    /** @brief get the components in the image */
    EPreMultiplication getPreMultiplication( void ) const;

    /** @brief which spatial field comes first temporally */
    EField getFieldOrder( void ) const;

    /** @brief is the clip connected */
    bool isConnected( void ) const;

    /** @brief can the clip be continuously sampled */
    bool hasContinuousSamples( void ) const;

    /** @brief get the scale factor that has been applied to this clip */
    double getPixelAspectRatio( void ) const;

    /** @brief get the frame rate, in frames per second on this clip, after any clip preferences have been applied */
    double getFrameRate( void ) const;

    /** @brief return the range of frames over which this clip has images, after any clip preferences have been applied */
    OfxRangeD getFrameRange( void ) const;

    /** @brief get the frame rate, in frames per second on this clip, before any clip preferences have been applied */
    double getUnmappedFrameRate( void ) const;

    /** @brief return the range of frames over which this clip has images, before any clip preferences have been applied */
    OfxRangeD getUnmappedFrameRange( void ) const;

    /** @brief get the RoD for this clip in the cannonical coordinate system */
    OfxRectD getCanonicalRod( const OfxTime t ) const;
    OfxRectD getCanonicalRod( const OfxTime t, const OfxPointD& renderScale ) const;
    OfxPointD getCanonicalRodSize( const OfxTime t ) const
    {
        OfxRectD r = getCanonicalRod(t);
        OfxPointD p = {r.x2-r.x1, r.y2-r.y1};
        return p;
    }
    OfxPointD getCanonicalRodSize( const OfxTime t, const OfxPointD& renderScale ) const
    {
        OfxPointD p = getCanonicalRodSize(t);
        p.x *= renderScale.x;
        p.y *= renderScale.y;
        return p;
    }

    /** @brief get the RoD for this clip in pixel space */
    OfxRectI getPixelRod( const OfxTime t ) const;
    OfxRectI getPixelRod( const OfxTime t, const OfxPointD& renderScale ) const;
    OfxPointI getPixelRodSize( const OfxTime t ) const
    {
        OfxRectI r = getPixelRod(t);
        OfxPointI p = {r.x2-r.x1, r.y2-r.y1};
        return p;
    }
    OfxPointI getPixelRodSize( const OfxTime t, const OfxPointD& renderScale ) const
    {
        OfxPointI p = getPixelRodSize(t);
        p.x = static_cast<int>( p.x * renderScale.x );
        p.y = static_cast<int>( p.y * renderScale.y );
        return p;
    }

    /** @brief fetch an image
     *
     * When finished with, the client code must delete the image.
     *
     * If the same image is fetched twice, it must be deleted in each case, they will not be the same pointer.
     */
    Image* fetchImage( OfxTime t );

    /** @brief fetch an image, with a specific region in cannonical coordinates
     *
     * When finished with, the client code must delete the image.
     *
     * If the same image is fetched twice, it must be deleted in each case, they will not be the same pointer.
     */
    Image* fetchImage( OfxTime t, OfxRectD bounds );

    /** @brief fetch an image, with a specific region in cannonical coordinates
     *
     * When finished with, the client code must delete the image.
     *
     * If the same image is fetched twice, it must be deleted in each case, they will not be the same pointer.
     */
    Image* fetchImage( OfxTime t, OfxRectD* bounds )
    {
        if( bounds )
            return fetchImage( t, *bounds );
        else
            return fetchImage( t );
    }

};

////////////////////////////////////////////////////////////////////////////////
/** @brief Class that skins image memory allocation */
class ImageMemory
{
protected:
    OfxImageMemoryHandle _handle;
	bool _alloc;
	
public:
    ImageMemory();
    ImageMemory( size_t nBytes, ImageEffect* associatedEffect = 0 );
    ~ImageMemory();

	void alloc( size_t nBytes, ImageEffect* associatedEffect );
	
    /** @brief lock the memory and return a pointer to it */
    void* lock( void );

    /** @brief unlock the memory */
    void unlock( void );
};

////////////////////////////////////////////////////////////////////////////////
/** @brief POD struct to pass rendering arguments into @ref ImageEffect::render and @ref OFX::ImageEffect::isIdentity */
struct RenderArguments
{
    double time;
    OfxPointD renderScale;
    OfxRectI renderWindow;
    EField fieldToRender;
};

/** @brief POD struct to pass arguments into  @ref OFX::ImageEffect::render */
struct BeginSequenceRenderArguments
{
    OfxRangeD frameRange;
    double frameStep;
    bool isInteractive;
    OfxPointD renderScale;
};

/** @brief POD struct to pass arguments into  @ref OFX::ImageEffect::beginSequenceRender */
struct EndSequenceRenderArguments
{
    bool isInteractive;
    OfxPointD renderScale;
};

/** @brief POD struct to pass arguments into  @ref OFX::ImageEffect::getRegionOfDefinition */
struct RegionOfDefinitionArguments
{
    double time;
    OfxPointD renderScale;
};

/** @brief POD struct to pass arguments into @ref OFX::ImageEffect::getRegionsOfInterest */
struct RegionsOfInterestArguments
{
    double time;
    OfxPointD renderScale;
    OfxRectD regionOfInterest;
};

/** @brief Class used to set regions of interest on a clip in @ref OFX::ImageEffect::getRegionsOfInterest
 *
 * This is a base class, the actual class is private and you don't need to see the glue involved.
 */
class RegionOfInterestSetter
{
public:
    /** @brief function to set the RoI of a clip, pass in the clip to set the RoI of, and the RoI itself */
    virtual void setRegionOfInterest( const Clip& clip, const OfxRectD& RoI ) = 0;
    virtual ~RegionOfInterestSetter() = 0;
};

/** @brief POD struct to pass arguments into @ref OFX::ImageEffect::getFramesNeeded */
struct FramesNeededArguments
{
    double time;
};

/** @brief Class used to set the frames needed to render a single frame of a clip in @ref OFX::ImageEffect::getFramesNeeded
 *
 * This is a base class, the actual class is private and you don't need to see the glue involved.
 */
class FramesNeededSetter
{
public:
    /** @brief function to set the frames needed on a clip, the range is min <= time <= max */
    virtual void setFramesNeeded( const Clip& clip, const OfxRangeD& range ) = 0;
    virtual ~FramesNeededSetter() = 0;
};

/** @brief Class used to set the clip preferences of the effect.
 */
class ClipPreferencesSetter
{
OFX::PropertySet outArgs_;
bool doneSomething_;
typedef std::map<std::string, std::string> StringStringMap;
const StringStringMap& clipDepthPropNames_;
const StringStringMap& clipComponentPropNames_;
const StringStringMap& clipPARPropNames_;
const std::string& extractValueForName( const StringStringMap& m, const std::string& name );

public:
    ImageEffectHostDescription* _imageEffectHostDescription;

public:
    ClipPreferencesSetter( OFX::PropertySet       props,
                           const StringStringMap& depthPropNames,
                           const StringStringMap& componentPropNames,
                           const StringStringMap& PARPropNames )
        : outArgs_( props ),
        doneSomething_( false ),
        clipDepthPropNames_( depthPropNames ),
        clipComponentPropNames_( componentPropNames ),
        clipPARPropNames_( PARPropNames )
    {
            _imageEffectHostDescription = getImageEffectHostDescription();
    }

    bool didSomething( void ) const { return doneSomething_; }

    /** @brief, force the host to set a clip's mapped component type to be \em comps.
     *
     * Only callable on non optional clips in all contexts. Must set comps to be one of the types the effect says it supports on the given clip.
     *
     * See the OFX API documentation for the default values of this.
     */
    void setClipComponents( Clip& clip, EPixelComponent comps );

    /** @brief, force the host to set a clip's mapped bit depth be \em bitDepth
     *
     * Only callable if the OFX::ImageEffectHostDescription::supportsMultipleClipDepths is true.
     *
     * See the OFX API documentation for the default values of this.
     */
    void setClipBitDepth( Clip& clip, EBitDepth bitDepth );

    /** @brief, force the host to set a clip's mapped Pixel Aspect Ratio to be \em PAR
     *
     * Only callable if the OFX::ImageEffectHostDescription::supportsMultipleClipPARs is true.
     *
     * Default is up to the host, generally based on the input clips.
     *
     * Not supported by most host applications.
     */
    void setPixelAspectRatio( Clip& clip, double PAR );

    /** @brief Allows an effect to change the output frame rate
     *
     * Only callable if OFX::ImageEffectHostDescription::supportsSetableFrameRate is true.
     *
     * Default is controlled by the host, typically the framerate of the input clips.
     */
    void setOutputFrameRate( double v );

    /** @brief Set the premultiplication state of the output clip.
     *
     * Defaults to the premultiplication state of ???
     */
    void setOutputPremultiplication( EPreMultiplication v );

    /** @brief Set whether the effect can be continously sampled.
     *
     * Defaults to false.
     */
    void setOutputHasContinousSamples( bool v );

    /** @brief Sets whether the effect will produce different images in all frames, even if the no params or input images are varying (eg: a noise generator).
     *
     * Defaults to false.
     */
    void setOutputFrameVarying( bool v );

    /** @brief Sets the output fielding
     *
     * Default is host dependent, must be one of
     * - eFieldNone,
     * - eFieldLower,
     * - eFieldUpper
     */
    void setOutputFielding( EField v );
};

static const OfxPointD kNoRenderScale = { 1.0, 1.0 };

/** @brief POD data structure passing in the instance changed args */
struct InstanceChangedArgs
{
    InstanceChangedArgs( const OfxTime time = 0.0, const OfxPointD renderScale = kNoRenderScale, const InstanceChangeReason reason = OFX::eChangePluginEdit )
    : time(time)
    , renderScale( renderScale )
    , reason( reason )
    {}

    OfxTime time; //< time of the change
    OfxPointD renderScale; ///< the renderscale on the instance
    InstanceChangeReason reason; ///< why did it change
};


////////////////////////////////////////////////////////////////////////////////
/** @brief Wraps up an effect instance, plugin implementations need to inherit from this */
class ImageEffect : public ParamSet
{
protected:
    mDeclareProtectedAssignAndCC( ImageEffect );

private:
    /** @brief to get access to the effect handle without exposing it generally via a function */
    friend class ImageMemory;

    /** @brief The effect handle */
    OfxImageEffectHandle _effectHandle;

    /** @brief properties for this node */
    PropertySet _effectProps;

    /** @brief the context of the effect */
    EContext _context;

    /** @brief Set of all previously defined parameters, defined on demand */
    std::map<std::string, Clip*> _fetchedClips;

    /** @brief the overlay interacts that are open on this image effect */
    std::list<OverlayInteract*> _overlayInteracts;

    /** @brief cached result of whether progress start succeeded. */
    bool _progressStartSuccess;

public:
    /** @brief ctor */
    ImageEffect( OfxImageEffectHandle handle );

    /** @brief dtor */
    virtual ~ImageEffect();

    const PropertySet& getPropertySet() const { return _effectProps; }

    PropertySet& getPropertySet() { return _effectProps; }

    OfxImageEffectHandle getHandle( void ) const { return _effectHandle; }

	std::string getName() const;

    /** @brief the context this effect was instantiate in */
    EContext getContext( void ) const;

    /** @brief size of the project */
    OfxPointD getProjectSize( void ) const;

    /** @brief origin of the project */
    OfxPointD getProjectOffset( void ) const;

    /** @brief extent of the project */
    OfxPointD getProjectExtent( void ) const;

    /** @brief pixel aspect ratio of the project */
    double getProjectPixelAspectRatio( void ) const;

    /** @brief how long does the effect last */
    double getEffectDuration( void ) const;

    /** @brief the frame rate of the project */
    double getFrameRate( void ) const;

    /** @brief is the instance currently being interacted with */
    bool isInteractive( void ) const;

    /** @brief set the instance to be sequentially renderred, this should have been part of clip preferences! */
    void setSequentialRender( bool v );

    /** @brief Have we informed the host we want to be seqentially renderred ? */
    bool getSequentialRender( void ) const;

    OFX::Message::EMessageReply sendMessage( OFX::Message::EMessageType type, const std::string& id, const std::string& msg );

    /** @brief Fetch the named clip from this instance
     *
     * The returned clip \em must not be deleted by the client code. This is all managed by the ImageEffect itself.
     */
    Clip* fetchClip( const std::string& name );

    CameraParam* fetchCameraParam( const std::string& name );

    /** @brief does the host want us to abort rendering? */
    bool abort() const;

    /** @brief adds a new interact to the set of interacts open on this effect */
    void addOverlayInteract( OverlayInteract* interact );

    /** @brief removes an interact to the set of interacts open on this effect */
    void removeOverlayInteract( OverlayInteract* interact );

    /** @brief force all overlays on this interact to be redrawn */
    void redrawOverlays( void );

    ////////////////////////////////////////////////////////////////////////////////
    // these are actions that need to be overridden by a plugin that implements an effect host

    /** @brief The purge caches action, a request for an instance to free up as much memory as possible in low memory situations */
    virtual void purgeCaches( void );

    /** @brief The sync private data action, called when the effect needs to sync any private data to persistant parameters */
    virtual void syncPrivateData( void );

    /** @brief client render function, this is one of the few that must be overridden */
    virtual void render( const RenderArguments& args ) = 0;

    /** @brief client begin sequence render function */
    virtual void beginSequenceRender( const BeginSequenceRenderArguments& args );

    /** @brief client end sequence render function */
    virtual void endSequenceRender( const EndSequenceRenderArguments& args );

    /** @brief client is identity function, returns the clip and time for the identity function
     *
     * If the effect would do no processing for the given param set and render arguments, then this
     * function should return true and set the \em identityClip pointer to point to the clip that is the identity
     * and \em identityTime to be the time at which to access the clip for the identity operation.
     */
    virtual bool isIdentity( const RenderArguments& args, Clip*& identityClip, double& identityTime );

    /** @brief The get RoD action.
     *
     * If the effect wants change the rod from the default value (which is the union of RoD's of all input clips)
     * it should set the \em rod argument and return true.
     *
     * This is all in cannonical coordinates.
     */
    virtual bool getRegionOfDefinition( const RegionOfDefinitionArguments& args, OfxRectD& rod );

    /** @brief the get region of interest action
     *
     * If the effect wants change its region of interest on any input clip from the default values (which is the same as the RoI in the arguments)
     * it should do so by calling the OFX::RegionOfInterestSetter::setRegionOfInterest function on the \em rois argument.
     *
     * Note, everything is in \em cannonical \em coordinates.
     */
    virtual void getRegionsOfInterest( const RegionsOfInterestArguments& args, RegionOfInterestSetter& rois );

    /** @brief the get frames needed action
     *
     * If the effect wants change the frames needed on an input clip from the default values (which is the same as the frame to be renderred)
     * it should do so by calling the OFX::FramesNeededSetter::setFramesNeeded function on the \em frames argument.
     */
    virtual void getFramesNeeded( const FramesNeededArguments& args, FramesNeededSetter& frames );

    /** @brief get the clip preferences */
    virtual void getClipPreferences( ClipPreferencesSetter& clipPreferences );

    /** @brief the effect is about to be actively edited by a user, called when the first user interface is opened on an instance */
    virtual void beginEdit( void );

    /** @brief the effect is no longer being edited by a user, called when the last user interface is closed on an instance */
    virtual void endEdit( void );

    /** @brief the effect is about to have some values changed */
    virtual void beginChanged( InstanceChangeReason reason );

    /** @brief called when a param has just had its value changed */
    virtual void changedParam( const InstanceChangedArgs& args, const std::string& paramName );

    /** @brief called when a clip has just been changed in some way (a rewire maybe) */
    virtual void changedClip( const InstanceChangedArgs& args, const std::string& clipName );

    /** @brief the effect has just had some values changed */
    virtual void endChanged( InstanceChangeReason reason );

    /** @brief what is the time domain of this effect, valid only in the general context
     *
     * return true if range was set, otherwise the default (the union of the time domain of all input clips) is used
     */
    virtual bool getTimeDomain( OfxRangeD& range );

    /// Start doing progress.
    void progressStart( const std::string& message );

    /// finish yer progress
    void progressEnd();

    /// set the progress to some level of completion,
    /// returns true if you should abandon processing, false to continue
    bool progressUpdate( const double t );

    /// get the current time on the timeline. This is not necessarily the same
    /// time as being passed to an action (eg render)
    double timeLineGetTime();

    /// set the timeline to a specific time
    void timeLineGotoTime( const double t );

    /// get the first and last times available on the effect's timeline
    void timeLineGetBounds( double& t1, double& t2 );
    inline OfxRangeD timeLineGetBounds() { OfxRangeD range; timeLineGetBounds( range.min, range.max ); return range; }
};

////////////////////////////////////////////////////////////////////////////////
/** @brief The OFX::Plugin namespace. All the functions in here needs to be defined by each plugin that uses the support libs.
 */
namespace Plugin {
/** @brief Plugin side function used to identify the plugin to the support library */
void getPluginID( OFX::PluginFactoryArray& id );

/// If the client has defined its own exception type, allow it to catch it in the main function
#ifdef OFX_CLIENT_EXCEPTION_TYPE
OfxStatus catchException( OFX_CLIENT_EXCEPTION_TYPE& ex );
#endif
};

};

// undeclare the protected assign and CC macro
#undef mDeclareProtectedAssignAndCC

#endif
