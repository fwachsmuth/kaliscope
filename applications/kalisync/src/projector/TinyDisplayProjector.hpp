#ifndef _KALI_TINYDISPLAYPROJECTOR_HPP_
#define	_KALI_TINYDISPLAYPROJECTOR_HPP_

#include "IProjector.hpp"

#include <stdlib.h>
#include <linux/fb.h>
#include <sys/mman.h>

namespace kaliscope
{

class TinyDisplayProjector : public IProjector
{
public:
    TinyDisplayProjector();
    virtual ~TinyDisplayProjector();
    
    /**
     * @brief initialize projector
     */
    virtual void init();

    /**
     * @brief switch projector on
     */
    void switchOn();

    /**
     * @brief switch projector off
     */
    void switchOff();

private:
    void drawPixel( const std::size_t x, const std::size_t y, int r, int g, int b );

    void drawSquare( const std::size_t x, const std::size_t y, const std::size_t height, const std::size_t width, int c );

    void closeFramebuffer();

    char *_fbp = NULL;
    int _fb = 0;
    int _width = 0;
    int _height = 0;
    long int _screensize = 0;
    fb_fix_screeninfo _fix;
    fb_var_screeninfo _origVar;
    fb_var_screeninfo _var;
};

}

#endif
