/** @file latticedrawer.hpp */
//
// Copyright 2015 Arvind Singh
//
// This file is part of the mtools library.
//
// mtools is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with mtools  If not, see <http://www.gnu.org/licenses/>.


#pragma once


#include "drawable2Dobject.hpp"
#include "customcimg.hpp"
#include "rgbc.hpp"
#include "maths/rect.hpp"
#include "misc/misc.hpp"
#include "misc/metaprog.hpp"

#include <algorithm>
#include <ctime>
#include <mutex>
#include <atomic>


namespace mtools
{

    using cimg_library::CImg;


    /**
     * Encapsulate a `getColor()` function into a lattice object that can be used with the LatticeDrawer
     * class. This wrapper class contain no data and just a static method, therefore, there is no need 
     * to create an instance one can just pass nullptr to the LatticeDrawer as the associated object.
     * 
     * @par Example
     * @code{.cpp}
     * RGBc myColorFun(iVec2 pos)  {...}
     * 
     * LatticeDrawer<LatticeObj<myColorFct> >(nullptr); // create a Lattice drawer associated with myColorFun function
     * @endcode
     * 
     * @tparam  getColorFun The getColor method that will be called when queriyng the color of a
     *                      site. It signature must be exactly `mtools::RGBc getColor(mtools::iVec2 pos)`.
     **/
    template<mtools::RGBc (*getColorFun)(mtools::iVec2 pos)> class LatticeObj
        {
        public:
       inline static RGBc getColor(mtools::iVec2 pos) { return getColorFun(pos); }
       inline static LatticeObj<getColorFun> * get() { return(nullptr); }
        };



    /**
     * Encapsulate both a `getColor()` and a `getImage()` function into a lattice object that can be
     * used with the LatticeDrawer class. This wrapper class contain no data and just a static method, 
     * therefore, there is no need to create an instance one can just pass nullptr to the LatticeDrawer 
     * as the associated object.
     *
     * @par Example
     * @code{.cpp}
     * RGBc myColorFun(iVec2 pos)  {...}
     * const CImg<unsigned char> myImageFun(iVec2 pos,iVec2 size)  {...}
     *
     * LatticeDrawer<LatticeObj<myColorFct, myImageFun> >(nullptr); // create a Lattice drawer associated with myColorFun and myImageFun functions
     * @endcode
     *
     * @tparam  getColorFun The getColor method that will be called when queriyng the color of a
     *                      site. It signature must be exactly `mtools::RGBc getColor(mtools::iVec2
     *                      pos)`.
     * @tparam  getImageFun The getImage method that will be called when querying the image of a
     *                      site. It signature must be exactly `const CImg<unsigned char> *
     *                      getColor(mtools::iVec2 pos, mtools::iVec2 size)`. The pointer to the
     *                      image returned by this function must either be null or point to a 3 or 4
     *                      channel image. The image is not modified and can be disposed of
     *                      afterward. If the method returns nullptr, then the site is completely
     *                      transparent. The parameter size is an indication of the preferred size be
     *                      images of any size can be returned (although the quality will be less and
     *                      more time expensive).
     * @param   pos     The position.
     * @param   size    The size.
     **/
    template<mtools::RGBc(*getColorFun)(mtools::iVec2 pos), const cimg_library::CImg<unsigned char>* (*getImageFun)(mtools::iVec2 pos, mtools::iVec2 size) > class LatticeObjImage
        {
        public:

        inline static RGBc getColor(mtools::iVec2 pos) { return getColorFun(pos); }
        inline static const cimg_library::CImg<unsigned char> * getImage(mtools::iVec2 pos, mtools::iVec2 size) { return getImageFun(pos,size); }
        inline static LatticeObjImage<getColorFun, getImageFun> * get() { return(nullptr); }

        };


/**
 * Draws part of a lattice object into into a CImg image. This class implement the
 * Drawable2DObject interface.
 * 
 * - The parameters of the drawing are set using the `setImageType` , `setParam`, `ResetDrawing`
 * method. THe method `work` is used to create the drawing itself. The actual warping of the
 * image into a given CImg image is performed using the `drawOnto` method which is quite fast.
 * 
 * - All the public methods of this class are thread-safe : they can be called simultaneously
 * from any thread and the call are lined up. In particular, the `work` method can be time
 * expensive and might be better called from a worker thread : see the `AutoDrawable2DObject`
 * class for a generic implementation.
 * 
 * - The template LatticeObj must implement a method `RGBc getColor(iVec2 pos)` which return the
 * color associated with a given site. The method should be made as fast as possible. The fourth
 * channel of the returned color will be used when drawing on 4 channel images and ignored when
 * drawing on 3 channel images.
 * 
 * - If TYPEIMAGE is selected, the plotter can request an image of the sites by calling the
 * object method `const CImg<unsigned char> * getImage(iVec pos,iVec size)` if it is present.
 * The presence of this method is automatically detected by the drawer. If it cannot find it, if
 * falls back using the `getColor` method and simply draw a square of the given color.
 * 
 * - The size parameter passed to getImage is an indication of the prefered size of the sprite
 * but is not an obligation and an image of any size can be returned. However, returning an
 * image of the requested size guarantee the fastest drawing and the best quality of the image.
 * Furthermore, the returned image is not modified and can be a shared image. It can be re-used
 * for subsequent calls. Since all the images of site of a given drawing have the same size, it
 * is usually interesting to redraw on a single image which is resized when needed (this will
 * happen only when the range/size of the drawing changes).
 * 
 * - The `getImage` method must return a pointer to a CImg image. It can be nullptr: in this
 * case, the drawer interpret this as the site being completely transparent. If the returned
 * pointer is not null, then it must point to a 3 or 4 channel image. If the image only has 3
 * channel, a fourth, completely opaque channel is added by the plotter when drawing the lattice
 * on a 4 channel images.
 *
 * @par Example
 * @code{.cpp}
using namespace mtools;

RGBc colorCircle(iVec2 pos)
    { // color red inside a circle of radius 100 around the origin, (transparent) white outside
    if (pos.norm() < 100) { return RGBc::c_Red; }
    return RGBc::c_TransparentWhite;
    }

const CImg<unsigned char> * imageCircle(iVec2 pos, iVec2 size)
    { // no image outside of the circle. Small red 'site circle' inside with transprent background
    static CImg<unsigned char>  im;
    im.resize((int)size.X(), (int)size.Y(), 1, 4, -1); // resize if needed, we just use 4 channel so we can use transparency
    EdgeSiteImage ES;
    if (pos.norm() < 100) { ES.site(true).makeImage(im, size); }
    else { return nullptr; } // red site of nothing
    return(&im);
    }

int main()
    {
    fRect r(-200, 200, -200, 200);						        // the range displayed
    CImg<unsigned char> image(1000, 800, 1, 4, false);	// the image, use only 4 channels for trnasparency
    LatticeDrawer<LatticeObjImage<colorCircle, imageCircle> > LD(nullptr); // create the drawer
    LD.setParam(r, iVec2(1000, 800));                           // set the parameters
    int drawtype = 0, isaxe = 1, isgrid = 0, iscell = 1; // flags
    cimg_library::CImgDisplay DD(image); // display
    while ((!DD.is_closed())) {
        uint32 k = DD.key();
        if ((DD.is_key(cimg_library::cimg::keyA))) { isaxe = 1 - isaxe; std::this_thread::sleep_for(std::chrono::milliseconds(50)); }   // type A for toggle axe (with graduations)
        if ((DD.is_key(cimg_library::cimg::keyG))) { isgrid = 1 - isgrid; std::this_thread::sleep_for(std::chrono::milliseconds(50)); } // type G for toggle grid
        if ((DD.is_key(cimg_library::cimg::keyC))) { iscell = 1 - iscell; std::this_thread::sleep_for(std::chrono::milliseconds(50)); } // type C for toggle cell
        if (DD.is_key(cimg_library::cimg::keyESC)) { LD.resetDrawing(); } // [ESC] to force complete redraw
        if (DD.is_key(cimg_library::cimg::keyENTER)) { drawtype = 1 - drawtype; LD.setImageType(drawtype); std::this_thread::sleep_for(std::chrono::milliseconds(50)); } // enter to change the type of drawing
        if (DD.is_key(cimg_library::cimg::keyARROWUP)) { double sh = r.ly() / 20; r.ymin += sh; r.ymax += sh; LD.setParam(r, iVec2(1000, 800)); } // move n the four directions
        if (DD.is_key(cimg_library::cimg::keyARROWDOWN)) { double sh = r.ly() / 20; r.ymin -= sh; r.ymax -= sh; LD.setParam(r, iVec2(1000, 800)); } //
        if (DD.is_key(cimg_library::cimg::keyARROWLEFT)) { double sh = r.lx() / 20; r.xmin -= sh; r.xmax -= sh; LD.setParam(r, iVec2(1000, 800)); } //
        if (DD.is_key(cimg_library::cimg::keyARROWRIGHT)) { double sh = r.lx() / 20; r.xmin += sh; r.xmax += sh; LD.setParam(r, iVec2(1000, 800)); } //
        if (DD.is_key(cimg_library::cimg::keyPAGEDOWN)) { double lx = r.xmax - r.xmin; double ly = r.ymax - r.ymin; r.xmin = r.xmin - (lx / 8.0); r.xmax = r.xmax + (lx / 8.0); r.ymin = r.ymin - (ly / 8.0);  r.ymax = r.ymax + (ly / 8.0); LD.setParam(r, iVec2(1000, 800)); }
        if (DD.is_key(cimg_library::cimg::keyPAGEUP)) { if ((r.lx()>0.5) && (r.ly()>0.5)) { double lx = r.xmax - r.xmin; double ly = r.ymax - r.ymin; r.xmin = r.xmin + (lx / 10.0); r.xmax = r.xmax - (lx / 10.0); r.ymin = r.ymin + (ly / 10.0); r.ymax = r.ymax - (ly / 10.0); } LD.setParam(r, iVec2(1000, 800)); }
        std::cout << "quality = " << LD.work(50) << "\n"; // work a little bit
        image.checkerboard(); // draw a gray checker board so we can see the "transparency !"
        LD.drawOnto(image, 1.0); // draw onto the image with full opacity
        if (isaxe) { image.fRect_drawAxes(r).fRect_drawGraduations(r).fRect_drawNumbers(r); }
        if (isgrid) { image.fRect_drawGrid(r); }
        if (iscell) { image.fRect_drawCells(r); }
        DD.display(image);
        }
    return 0;
    }
 *@endcode
 *        
 * @tparam  LatticeObj  Type of the lattice object. Can be any class provided that it defines the 
 *                      method `RGBc getColor(iVec2 pos)` method and possibly (but not necessary) a 
 *                      method `const CImg<unsigned char> * getImage(iVec2 pos,iVec2 size)`.
 **/
template<class LatticeObj> class LatticeDrawer : public mtools::internals_graphics::Drawable2DObject
{
  
public:

	static const int TYPEPIXEL		= 0; ///< draw each site with a square of a given color (0)
	static const int TYPEIMAGE		= 1; ///< draw (if possible) each site using an image for the site (1)


    /**
     * Constructor. Set the lattice object that will be drawn. 
     *
     * @param [in,out]  obj The object to draw, it must survive the drawer.
     **/
    LatticeDrawer(LatticeObj * obj) : _g_requestAbort(0), _g_current_quality(0), _g_obj(obj), _g_drawingtype(TYPEPIXEL), _g_reqdrawtype(TYPEPIXEL), _g_imSize(201, 201), _g_r(-100.5, 100.5, -100.5, 100.5), _g_redraw_im(true), _g_redraw_pix(true)
		{
        static_assert(mtools::metaprog::has_getColor<LatticeObj, mtools::RGBc, mtools::iVec2>::value, "The object T must be implement a 'RGBc getColor(iVec2 pos)' method.");
        _initInt16Buf();
		_initRand();
		}


    /**
     * Destructor.
     **/
	~LatticeDrawer()
		{
        _g_requestAbort++; // request immediate stop of the work method if active.
            {
            std::lock_guard<std::timed_mutex> lg(_g_lock); // and wait until we aquire the lock 
            _removeInt16Buf(); // remove the int16 buffer
            }
        }


    /**
     * change the type of drawing. Calling this method interrupt any work() in progress. Even if
     * TYPEIMAGE is requested, the drawer may decide to paint a TYPEPIXEL drawing. For instance if
     * the object does not implement getImage() or if the zoom is to far away. This method is fast,
     * it does not draw anything.
     *
     * @param   imageType   Type of the drawing, either TYPEIMAGE or TYPEPIXEL.
     *
     * @return  The drawing type that will be used anyway.
     **/
    int setImageType(int imageType = TYPEPIXEL)
        {
        _g_requestAbort++; // request immediate stop of the work method if active.
            {
            std::lock_guard<std::timed_mutex> lg(_g_lock); // and wait until we aquire the lock 
            _g_requestAbort--; // and then remove the stop request
            _g_reqdrawtype = imageType; // save the requested drawing type
            _setDrawingMode(imageType); // set the new drawing type but silently change it if needed
            if (_g_drawingtype == TYPEPIXEL) { _workPixel(0); } else { _workImage(0); } // work for a zero length period to update the quality and sync things.
            return _g_drawingtype;
            }
        }


    /**
     * Return the type of drawing performed. This is the type of drawing that is requested but the
     * drawing ctually done may differ. If there is not getImage method, this method will return
     * true regardeless of the last drawing type requested via setImageType.
     *
     * @return  The type of drawing requested, either TYPEIMAGE or TYPEPIXEL.
     **/
    int imageType() const
        {
        return(hasImage() ? (int)_g_reqdrawtype : TYPEPIXEL);
        }


    /**
     * Query if the object can draw using images for sites (i.e. if a getImage() method was found).
     *
     * @return  true if we can use images to represent integer sites and false otherwise.
     **/
    bool hasImage() const
        {
        return metaprog::has_getImage<LatticeObj, const cimg_library::CImg<unsigned char>*, mtools::iVec2, mtools::iVec2>::value;
        }


    /**
    * Set the parameters of the drawing. Calling this method interrupt any work() in progress. 
    * This method is fast, it does not draw anything.
    **/
    virtual void setParam(mtools::fRect range, mtools::iVec2 imageSize) override
        {
        MTOOLS_ASSERT(!range.isEmpty());
        MTOOLS_ASSERT((imageSize.X() >0) && (imageSize.Y()>0));     // make sure the image not empty.
        ++_g_requestAbort; // request immediate stop of the work method if active.
            {
            std::lock_guard<std::timed_mutex> lg(_g_lock); // and wait until we aquire the lock 
            --_g_requestAbort; // and then remove the stop request
            _g_imSize = imageSize;
            _g_r = range;
            _setDrawingMode(_g_reqdrawtype); // change the drawing type if needed
            if (_g_drawingtype == TYPEPIXEL) { _workPixel(0); } else { _workImage(0); } // work for a zero length period to update the quality and sync things.
            }
        }


    /**
     * Force a reset of the drawing. Calling this method interrupt any work() is progress. This
     * method is fast, it does not draw anything.
     **/
    virtual void resetDrawing() override
        {
        ++_g_requestAbort; // request immediate stop of the work method if active.
            {
            std::lock_guard<std::timed_mutex> lg(_g_lock); // and wait until we aquire the lock 
            --_g_requestAbort; // and then remove the stop request
            _g_redraw_im = true;
            _g_redraw_pix = true;
            if (_g_drawingtype == TYPEPIXEL) { _workPixel(0); } else { _workImage(0); } // work for a zero length period to update the quality and sync things.
            }
        }


    /**
     * Draw onto a given image. This method is called by the composer when it want the picture. This
     * method is fast and does not "compute" anything. It simply warp the current drawing onto a
     * given cimg image.
     * 
     * The provided cimg image may must have 3 or 4 channel and the same size as that set via
     * setParam().
     * 
     * - If im has 3 channels, then the drawer uses only 3 channels for the lattice and simply
     * superpose the image created over im (multiplying it by an optional opacity parameter).
     * 
     * - If im has 4 channels, the drawer uses also 4th channel for the lattice.
     *     - For pixel type drawing : this is simply the 4th channels of the RGBc return by the
     *     getColor() method.
     *     - For Image drawing. This is the 4th channel of the image returned by the getImage()
     *     method. If the returned sprite has only 3 channels, a 4th completely opaque channel (255)
     *     is added.
     * 
     * The method is faster when transparency is not used (i.e. when the supplied image im has 3
     * channel) and fastest when opacity is 1.0 (or 0.0, but this does nothing).
     *
     * @param [in,out]  im  The image to draw onto (must be a 3 or 4 channel image and it size must
     *                      be equal to the size previously set via the setParam() method.
     * @param   opacity     The opacity that should be applied to the picture prior to drawing onto
     *                      im. If set to 0.0, then the method returns without drawing anything. The
     *                      opacity is multiplied with other opacities when dealing with 4 channel
     *                      images.
     *
     * @return  The quality of the drawing performed (0 = nothing drawn, 100 = perfect drawing).
     **/
    virtual int drawOnto(cimg_library::CImg<unsigned char> & im, float opacity = 1.0) override
        {
        MTOOLS_ASSERT((im.width() == _g_imSize.X()) && (im.height() == _g_imSize.Y()));
        MTOOLS_ASSERT((im.spectrum() == 3) || (im.spectrum() == 4));
        ++_g_requestAbort; // request immediate stop of the work method if active.
            {
            std::lock_guard<std::timed_mutex> lg(_g_lock); // and wait until we aquire the lock 
            --_g_requestAbort; // and then remove the stop request
            if (opacity > 0.0)
                {
                if (_g_drawingtype == TYPEPIXEL) { _drawOntoPixel(im, opacity); } else { _drawOntoImage(im, opacity); } //  warp the image 
                }
            return _g_current_quality; // return the quality of the drawing
            }
        }


    /**
     * Return the quality of the current drawing. Very fast and does not interrupt any other
     * method/work that might be in progress.
     *
     * @return  The current quality between 0 (nothing to show) and 100 (perfect drawing).
     **/
    virtual int quality() const override {return _g_current_quality;}


    /**
     * Works on the drawing for a maximum specified period of time. If the drawing is already
     * completed, returns directly.
     * 
     * The function has the lowest priority of all the public methods and may be interrupted (hence
     * returning early) if another method such as drawOnto(),setParam()... is accessed by another
     * thread simultaneously.
     * 
     * If another thread already launched some work, this method will wait until the lock is
     * released (but will return if the time allowed is exceeded).
     * 
     * A typical time span of 50/100ms is usually enough to get some drawing to show. If maxtime_ms
     * = 0, then the function return immediatly (with the current quality of the drawing).
     *
     * @param   maxtime_ms  The maximum time in millisecond allowed for drawing.
     *
     * @return  The quality of the current drawing:  0 = nothing to show, 100 = perfect drawing  that
     *          cannot be improved.
     **/
    virtual int work(int maxtime_ms) override
        {
        MTOOLS_ASSERT(maxtime_ms >= 0);
        if (((int)_g_requestAbort > 0) || (maxtime_ms <= 0)) { return _g_current_quality; } // do not even try to work if the flag is set
        if (!_g_lock.try_lock_for(std::chrono::milliseconds((maxtime_ms/2)+1))) { return _g_current_quality; } // could not lock the mutex, we return without doing anything
        if ((int)_g_requestAbort > 0) { _g_lock.unlock(); return _g_current_quality; } // do not even try to work if the flag is set
        if (_g_drawingtype == TYPEPIXEL) { _workPixel(maxtime_ms); } else { _workImage(maxtime_ms); } // work...
        _g_lock.unlock(); // release mutex
        return _g_current_quality; // ..and return quality
        }


    /**
     * This object need work to construct a drawing. Returns true without interrupting anything.
     *
     * @return  true.
     **/
    virtual bool needWork() const override { return true; }



    /**
     * Stop any ongoing work and then return
     **/
    virtual void stopWork() override
        {
        ++_g_requestAbort; // request immediate stop of the work method if active.
            {
            std::lock_guard<std::timed_mutex> lg(_g_lock); // and wait until we aquire the lock 
            --_g_requestAbort; // and then remove the stop request
            }
        }



private:



	/**************************************************************************************************************************************************
	*                                                                    PRIVATE PART 
	************************************************************************************************************************************************/

    std::timed_mutex  _g_lock;             // mutex for locking
    std::atomic<int>  _g_requestAbort;     // flag used for requesting the work() method to abort.
    mutable std::atomic<int> _g_current_quality;  // the current quality of the drawing
    LatticeObj *      _g_obj;               // the object to draw
    std::atomic<int>  _g_drawingtype;       // type of the current drawing
    std::atomic<int>  _g_reqdrawtype;       // last requested drawing type
    iVec2             _g_imSize;            // size of the drawing
    fRect             _g_r;                 // current range.
    std::atomic<bool> _g_redraw_im;         // true if we should redraw from scratch the pixel image
    std::atomic<bool> _g_redraw_pix;         // true if we should redraw from scratch the sprite image



    /* set the new drawing mode to imageType (but silently change it if needed) */
    void _setDrawingMode(int imageType)
        {
        if (imageType != TYPEIMAGE) { _g_drawingtype = TYPEPIXEL; return; }
        if (!hasImage()) { _g_drawingtype = TYPEPIXEL; return; }
        if (((_g_imSize.X() / _g_r.lx()) < 6) || ((_g_imSize.Y() / _g_r.ly()) < 6))  { _g_drawingtype = TYPEPIXEL; return; }
        if ((_g_r.lx() < 0.25) || (_g_r.ly() < 0.25)) { _g_drawingtype = TYPEPIXEL; return; }
        _g_drawingtype = TYPEIMAGE; 
        return;
        }
    

// ****************************************************************
// THE PIXEL DRAWER
// ****************************************************************

fRect           _pr;                    // the current range
uint32 			_counter1,_counter2;	// counter for the number of pixel added in each cell: counter1 for cells < (_qi,_qj) and counter2 for cells >= (_qi,qj)
uint32 			_qi,_qj;		        // position where we stopped previously
int 			_phase;			        // the current phase of the drawing



/* update the quality of the picture  */
void _qualityPixelDraw() const
    {
    switch (_phase)
        {
        case 0: {_g_current_quality = 0; break; }
        case 1: {_g_current_quality = _getLinePourcent(_counter2, _nbPointToDraw(_pr, _int16_buffer_dim), 1, 25); break; }
        case 2: {_g_current_quality = _getLinePourcent(_qj, (int)_int16_buffer_dim.Y(), 26, 99); break; }
        case 3: {_g_current_quality = 100; break; }
        default: MTOOLS_INSURE(false); // wtf are we doing here
        }
    return;
    }


/* draw as much as possible of a fast drawing, return true if finished false otherwise
  if finished, then _qi,_qj are set to zero and counter1 = counter2 has the correct value */
void _drawPixel_fast(int maxtime_ms)
	{
    const fRect r = _pr;
    const double px = ((double)r.lx()) / ((double)_int16_buffer_dim.X())  // size of a pixel
               , py = ((double)r.ly()) / ((double)_int16_buffer_dim.Y()); 
	_counter1 = 1;
	bool fixstart = true; 
    RGBc coul;
    int64 prevsx = (int64)floor(r.xmin) - 2;
    int64 prevsy = (int64)floor(r.ymax) + 2;
    for (int j = 0; j < _int16_buffer_dim.Y(); j++)
    for (int i = 0; i < _int16_buffer_dim.X(); i++)
		{
		if (fixstart) {i = _qi; j = _qj; fixstart=false;}					        // fix the position of thestarting pixel 
		if (_isTime(maxtime_ms)) {_qi = i; _qj = j; return;}	                    // time's up : we quit
		double x = r.xmin + (i + 0.5)*px, y = r.ymax - (j + 0.5)*py;            	// pick the center point inside the pixel
		int64 sx = (int64)floor(x + 0.5); int64 sy = (int64)floor(y + 0.5); 		// compute the integer position which covers it
        if ((prevsx != sx) || (prevsy != sy)) 
            { // not the same point as before
            coul = _g_obj->getColor({ sx, sy });
            prevsx = sx; prevsy = sy;
            }
        _setInt16Buf(i, j, coul);						    // set the color in the buffer
		}
	// we are done
	_counter2 = _counter1; _qi=0; _qj=0;
    if (_skipStochastic(r, _int16_buffer_dim)) { _phase = 2; } else { _phase = 1; } // go to next phase, skip stochastic if not needed.
	return;
	}


/* draw as much as possible of a stochastic drawing, return true if finished, false otherwise
  if finished, then _qi,_qj are set to zero and counter1 = counter2 has the correct value */
void _drawPixel_stochastic(int maxtime_ms)
	{
    const fRect r = _pr;
    const double px = ((double)r.lx()) / ((double)_int16_buffer_dim.X())  // size of a pixel
               , py = ((double)r.ly()) / ((double)_int16_buffer_dim.Y());
    uint32 ndraw = _nbDrawPerTurn(r, _int16_buffer_dim);
    while(_counter2 < _nbPointToDraw(r, _int16_buffer_dim))
		{
		if (_counter2 == _counter1) {++_counter1;} // start of a loop: we increase counter1 
		bool fixstart = true; 
        for (int j = 0; j < _int16_buffer_dim.Y(); j++)
        for (int i = 0; i < _int16_buffer_dim.X(); i++)
			{
			if (fixstart) {i = _qi; j = _qj; fixstart=false;}		// fix the position of thestarting pixel
            if (_isTime(maxtime_ms)) { _qi = i; _qj = j; return; }	// time's up : we quit
			uint32 R=0,G=0,B=0,A=0;
 			for(uint32 k=0;k<ndraw;k++)
				{
				double x = r.xmin + (i + _rand_double0())*px, y = r.ymax - (j + _rand_double0())*py; 	// pick a point at random inside the pixel
				int64 sx = (int64)floor(x + 0.5); int64 sy = (int64)floor(y + 0.5);     			// compute the integer position which covers it
                RGBc coul = _g_obj->getColor({ sx, sy }); 			                     				// get the color of the site
                R += coul.R; G += coul.G; B += coul.B; A += coul.A;
				}
			_addInt16Buf(i,j,R/ndraw,G/ndraw,B/ndraw,A/ndraw);
			}
		// we finished a loop
		_counter2 = _counter1;	_qi=0; _qj=0;
		}
    _phase = 2; // go to next phase
	return;
	}


/* draw as much as possible of a perfect drawing, return true if finished, false otherwise
  if finished, then _qi,_qj are set to zero and counter1 = counter2 has the correct value */
void _drawPixel_perfect(int maxtime_ms)
	{
    const fRect r = _pr;
    const double px = ((double)r.lx()) / ((double)_int16_buffer_dim.X())  // size of a pixel
               , py = ((double)r.ly()) / ((double)_int16_buffer_dim.Y());
	_counter1 = 1; // counter1 must be 1
	bool fixstart = true; 
    RGBc coul;
    int64 pk = (int64)floor(r.xmin) - 2;
    int64 pl = (int64)floor(r.ymax) + 2;
    for (int j = 0; j < _int16_buffer_dim.Y(); j++)
    for (int i = 0; i < _int16_buffer_dim.X(); i++)
		{
		if (fixstart) {i = _qi; j = _qj; fixstart=false;}	// fix the position of thestarting pixel 
		fRect pixr(r.xmin + i*px,r.xmin + (i+1)*px,r.ymax - (j+1)*py,r.ymax - j*py); // the rectangle corresponding to pixel (i,j)
		iRect ipixr = pixr.integerEnclosingRect(); // the integer sites whose square intersect the pixel square
		double cr=0.0 ,cg=0.0, cb=0.0, ca=0.0, tot=0.0;
		for(int64 k=ipixr.xmin;k<=ipixr.xmax;k++) for(int64 l=ipixr.ymin;l<=ipixr.ymax;l++) // iterate over all those points
			{
            if (_isTime(maxtime_ms)) { _qi = i; _qj = j; return; } // time's up : we quit and abandon this pixel
			double a = pixr.pointArea((double)k,(double)l); // get the surface of the intersection
            if ((k != pk) || (l != pl))
                {
                coul = _g_obj->getColor({ k, l });
                pk = k; pl = l;
                }
            cr += (coul.R*a); cg += (coul.G*a); cb += (coul.B*a); ca += (coul.A*a); // get the color and add it proportionally to the intersection
			tot+=a;
			}
		_setInt16Buf(i,j,cr/tot,cg/tot,cb/tot,ca/tot);
		}
	_qi=0; _qj=0; _counter2 = _counter1;
    _phase = 3; // we are done, perfect drawing !
	return;
	}


/* the main method for drawing a pixel image */
void _workPixel(int maxtime_ms)
    {
    _startTimer();
    if (_g_imSize != _int16_buffer_dim) { _g_redraw_pix = true; } //set redraw to true if the size of the image changed
    if (_g_r != _pr) _g_redraw_pix = true; // set redraw to true if the range changed
    if (_g_redraw_pix)
        { // we must completly redraw, initialize everything
        _g_redraw_pix = false;
        _pr = _g_r;
        _qi = 0; _qj = 0;
        _counter1 = 0; _counter2 = 0;
        _resizeInt16Buf(_g_imSize);
        _phase = 0;
        }
    if (maxtime_ms > 0) 
        {
        while ((_phase != 3) && (!_isTime(maxtime_ms)))
            {
            switch (_phase)
                {
                case 0: // fast drawing phase : start from _qi,qj and make the fastest drawing possible
                    {
                    _drawPixel_fast(maxtime_ms);
                    break;
                    }
                case 1: // stochastic drawing phase : start from _qi,qj and make a stochastic drawing
                    {
                    _drawPixel_stochastic(maxtime_ms);
                    break;
                    }
                case 2: // perfect drawing phase : start from _qi,qj and make a stochastic drawing
                    {                    
                    _drawPixel_perfect(maxtime_ms);
                    break;
                    }
                default: MTOOLS_INSURE(false); // wtf are we doing here
                }
            }
        }
    _qualityPixelDraw(); // update the quality
    return;
    }



// *****************************
// Dealing with the int16 buffer 
// *****************************
uint16 * _int16_buffer;						// buffer for pixel drawing
iVec2    _int16_buffer_dim;                 // dimension of the buffer;

/* initialise the buffer */
inline void _initInt16Buf()
	{
	_int16_buffer = nullptr; 
    _int16_buffer_dim = iVec2(0, 0);
	}

/* remove the int16 buffer */
inline void _removeInt16Buf() {_resizeInt16Buf(iVec2(0,0));}

/* resize the buffer to the given dimension */
inline void _resizeInt16Buf(iVec2 nSize)
	{
    const int64 prod = nSize.X()*nSize.Y();
	if (prod == _int16_buffer_dim.X()*_int16_buffer_dim.Y()) {return;}
	delete []  _int16_buffer; 
    if (prod == 0) { _int16_buffer = nullptr; _int16_buffer_dim = iVec2(0, 0); return; }
	_int16_buffer = new uint16[(size_t)(prod*4)];
    _int16_buffer_dim = nSize;
	}

/* set a color at position (i,j) */
inline void _setInt16Buf(uint32 x,uint32 y,const RGBc & color)
	{
    const size_t dx = (size_t)_int16_buffer_dim.X();
    const size_t dxy = (size_t)(dx * _int16_buffer_dim.Y());
    _int16_buffer[x + y*dx] = color.R;
    _int16_buffer[x + y*dx + dxy] = color.G;
    _int16_buffer[x + y*dx + 2 * dxy] = color.B;
    _int16_buffer[x + y*dx + 3 * dxy] = color.A;

    }

/* set a color at position (i,j) */
inline void _setInt16Buf(uint32 x,uint32 y,double R,double G,double B,double A)
	{
    const size_t dx = (size_t)_int16_buffer_dim.X();
    const size_t dxy = (size_t)(dx * _int16_buffer_dim.Y());
    _int16_buffer[x + y*dx] = (uint16)round(R);
	_int16_buffer[x + y*dx + dxy] = (uint16)round(G);
	_int16_buffer[x + y*dx + 2*dxy] = (uint16)round(B);
    _int16_buffer[x + y*dx + 3*dxy] = (uint16)round(A);
    }

/* add a color at position (i,j) */
inline void _addInt16Buf(uint32 x,uint32 y,uint32 R,uint32 G,uint32 B,uint32 A)
	{
    const size_t dx = (size_t)_int16_buffer_dim.X();
    const size_t dxy = (size_t)(dx * _int16_buffer_dim.Y());
    _int16_buffer[x + y*dx] += R;
	_int16_buffer[x + y*dx + dxy] += G;
	_int16_buffer[x + y*dx + 2*dxy] += B;
    _int16_buffer[x + y*dx + 3*dxy] += A;
    }



/* the main method for warping the pixel image to the cimg image*/
void _drawOntoPixel(cimg_library::CImg<unsigned char> & im, float opacity)
    {
    _workPixel(0); // make sure everything is in sync. 
    if (_g_current_quality > 0)
        {
        if (im.spectrum() == 4)
            {
            _warpInt16Buf_4channel(im, opacity);
            }
        else
            {
            if (opacity >= 1.0) _warpInt16Buf_opaque(im); else _warpInt16Buf(im, opacity);
            }
        }
    return;
    }



/* warp the buffer onto an image using _qi,_qj,_counter1 and _counter2 : fast method when opacity = 1.0*/
inline void _warpInt16Buf(CImg<unsigned char> & im,const float op) const
	{
    const float po = 1 - op;
    const size_t dx = (size_t)_int16_buffer_dim.X();
    const size_t dxy = (size_t)(dx * _int16_buffer_dim.Y());
	size_t l1 = _qi + (dx*_qj);
	size_t l2 = (dxy) - l1;
	for(int c=0;c<im.spectrum();c++)
		{
		if (l1>0)
			{
			unsigned char * pdest = im.data(0,0,0,c);
			uint16 * psource = _int16_buffer + c*dxy;
			if (_counter1==0) {/* memset(pdest,0,l1); */}
			else 
				{     
                if (_counter1 == 1) { for (size_t i = 0; i < l1; i++) { (*pdest) = (unsigned char)((*pdest)*po + (*psource)*op); ++pdest; ++psource; } }
                else { for (size_t i = 0; i<l1; i++) { (*pdest) = (unsigned char)((*pdest)*po + (*psource)*op/_counter1); ++pdest; ++psource; } }
				}
			}
		if (l2>0)
			{
			unsigned char * pdest = im.data(_qi,_qj,0,c);
			uint16 * psource      = _int16_buffer + c*dxy + l1;
			if (_counter2==0) {/* memset(pdest,0,l2); */}
			else 
				{
                if (_counter2 == 1) { for (size_t i = 0; i<l2; i++) { (*pdest) = (unsigned char)((*pdest)*po + (*psource)*op); ++pdest; ++psource; } }
                else { for (size_t i = 0; i<l2; i++) { (*pdest) = (unsigned char)((*pdest)*po + (*psource)*op / _counter2);  ++pdest; ++psource; } }
				}
			}
		}
	return;
	}

/* warp the buffer onto an image using _qi,_qj,_counter1 and _counter2 : fast method when opacity = 1.0*/
inline void _warpInt16Buf_opaque(CImg<unsigned char> & im) const
{
    const size_t dx = (size_t)_int16_buffer_dim.X();
    const size_t dxy = (size_t)(dx * _int16_buffer_dim.Y());
    size_t l1 = _qi + (dx*_qj);
    size_t l2 = (dxy)-l1;
    for (int c = 0; c< im.spectrum(); c++)
    {
        if (l1>0)
        {
            unsigned char * pdest = im.data(0, 0, 0, c);
            uint16 * psource = _int16_buffer + c*dxy;
            if (_counter1 == 0) {/* memset(pdest,0,l1); */ } else
            {
                if (_counter1 == 1) { for (size_t i = 0; i<l1; i++) { (*pdest) = (unsigned char)(*psource); ++pdest; ++psource; } } else { for (size_t i = 0; i<l1; i++) { (*pdest) = (unsigned char)((*psource) / _counter1); ++pdest; ++psource; } }
            }
        }
        if (l2>0)
        {
            unsigned char * pdest = im.data(_qi, _qj, 0, c);
            uint16 * psource = _int16_buffer + c*dxy + l1;
            if (_counter2 == 0) {/* memset(pdest,0,l2); */ } else
            {
                if (_counter2 == 1) { for (size_t i = 0; i<l2; i++) { (*pdest) = (unsigned char)(*psource); ++pdest; ++psource; } } else { for (size_t i = 0; i<l2; i++) { (*pdest) = (unsigned char)((*psource) / _counter2);  ++pdest; ++psource; } }
            }
        }
    }
    return;
}

/* make B -> A */
inline unsigned char _blendcolor(unsigned char & A, float opA, unsigned char B, float opB ) const
    {
    float o = opB + opA*(1 - opB);
    if (o == 0)  return 0;
    A = (unsigned char)((B*opB + A*opA*(1 - opB))/o);
    return (unsigned char)(255 * o);
    }

/* warp the buffer onto an image using _qi,_qj,_counter1 and _counter2 : four channel on the image : use transparency 
 * PARTIAL SUPPORT */
inline void _warpInt16Buf_4channel(CImg<unsigned char> & im, float op) const
{
    const float po = 1.0;
    const size_t dx = (size_t)_int16_buffer_dim.X();
    const size_t dxy = (size_t)(dx * _int16_buffer_dim.Y());
    const size_t l1 = _qi + (dx*_qj);
    const size_t l2 = (dxy)-l1;
    if (l1>0)
        {
        unsigned char * pdest0 = im.data(0, 0, 0, 0);
        unsigned char * pdest1 = im.data(0, 0, 0, 1);
        unsigned char * pdest2 = im.data(0, 0, 0, 2);
        unsigned char * pdest_opa = im.data(0, 0, 0, 3);
        uint16 * psource0 = _int16_buffer;
        uint16 * psource1 = _int16_buffer + dxy;
        uint16 * psource2 = _int16_buffer + 2*dxy;
        uint16 * psource_opb = _int16_buffer + 3 * dxy;
        if (_counter1 == 0) {/* memset(pdest,0,l1); */ } else
            {
            if (_counter1 == 1) 
                { 
                for (size_t i = 0; i < l1; i++) 
                    { 
                    _blendcolor((*pdest0), ((po*(*pdest_opa)) / 255), (unsigned char)(*psource0), ((op*(*psource_opb)) / 255));
                    _blendcolor((*pdest1), ((po*(*pdest_opa)) / 255), (unsigned char)(*psource1), ((op*(*psource_opb)) / 255));
                    (*pdest_opa) = _blendcolor((*pdest2), ((po*(*pdest_opa)) / 255), (unsigned char)(*psource2), ((op*(*psource_opb)) / 255));
                    ++pdest0; ++pdest1; ++pdest2; ++pdest_opa;
                    ++psource0; ++psource1; ++psource2; ++psource_opb;
                    } 
                }
            else 
               { 
               for (size_t i = 0; i < l1; i++) 
                    { 
                    _blendcolor((*pdest0), ((po*(*pdest_opa)) / 255), (unsigned char)((*psource0) / _counter1), ((op*(*psource_opb) / _counter1) / 255));
                    _blendcolor((*pdest1), ((po*(*pdest_opa)) / 255), (unsigned char)((*psource1) / _counter1), ((op*(*psource_opb) / _counter1) / 255));
                    (*pdest_opa) = _blendcolor((*pdest2), ((po*(*pdest_opa)) / 255), (unsigned char)((*psource2) / _counter1), ((op*(*psource_opb) / _counter1) / 255));
                    ++pdest0; ++pdest1; ++pdest2; ++pdest_opa;
                    ++psource0; ++psource1; ++psource2; ++psource_opb;
                    } 
                }
            }
        }
    if (l2>0)
        {
        unsigned char * pdest0 = im.data(_qi, _qj, 0, 0);
        unsigned char * pdest1 = im.data(_qi, _qj, 0, 1);
        unsigned char * pdest2 = im.data(_qi, _qj, 0, 2);
        unsigned char * pdest_opa = im.data(_qi, _qj, 0, 3);
        uint16 * psource0 = _int16_buffer +  + l1;
        uint16 * psource1 = _int16_buffer + dxy + l1;
        uint16 * psource2 = _int16_buffer + 2*dxy + l1;
        uint16 * psource_opb = _int16_buffer + 3*dxy + l1;
        if (_counter2 == 0) {/* memset(pdest,0,l2); */ } else
            {
            if (_counter2 == 1) 
                {
                for (size_t i = 0; i<l2; i++) 
                    { 
                    _blendcolor((*pdest0), (po*(*pdest_opa)) / 255, (unsigned char)(*psource0), (op*(*psource_opb)) / 255);
                    _blendcolor((*pdest1), (po*(*pdest_opa)) / 255, (unsigned char)(*psource1), (op*(*psource_opb)) / 255);
                    (*pdest_opa) = _blendcolor((*pdest2), (po*(*pdest_opa)) / 255, (unsigned char)(*psource2), (op*(*psource_opb)) / 255);
                    ++pdest0; ++pdest1; ++pdest2; ++pdest_opa;
                    ++psource0; ++psource1; ++psource2; ++psource_opb;
                    }
                }
            else 
                { 
                for (size_t i = 0; i<l2; i++) 
                    { 
                    _blendcolor((*pdest0), (po*(*pdest_opa)) / 255, (*psource0) / _counter2, (op*(*psource_opb) / _counter2) / 255); 
                    _blendcolor((*pdest1), (po*(*pdest_opa)) / 255, (*psource1) / _counter2, (op*(*psource_opb) / _counter2) / 255);
                    (*pdest_opa) = _blendcolor((*pdest2), (po*(*pdest_opa)) / 255, (*psource2) / _counter2, (op*(*psource_opb) / _counter2) / 255);
                    ++pdest0; ++pdest1; ++pdest2; ++pdest_opa;
                    ++psource0; ++psource1; ++psource2; ++psource_opb;
                    }
                }
            }
         }
    return;
}









// ****************************************************************
// THE IMAGE DRAWER
// ****************************************************************

/* for the image drawer */
CImg<unsigned char> _exact_qbuf;			// quality buffer of the same size as exact_im: 0 = not drawn. 1 = dirty. 2 = clean 
CImg<unsigned char> _exact_im;		    	// the non-rescaled image of size (_wr.lx()*_exact_sx , _wr.ly()*_exact_sy)
int					_exact_sx,_exact_sy;	// size of a site image in the exact image
iRect				_exact_r;				// the rectangle describing the sites in the exact_image
int					_exact_qi,_exact_qj;	// position we continue from for improving quality
int					_exact_phase;			// 0 = remain undrawn sites, 1 = remain dirty site, 2 = finished
uint32              _exact_Q0;              // number of images not drawn 
uint32              _exact_Q23;             // number of images of good quality


/* version when LatticeObj implement the getImage method */
inline const CImg<unsigned char > * _getimage(int64 i, int64 j, int lx, int ly, mtools::metaprog::dummy<true> D)
    {
    return _g_obj->getImage({ i, j }, { lx, ly });
    }

/* fallback method when LatticeObj does not implement getImage() method */
inline const CImg<unsigned char> * _getimage(int64 i, int64 j, int lx, int ly, mtools::metaprog::dummy<false> D)
    {
    MTOOLS_INSURE(false);
    return(nullptr);
    }


/* improve the quality of the image */
void _improveImage(int maxtime_ms)
	{
    if ((maxtime_ms == 0) || (_isTime2(maxtime_ms))) { _qualityImageDraw(); return; }
	while(1)
		{
		switch(_exact_phase)
			{
			case 0:
				{
				bool fixstart = true;
				cimg_forXY(_exact_qbuf,i,j)
					{
					if (fixstart) {i = _exact_qi; j= _exact_qj; fixstart = false;}
                    if (_isTime2(maxtime_ms)) { _exact_qi = i; _exact_qj = j; _qualityImageDraw();  return; }
					if (_exact_qbuf(i,j) == 0) // site must be redrawn
						{
                        --_exact_Q0;
                        const CImg<unsigned char>  * spr = _getimage(_exact_r.xmin + i, _exact_r.ymin + j, _exact_sx, _exact_sy, metaprog::dummy< metaprog::has_getImage<LatticeObj, const cimg_library::CImg<unsigned char>*, mtools::iVec2, mtools::iVec2>::value >());
                        if (spr == nullptr) { _exact_qbuf(i, j) = 3; ++_exact_Q23; } // no image, don't do anything
                        else
                            {
                            MTOOLS_ASSERT((spr->spectrum() == 3) || (spr->spectrum() == 4));
                            MTOOLS_ASSERT(spr->height()*spr->width() > 0);
                            if ((spr->width() == _exact_sx) && (spr->height() == _exact_sy))
                                { // good, image is at the right size. We do not need to copy it.
                                _exact_qbuf(i, j) = 2; ++_exact_Q23;
                                _exact_im.draw_image(_exact_sx*i, _exact_sy*(_exact_qbuf.height() - 1 - j), 0, 0, *spr); // copy
                                } 
                            else
                                { // not at the right dimension, we resize before blitting
                                _exact_qbuf(i, j) = 1;
                                CImg<unsigned char> sprite = (*spr).get_resize(_exact_sx, _exact_sy, 1, spr->spectrum(), 1); //fast resizing
                                _exact_im.draw_image(_exact_sx*i, _exact_sy*(_exact_qbuf.height() - 1 - j), 0, 0, sprite); // copy
                                }
                            if (spr->spectrum() == 3) // fill the last channel with 255 (opaque) if the sprite only has 3 channel
                                {
                                const int mxmin = _exact_sx*i;
                                const int mxmax = mxmin + _exact_sx;
                                const int mymin = _exact_sy*(_exact_qbuf.height() - 1 - j);
                                const int mymax = mymin + _exact_sy;
                                for (int mj = mymin; mj < mymax; mj++) { for (int mi = mxmin; mi < mxmax; mi++) { _exact_im(mi, mj, 0, 3) = 255; } }
                                }
                            }
                        }
					}
				_exact_qi = 0; _exact_qj =0;
				_exact_phase = 1;
                if (_exact_Q23 == (uint32)(_exact_qbuf.width()*_exact_qbuf.height())) { _exact_phase = 2; }
                break;
				}
			case 1:
                {
				bool fixstart = true;
				cimg_forXY(_exact_qbuf,i,j)
					{
					if (fixstart) {i = _exact_qi; j= _exact_qj; fixstart = false;}
                    if (_isTime2(maxtime_ms)) { _exact_qi = i; _exact_qj = j; _qualityImageDraw();  return; }
					if (_exact_qbuf(i,j) == 1) // site must be redrawn
						{
                        _exact_Q23++;
                        const CImg<unsigned char>  * spr = _getimage(_exact_r.xmin + i, _exact_r.ymin + j, _exact_sx, _exact_sy, metaprog::dummy< metaprog::has_getImage<LatticeObj, const cimg_library::CImg<unsigned char>*, mtools::iVec2, mtools::iVec2>::value >());
                        if (spr == nullptr) { _exact_qbuf(i, j) = 3; } // no image (a change in the lattice occured betwen phase 0 and 1) don't do anything
                        else
                            {
                            MTOOLS_ASSERT((spr->spectrum() == 3) || (spr->spectrum() == 4));
                            MTOOLS_ASSERT(spr->height()*spr->width() > 0);
                            _exact_qbuf(i, j) = 2;
                            if ((spr->width() == _exact_sx) && (spr->height() == _exact_sy))
                                { // weird, this second time it is at the right dimension... anyway, that's good for us... 
                                _exact_im.draw_image(_exact_sx*i, _exact_sy*(_exact_qbuf.height() - 1 - j), 0, 0, *spr); // copy
                                }
                            else
                                { // still not at the right dimension, we resize before blitting
                                CImg<unsigned char> sprite = (*spr).get_resize(_exact_sx, _exact_sy, 1, sprite.spectrum(), 5); //quality resizing
                                _exact_im.draw_image(_exact_sx*i, _exact_sy*(_exact_qbuf.height() - 1 - j), 0, 0, sprite); // copy
                                }
                            if (spr->spectrum() == 3) // fill the last channel with 255 (opaque) if the sprite only has 3 channel
                                {
                                const int mxmin = _exact_sx*i;
                                const int mxmax = mxmin + _exact_sx;
                                const int mymin = _exact_sy*(_exact_qbuf.height() - 1 - j);
                                const int mymax = mymin + _exact_sy;
                                for (int mj = mymin; mj < mymax; mj++) { for (int mi = mxmin; mi < mxmax; mi++) { _exact_im(mi, mj, 0, 3) = 255; } }
                                }
                            }
						}
					}
				_exact_qi = 0; _exact_qj =0;
				_exact_phase = 2;
				}
			case 2: 
				{
                _qualityImageDraw();
                return;
				}
			default:
				{
                MTOOLS_INSURE(false); // wtf are we doing here
				}
			}
		}	
	}



/* return true if we should try to keep part of the old image and blit it into the new one */
inline bool _keepOldImage(int newim_lx,int newim_ly) const
	{
	if ((((int64)newim_lx)*(newim_ly)*4)  > (1024*1024*128)) {return false;} // do not keep if it needs to create a buffers larger than 128MB
	return true;
	}



/* redraw the _exact_im image */
void _redrawImage(iRect new_wr, int new_sx, int new_sy, int maxtime_ms)
{
    if ((!_g_redraw_im) && (new_wr == _exact_r) && (_exact_sx == new_sx) && (_exact_sy == new_sy)) { _improveImage(maxtime_ms); return; } //ok, we can keep previous work and we continu from there on
    if (maxtime_ms == 0) { _g_current_quality = 0;  return; }; // nothing to show.
    const int32 new_im_x = (int32)(new_wr.lx() + 1)*new_sx; // size of the new image    
    const int32 new_im_y = ((int32)new_wr.ly() + 1)*new_sy;	// 
    _exact_Q0 = (int)((new_wr.lx() + 1)*(new_wr.ly() + 1));
    _exact_Q23 = 0;
    auto prevphase = _exact_phase;
    _exact_phase = 0;
    if ((!_g_redraw_im) && (_keepOldImage(new_im_x, new_im_y)) && (prevphase >= 1))
        { // we try to keep something from the previous image
        CImg<unsigned char> new_im(new_im_x, new_im_y, 1, 4, 255);
        CImg<unsigned char> new_qbuf((int32)new_wr.lx() + 1, (int32)new_wr.ly() + 1, 1, 1, 0);  // create buffer with zeros
        // there has been some change but we may be able to keep something
        //const int32 im_x = _exact_im.width();   // size of the current image
        //const int32 im_y = _exact_im.height();	//
        bool samescale = ((new_sx == _exact_sx) && (new_sy == _exact_sy)); // true if we are on the same scale as before
        iRect in_newR = new_wr.relativeSubRect(_exact_r); // the intersection rectangle seen as a sub rectangle of the new site rectangle
        iRect in_oldR = _exact_r.relativeSubRect(new_wr); // the intersection rectangle seen as a sub rectangle of the old site rectangle
        if (!in_newR.isEmpty()) // intersection is not empty, we fill the new quality with what we keep from the old image
            {
            for (int i = 0; i < (in_newR.lx() + 1); i++)  for (int j = 0; j < (in_newR.ly() + 1); j++)
                {
                unsigned char v = _exact_qbuf((int32)in_oldR.xmin + i, (int32)in_oldR.ymin + j); // get the quality of the site
                if ((v == 2) && (!samescale)) { v = 1; } // downgrade quality if we are changing scale
                if (v != 0) { --_exact_Q0; if (v >= 2) { ++_exact_Q23; } }
                new_qbuf((int32)in_newR.xmin + i, (int32)in_newR.ymin + j) = v; // save the quality in the new quality buffer
                }
            if (!samescale)
                {
                _exact_im.crop((int32)in_oldR.xmin*_exact_sx, (int32)(_exact_r.ly() - in_oldR.ymax)*_exact_sy, 0, 0, (int32)(in_oldR.xmax + 1)*_exact_sx - 1, (int32)(_exact_r.ly() - in_oldR.ymin + 1)*_exact_sy - 1, 0, 3); // crop the old image keeping only the part we reuse  
                _exact_im.resize((int32)(in_newR.lx() + 1)*new_sx, (int32)(in_newR.ly() + 1)*new_sy, 1, 4, 1); // resize quickly (bad quality resizing)
                new_im.draw_image((int32)in_newR.xmin*new_sx, new_im.height() - _exact_im.height() - (int32)in_newR.ymin*new_sy, 0, 0, _exact_im); // copy it at the right position in the new image
                }
            else
                {
                _exact_im.crop((int32)in_oldR.xmin*_exact_sx, (int32)(_exact_r.ly() - in_oldR.ymax)*_exact_sy, 0, 0, (int32)(in_oldR.xmax + 1)*_exact_sx - 1, (int32)(_exact_r.ly() - in_oldR.ymin + 1)*_exact_sy - 1, 0, 3); // crop the old image keeping only the part we reuse
                //const int Z = new_im.height() - _exact_im.height() - (int32)in_newR.ymin*new_sy;
                new_im.draw_image((int32)in_newR.xmin*new_sx, new_im.height() - _exact_im.height() - (int32)in_newR.ymin*new_sy, 0, 0, _exact_im); // copy it at the right position in the new image
                }
            }
        new_qbuf.move_to(_exact_qbuf);
        new_im.move_to(_exact_im);
        if (_exact_Q0 == 0) { _exact_phase = 1; } // should not happen
        if (_exact_Q23 == (uint32)(_exact_qbuf.width()*_exact_qbuf.height())) { _exact_phase = 2; } // even less so...
        }
    else
        { // we start from scratch
        _g_redraw_im = false;
        _exact_im.assign(new_im_x, new_im_y, 1, 4, 0); // blank image with 4 channel RGB(0,0,0,0)
        _exact_qbuf.assign((int32)new_wr.lx() + 1, (int32)new_wr.ly() + 1, 1, 1, 0);
        }
    // done, we update the member and start improving the image
    _exact_r = new_wr;
    _exact_sx = new_sx;
    _exact_sy = new_sy;
    _exact_qi = 0;
    _exact_qj = 0;
 	_improveImage(maxtime_ms);
    return;
}





/* compute the size of an image site, try to keep the same as before */
void _adjustSiteImageSize(int & sx,int & sy,int winx,int winy,const fRect & pr) const
	{
	double fsx = ((double)winx)/((double)pr.lx());
	double fsy = ((double)winy)/((double)pr.ly());
	if (std::abs(fsx - _exact_sx) < 1) {sx = _exact_sx;} else {sx = (int)ceil(fsx -0.5);}
	if (std::abs(fsy - _exact_sy) < 1) {sy = _exact_sy;} else {sy = (int)ceil(fsy -0.5);}
	return;
	}


void _workImage(int maxtime_ms)
{
    _startTimer();
    if (_exact_r.isEmpty()) { _g_redraw_im = true; } // just to be on the safe side..
    iRect ir = _g_r.integerEnclosingRect(); // compute the enclosing integer rectangle	
    int sx, sy;
    _adjustSiteImageSize(sx, sy, (int)_g_imSize.X(), (int)_g_imSize.Y(), _g_r); // compute the size of a site (sx,sy) (trying to keep the previous scaling (_exact_sx,_exact_sy) is possible)
    _redrawImage(ir, sx, sy, maxtime_ms); // redraw/improve the image on exact_im
    return;
}



/* update the quality of the current drawing */
void _qualityImageDraw() const
{
    const int32 N = _exact_qbuf.width()*_exact_qbuf.height();
    if (_exact_phase == 0) {_g_current_quality = _getLinePourcent(N - _exact_Q0, N, 0, 0); return; } // consider that the drawing is zero until all the site have been drawn at least in bad quality
    if (_exact_phase == 1) {_g_current_quality = _getLinePourcent(_exact_Q23, N, 2, 99); return;}
    _g_current_quality = 100;
}



inline void _drawOntoImage(cimg_library::CImg<unsigned char> & im, float op) 
{
    MTOOLS_ASSERT((im.spectrum() == 3) || (im.spectrum() == 4));
    MTOOLS_ASSERT((im.width() == _g_imSize.X()) && (im.height() == _g_imSize.Y()));
    _workImage(0); // make sure everything is in sync. 
        if (_g_current_quality > 0)
        {
        iRect ir = _g_r.integerEnclosingRect(); // compute the enclosing integer rectangle	
        fRect fir(ir.xmin - 0.5, ir.xmax + 0.5, ir.ymin - 0.5, ir.ymax + 0.5); // this is exactly the region drawn by _exact_im
        fRect rr = fir.relativeSubRect(_g_r);
        const int pxmin = (int)(((rr.xmin) / fir.lx())*_exact_im.width());
        const int pxmax = (int)(((rr.xmax) / fir.lx())*_exact_im.width());
        const int pymin = (int)(((rr.ymin) / fir.ly())*_exact_im.height());
        const int pymax = (int)(((rr.ymax) / fir.ly())*_exact_im.height());
        const int ax = pxmin;
        const int bx = pxmax - 1;
        const int ay = _exact_im.height() - pymax;
        const int by = _exact_im.height() - 1 - pymin;
        const int lx = bx - ax + 1;
        const int ly = by - ay + 1;
        const int nx = im.width();
        const int ny = im.height();
        // we must resize the portion [ax, bx]x[ay, by] of _exact_im to size (nx,ny) and put it in im.
        const double stepx = ((double)lx)/((double)nx);
        const double stepy = ((double)ly)/((double)ny);
        // iterate over the pixels of im
        if (im.spectrum() == 3)
            { // RGB image without transparency,
            if (op >= 1.0)
                { // opaque copy is fastest..
                cimg_forC(im, c)
                    {    
                    cimg_forXY(im, i, j)
                        {
                            const int x = ax + (int)(stepx*i);  // the corresponding pixel in exactim 
                            const int y = ay + (int)(stepy*j);  //
                            const int qqi = x / _exact_sx;                           // the associated quality buffer
                            const int qqy = _exact_qbuf.height() - 1 - y / _exact_sy;  //
                            if ((_exact_qbuf(qqi, qqy) != 3) && (_exact_qbuf(qqi, qqy) != 0)) // skip pixels that belong to sites without image or not yet dr
                            {
                            im(i, j, 0, c) = _exact_im(x, y, 0, c);
                            }
                        }
                    }
                }
            else
                { // not opaque, superpose with the image.
                const float po = 1 - op;
                cimg_forC(im, c)
                    {
                    cimg_forXY(im, i, j)
                        {
                        const int x = ax + (int)(stepx*i);  // the corresponding pixel in exactim 
                        const int y = ay + (int)(stepy*j);  //
                        const int qqi = x / _exact_sx;                           // the associated quality buffer
                        const int qqy = _exact_qbuf.height() - 1 - y / _exact_sy;  //
                        if ((_exact_qbuf(qqi, qqy) != 3) && (_exact_qbuf(qqi, qqy) != 0))  // skip pixels that belong to sites without image or not yet drawn
                            {
                            im(i, j, 0, c) = (unsigned char)(po*im(i, j, 0, c) + op*_exact_im(x, y, 0, c));
                            }
                        }
                    }
                }
            }
        else
            { // 4 channel image.
                const float po = 1;
                cimg_forXY(im, i, j)
                    {
                    const int x = ax + (int)(stepx*i);  // the corresponding pixel in exactim 
                    const int y = ay + (int)(stepy*j);  //
                    const int qqi = x / _exact_sx;                           // the associated quality buffer
                    const int qqy = _exact_qbuf.height() - 1 - y / _exact_sy;  //
                    if ((_exact_qbuf(qqi, qqy) != 3) && (_exact_qbuf(qqi, qqy) != 0))  // skip pixels that belong to sites without image or not yet drawn
                        {
                                         _blendcolor(im(i, j, 0, 0), im(i, j, 0, 3)*po / 255, _exact_im(x, y, 0, 0), _exact_im(x, y, 0, 3)*op / 255);
                                         _blendcolor(im(i, j, 0, 1), im(i, j, 0, 3)*po / 255, _exact_im(x, y, 0, 1), _exact_im(x, y, 0, 3)*op / 255);
                        im(i, j, 0, 3) = _blendcolor(im(i, j, 0, 2), im(i, j, 0, 3)*po / 255, _exact_im(x, y, 0, 2), _exact_im(x, y, 0, 3)*op / 255);
                        }
                    }
            }
        }
    return;
}


// ****************************************************************
// TIME FUNCTIONS : independant of everything else
// ****************************************************************

static const int _maxtic = 100;	    // number of tic until we look for time
static const int _maxtic2 = 10;   	// number of tic until we look for time
int _tic;							// current tic
clock_t _stime;						// start time

/* start the timer */
inline void _startTimer() {_stime = clock(); _tic = _maxtic; }


/* return true when time ms milliseconds has passed since calling startTimer() */
inline bool _isTime(uint32 ms)
	{
	++_tic;
    if (_g_requestAbort > 0) {return true; }
    if (_tic < _maxtic) return false;
    if (_g_drawingtype == TYPEPIXEL) { _qualityPixelDraw(); } else { _qualityImageDraw(); } // update the quality of the drawing
	if (((clock() - _stime)*1000)/CLOCKS_PER_SEC > (clock_t)ms) {_tic = _maxtic; return true;}
	_tic = 0;
	return false;
	}

/* check more often */
inline bool _isTime2(uint32 ms)
    {
    ++_tic;
    if (_g_requestAbort > 0) {return true; }
    if (_tic < _maxtic2) return false;
    if (_g_drawingtype == TYPEPIXEL) { _qualityPixelDraw(); } else { _qualityImageDraw(); } // update the quality of the drawing
    if (((clock() - _stime) * 1000) / CLOCKS_PER_SEC > (clock_t)ms) { _tic = _maxtic; return true; }
    _tic = 0;
    return false;
    }



// ****************************************************************
// RANDOM NUMBER GENERATION : independant of everything else
// ****************************************************************
uint32 _gen_x,_gen_y,_gen_z;		// state of the generator


/* initialize the random number generator */
inline void _initRand()
	{
	_gen_x = 123456789; 
	_gen_y = 362436069;
	_gen_z = 521288629;
	}

/* generate a uniform number in [0,1) */
inline double _rand_double0()
	{
	uint32 t;
	_gen_x ^= _gen_x << 16; _gen_x ^= _gen_x >> 5; _gen_x ^= _gen_x << 1;
	t = _gen_x; _gen_x = _gen_y; _gen_y = _gen_z; _gen_z = t ^ _gen_x ^ _gen_y;
	return(((double)_gen_z)/(4294967296.0));
	}


// ****************************************************************
// UTILITY FUNCTION : do not use any class member variable
// ****************************************************************


/* return the average number of site per pixel */
inline double _sitePerPixel(const fRect & r,const iVec2 & sizeIm) const {return (r.lx()/sizeIm.X())*(r.ly()/sizeIm.Y());}


/* return true if we should not use stochastic drawing */
inline bool _skipStochastic(const fRect & r, const iVec2 & sizeIm) const
	{
	if (_sitePerPixel(r,sizeIm) < 6) {return true;}
	return false;
	}


/* return the number of stochastic draw per pixel per turn */
inline uint32 _nbDrawPerTurn(const fRect & r,const iVec2 & sizeIm) const
	{
	return 5;
	}


/* return the number of stochastic turn to do */
inline uint32 _nbPointToDraw(const fRect & r,const iVec2 & sizeIm) const
	{
	int v = (int)_sitePerPixel(r,sizeIm)/20;
	if (v>254) {return 254;}
	if (v < 2) {return 2;}
	return v;
	}

/* return the pourcentage according to the line number of _qj */
inline int _getLinePourcent(int qj,int maxqj,int minv,int maxv) const
	{
	double v= ((double)qj)/((double)maxqj);
	int p = (int)(minv + v*(maxv-minv));
	return p;
	}


};

}

/* end of file */

