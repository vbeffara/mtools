/** @file grid_basic.hpp */
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

#include "misc/error.hpp"
#include "maths/vec.hpp"
#include "maths/rect.hpp"
#include "misc/metaprog.hpp"
#include "io/serialization.hpp"
#include "internals_grid.hpp"

#include <string>


namespace mtools
{

    /* forward declaration of the Grid_basic class*/
    template< size_t D, typename T, size_t NB_SPECIAL, size_t R> class Grid_factor;


    /**
     * A D-dimensional grid Z^d where each site contain an object of type T. This is a container
     * similar to a D-dimensionnal 'octree'. This is the 'basic' version: fastest get/set method and
     * need only T to be constructible. See Grid_factor for the more advanced (but with more
     * requirement on T) version where similar objects are 'factorized'.
     * 
     * - Each site of Z^d contain a unique objet of type T (which is not shared with any other
     * site). Objects at each sites are created on the fly. They are not deleted until the grid is
     * destroyed or reset.
     * 
     * - Internally, the grid Z^d is represented as a tree whose leafs are elementary sub-boxes of
     * the form [x-R,x+R]^D where R is a template parameter. Nodes of the tree have arity 3^D. The
     * tree grows dynamically to fit the set of sites accessed. This means that an object at some
     * position x may be created even without ever being accesssed.
     * 
     * - Access time to any given site is logarithmic with respect to its distance from the
     * previously accessed element.
     * 
     * - No objet of type T is ever deleted, copied or moved around during the whole life of the
     * grid. Thus, pointers/references to elements are never invalidated. Destructors of all the
     * objects created are called when the grid is destoyed or reset (unless the caller specifically
     * requests not to call the dtors).
     * 
     * - The type T need only be constructible with `T()` or `T(const Pos &)`. If both ctor exist,
     * the positional constructor is used. Furthermore, if T has a copy constructor, then the whole
     * grid object can be copied/assigned via the copy ctor/assignement operator (the assignement
     * operator of T is not used, even when assigning the grid).
     * 
     * - The grid can be serialized to file using the I/OArchive serialization classes and using the
     * default serialization method for T. Furthermore, it is possible to deserialize the object via
     * a specific constructor `T(IArchive &)` (this prevent having to construct a default object
     * first and then deserialize it).
     * 
     * - Grid_basic objects are compatible with Grid_factor objects (with the same template
     * parameters). Files saved with one object can be open with the other one and conversion using
     * copy construtor and assignement operators are implemented in both directions (provided, of
     * course, that the object T fullfills the requirement of Grid_factor and, conversely, that the
     * grid_factor objects do not have special objects at the time of conversion).
     *
     * @tparam  D   Dimension of the grid Z^D.
     * @tparam  T   Type of objects stored in each sites of the grid. This can be any type that has a
     *              public default constructor or a public positionnal constructor.
     * @tparam  R   Radius of an elementary box (leaf of the tree) i.e. each elementary box contain
     *              (2R+1)^D objects.
     *              | Dimension | default value for R
     *              |-----------|-------------------
     *              | 1         | 10000
     *              | 2         | 100
     *              | 3         | 20
     *              | 4         | 6
     *              | >= 5      | 1.
     *
     * @sa  Grid_factor
     **/
    template<size_t D, typename T, size_t R = internals_grid::defaultR<D>::val > class Grid_basic
    {

        template<size_t D2, typename T2, size_t NB_SPECIAL2, size_t R2> friend class Grid_factor;

        typedef internals_grid::_box<D, T, R> *     _pbox;
        typedef internals_grid::_node<D, T, R> *    _pnode;
        typedef internals_grid::_leaf<D, T, R> *    _pleaf;


    public:

        /**
         * Alias for a D-dimensional int64 vector representing a position in the grid.
         **/
        typedef iVec<D> Pos;


        /**
         * Constructor. An empty grid (no objet of type T is created).
         *
         * @param   callDtors   true (default) if we should call the destructor of T object when memory
         *                      is released. Setting this to false can speed up memory relase for basic
         *                      type that do not have 'important' destructors.
         **/
        Grid_basic(bool callDtors = true) : _pcurrent((_pbox)nullptr), _pcurrentpeek((_pbox)nullptr), _rangemin(std::numeric_limits<int64>::max()), _rangemax(std::numeric_limits<int64>::min()), _callDtors(callDtors) { _createBaseNode(); }


        /**
         * Constructor. Loads a grid from a file. If the loading fails, the grid is constructed empty.
         *
         * @param   filename    Filename of the file.
         **/
        Grid_basic(const std::string & filename) : _pcurrent((_pbox)nullptr), _pcurrentpeek((_pbox)nullptr), _rangemin(std::numeric_limits<int64>::max()), _rangemax(std::numeric_limits<int64>::min()), _callDtors(true) {load(filename);}
        
        
        /**
        * Constructor. Loads a grid from a file. If the loading fails, the grid is constructed empty.
        *
        * @param   str    Filename of the file.
        **/
        Grid_basic(const char * str) : _pcurrent((_pbox)nullptr), _pcurrentpeek((_pbox)nullptr), _rangemin(std::numeric_limits<int64>::max()), _rangemax(std::numeric_limits<int64>::min()), _callDtors(true) { load(std::string(str)); } // needed together with the std::string ctor to prevent implicit conversion to bool and call of the wrong ctor.


        /**
         * Destructor. Destroys the grid. The destructors of all the T objects in the grid are invoqued.
         * In order to prevent calling the dtors of T objects, invoque `callDtors(false)` prior to
         * destructing the grid.
         **/
        ~Grid_basic() { _destroyTree(); }


        /**
         * Copy Constructor. Makes a Deep copy of the grid. The source must have the same template
         * parameters D,T,R and the class T must be copyable by the copy operator `T(const T&)`.
         *
         * @param   G   The const Grid_basic<D,T,R> & to process.
         **/
        Grid_basic(const Grid_basic<D, T, R> & G) : _pcurrent((_pbox)nullptr), _pcurrentpeek((_pbox)nullptr), _rangemin(std::numeric_limits<int64>::max()), _rangemax(std::numeric_limits<int64>::min()), _callDtors(true)
            {
            static_assert(std::is_copy_constructible<T>::value, "The object T must be copy-constructible T(const T&) in order to use the copy constructor of the grid.");
            this->operator=(G);
            }


        /**
         * Copy constructor from a grid factor object. The source object must have the same template
         * parameters D,T,R and not have special values (use `Grid_factor::removeSpecialObjects()` to
         * remove special object if needed prior to converting).
         *
         * @param   G   The source Grid_factor object to copy.
         **/
        template<size_t NB_SPECIAL> Grid_basic(const Grid_factor<D, T, NB_SPECIAL, R> & G) : _pcurrent((_pbox)nullptr), _pcurrentpeek((_pbox)nullptr), _rangemin(std::numeric_limits<int64>::max()), _rangemax(std::numeric_limits<int64>::min()), _callDtors(true)
            {
            static_assert(std::is_copy_constructible<T>::value, "The object T must be copy-constructible T(const T&) in order to use the copy constructor of the grid.");
            this->operator=(G);
            }


        /**
         * Assignement operator. Create a deep copy of G. The source object must have the same template
         * parameters D,T,R. The class T must be copyable by the copy operator `T(const T&)` [the
         * assignement operator `operator=(const T &)` is never used and need not be defined]. If the
         * grid is not empty, it is first reset: all the objects inside are destroyed and their dtors
         * are invoqued if the callDtors flag is set.
         *
         * @param   G   the grid to copy.
         *
         * @return  the object for chaining.
         **/
        Grid_basic<D, T, R> & operator=(const Grid_basic<D, T, R> & G)
            {
            static_assert(std::is_copy_constructible<T>::value,"The object T must be copy constructible T(const T&) in order to use assignement operator= on the grid.");
            if ((&G) == this) { return(*this); }
            _destroyTree();
            _rangemin = G._rangemin;
            _rangemax = G._rangemax;
            _pcurrent = _copy(G._getRoot(),nullptr);
            _pcurrentpeek = (_pbox)_pcurrent;
            _callDtors = G._callDtors;
            return(*this);
            }


        /**
         * Assignment operator. Create a deep copy of a Grid_factor object. The source must have the
         * same template parameters D,T,R and not have special values (use
         * `Grid_factor::removeSpecialObjects()` to remove special object if needed prior to
         * converting). If the grid is not empty, it is first reset: all the objects inside are
         * destroyed and their dtors are invoqued if the callDtors flag is set.
         *
         * @param   G   The source Grid_factor object to copy.
         *
         * @return  The object for chaining.
         **/
        template<size_t NB_SPECIAL> Grid_basic<D, T, R> & operator=(const Grid_factor<D, T, NB_SPECIAL, R> & G);


        /**
         * Resets the grid to its initial state. Call the destructor of all the T objects if the flag
         * callDtors is set. When the method returns, there are no living T object inside the grid.
         **/
        void reset() { _destroyTree(); _createBaseNode(); }


        /**
         * Serializes the grid into an OArchive. If T implement a serialize method recognized by
         * OArchive, it is used for serialization otherwise OArchive uses its default serialization
         * method (which correspond to a basic memcpy() of the object memory).
         *
         * @param [in,out]  ar  The archive object to serialise the grid into.
         *
         * @sa  class OArchive, class IArchive
         **/
        void serialize(OArchive & ar) const
            {
            ar << "\nBegining of Grid_basic<" << D << " , [" << std::string(typeid(T).name()) << "] , " << R << ">\n";
            ar << "Version";    ar & ((uint64)1); ar.newline();
            ar << "Template D"; ar & ((uint64)D); ar.newline();
            ar << "Template R"; ar & ((uint64)R); ar.newline();
            ar << "object T";   ar & std::string(typeid(T).name()); ar.newline();
            ar << "sizeof(T)";  ar & ((uint64)sizeof(T)); ar.newline();
            ar << "call dtors"; ar & _callDtors; ar.newline();
            ar << "_rangemin";  ar & _rangemin; ar.newline();
            ar << "_rangemax";  ar & _rangemax; ar.newline();
            ar << "_minSpec";   ar & ((int64)0); ar.newline();
            ar << "_maxSpec";   ar & ((int64)-1); ar.newline();
            ar << "Grid tree\n";
            _serializeTree(ar, _getRoot());
            ar << "\nEnd of Grid_basic<" << D << " , [" << std::string(typeid(T).name()) << "] , " << R << ">\n";
            }


        /**
         * Deserializes the grid from an IArchive. If T has a constructor of the form T(IArchive &), it
         * is used for deserializing the T objects in the grid. Otherwise, if T implements one of the
         * serialize methods recognized by IArchive, the objects in the grid are first position/default
         * constructed and then deserialized using those methods. If no specific deserialization method
         * is found, IArchive falls back to its default derialization method.
         * 
         * If the grid is non-empty, it is first reset, possibly calling the ctor of the existing T
         * objects depending on the status of the callCtor flag.
         *
         * @param [in,out]  ar  The archive to deserialize the grid from.
         *
         * @sa  class OArchive, class IArchive
         **/
        void deserialize(IArchive & ar)
            {
            try
                {
                _destroyTree();
                uint64 ver;         ar & ver;      if (ver != 1) { MTOOLS_DEBUG("wrong version"); throw ""; }
                uint64 d;           ar & d;        if (d != D) { MTOOLS_DEBUG("wrong dimension"); throw ""; }
                uint64 r;           ar & r;        if (r != R) { MTOOLS_DEBUG("wrong R parameter"); throw ""; }
                std::string stype;  ar & stype;
                uint64 sizeofT;     ar & sizeofT;  if (sizeofT != sizeof(T)) { MTOOLS_DEBUG("wrong sizeof(T)"); throw ""; }
                ar & _callDtors;
                ar & _rangemin;
                ar & _rangemax;
                int64 minSpec; ar & minSpec;
                int64 maxSpec; ar & maxSpec;
                if (minSpec <= maxSpec) { MTOOLS_DEBUG("The file contain special objects hence must be opened using a Grid_factor instead of a Grid_basic object"); throw ""; }
                _pcurrent = _deserializeTree(ar, nullptr);
                _pcurrentpeek = (_pbox)_pcurrent;
                }
            catch(...)
                {
                _callDtors = false;
                _destroyTree();    // object are dumped into oblivion, may result in a memory leak.
                _callDtors = true;
                _createBaseNode();
                MTOOLS_DEBUG("Aborting deserialization of Grid_basic object");
                throw; // rethrow
                }
            }


        /**
         * Saves the grid into a file (using the archive format). The file is compressed if it ends by
         * the extension ".gz", ".gzip" or ".z".
         * 
         * Grid file for grid_Factor and Grid_basic are compatible so that the file can subsequently be
         * opened with a Grid_factor object (provided the template parameters T, R and D are the same
         * and T fullfills the requirement of Grid_factor).
         * 
         * The method simply call serialize() to create the archive file.
         *
         * @param   filename    The filename to save.
         *
         * @return  true on success, false on failure.
         *
         * @sa  load, serialize, deserialize, class OArchive, class IArchive
         **/
        bool save(const std::string & filename) const
            {
            try
                {
                OArchive ar(filename);
                ar & (*this); // use the serialize method.
                }
            catch (...) 
                {
                MTOOLS_DEBUG("Error saving Grid_basic object");
                return false;
                } // error
            return true; // ok
            }


        /**
         * Loads a grid from a file. Grid files are compatible between classes hence this method can
         * also load file created from a grid_factor class provided that the grid_factor object had no
         * special object at the time of saving the file.
         * 
         * If the grid is non-empty, it is first reset, possibly calling the dtors of T object depending
         * on the status of the callDtor flag.
         * 
         * The method simply call deserialize() to recreate the grid from the archive file.
         *
         * @param   filename    The filename to load.
         *
         * @return  true on success, false on failure [in this case, the lattice is reset].
         *
         * @sa  save, class OArchive, class IArchive
         **/
        bool load(const std::string & filename)
            {
            try
                {
                IArchive ar(filename);
                ar & (*this); // use the deserialize method.
                }
            catch (...) 
                {
                MTOOLS_DEBUG("Error loading Grid_basic object");
                _callDtors = false; _destroyTree(); _callDtors = true; _createBaseNode(); 
                return false; 
                } // error
            return true; // ok
            }


        /**
         * Return the range of elements accessed. The method returns maxpos<minpos if no element have
         * ever been accessed.
         *
         * @param [in,out]  minpos  a vector with the minimal coord. in each direction.
         * @param [in,out]  maxpos  a vector with the maximal coord. in each direction.
         **/
        inline void getPosRange(Pos & minpos, Pos & maxpos) const { minpos = _rangemin; maxpos = _rangemax;}


        /**
        * Return the spacial range of elements accessed in an iRect structure. The rectangle is empty
        * if no elements have ever been accessed. Specific to dimension D = 2.
        *
        * @return  an iRect containing the spacial range of element accessed.
        **/
        inline iRect getPosRangeiRect() const
            {
            static_assert(D == 2, "getRangeiRect() method can only be used when the dimension template parameter D is 2");
            return mtools::iRect(_rangemin.X(), _rangemax.X(), _rangemin.Y(), _rangemax.Y());
            }


        /**
         * Check if we should call the destructors of T objects when they are not needed anymore.
         *
         * @return  true if we call the dtors and false otherwise.
         **/
        bool callDtors() const { return _callDtors; }


        /**
        * Set whether we should, from now on, call the destructor of object when they are not needed
        * anymore.
        *
        * @param   callDtor    true to call the destructors.
        **/
        void callDtors(bool callDtor) { _callDtors = callDtor; }


        /**
        * Returns a string with some information concerning the object.
        *
        * @param   debug   Set this flag to true to enable the debug mode where the whole tree structure
        *                  of the lattice is written into the string [should not be used for large grids].
        *
        * @return  an info string.
        **/
        std::string toString(bool debug = false) const
            {
            std::string s;
            s += std::string("Grid_basic<") + mtools::toString(D) + " , " + typeid(T).name() + " , " + mtools::toString(R) + ">\n";
            s += std::string(" - Memory used : ") + mtools::toString((_poolLeaf.footprint() + _poolNode.footprint()) / (1024 * 1024)) + "MB\n";
            s += std::string(" - Range min = ") + _rangemin.toString(false) + "\n";
            s += std::string(" - Range max = ") + _rangemax.toString(false) + "\n";
            if (debug) { s += "\n" + _printTree(_getRoot(), ""); }
            return s;
            }


        /**
         * Sets the value at a given site. This method require T to be assignable via T.operator=. If the
         * value at the site does not exist prior to the call of the method, it is first created then the
         * assignement is performed.
         *
         * @param   pos The position of the site to access.
         * @param   val The value to set.
         **/
        inline void set(const Pos & pos, const T & val)
            {
            _get(pos) = val;
            }


        /**
         * Get a value at a given position. If the T object at that site does not exist, it is created.
         * (const version)
         *
         * @param   pos The position.
         *
         * @return  A const reference to the value.
         **/
        inline const T & get(const Pos & pos) const { return _get(pos); }


        /**
         * Get a value at a given position. If the T object at that site does not exist, it is created.
         *
         *
         * @param   pos The position.
         *
         * @return  A reference to the value.
         **/
        inline T & get(const Pos & pos) { return _get(pos); }


        /**
         * Get a value at a given position. If the T object at that site does not exist, it is created.
         * (const version)
         *
         * @param   pos The position.
         *
         * @return  A const reference to the value
         **/
        inline const T & operator[](const Pos & pos) const { return _get(pos); }


        /**
         * Get a value at a given position. If the T object at that site does not exist, it is created.
         *
         * @param   pos The position.
         *
         * @return  A reference to the value.
         **/
        inline T & operator[](const Pos & pos) { return _get(pos); }


        /**
         * Return a pointer to the object at a given position. If the value at the site was not yet
         * created, returns nullptr. This method does not create sites and is suited when drawing the
         * lattice using, for instance, the LatticeDrawer class.
         * 
         * This method is threadsafe : it is safe to call peek() while accessing the object via a get()
         * or a set() or one of their operator [], operator() equivalents. It may even be called while
         * the grid is being modified by assign/load operations. However, peek() should not be called
         * while another peek() is in progress. To perform multiple simulatneous peek, use the peek
         * version with hint below.
         * 
         * The implementation of this method is lock free hence there is zero penalty to peeking while
         * still reading/setting the object !
         *
         * @param   pos The position to peek.
         *
         * @return  nullptr if the value at that site was not yet created. A const pointer to it
         *          otherwise.
         **/
        inline const T * peek(const Pos & pos) const
            {
            _pbox c = _pcurrentpeek;
            if (c == nullptr) return nullptr;
            // check if we are at the right leaf
            if (c->isLeaf())
                {
                _pleaf p = (_pleaf)(c);
                if (p->isInBox(pos)) return(&(p->get(pos)));
                c = p->father;  
                if (c == nullptr) return nullptr;
                }
            // no we go up...
            _pnode q = (_pnode)(c);
            while(!q->isInBox(pos))
                {
                if (q->father == nullptr) { _pcurrentpeek = q; return nullptr; }
                q = (_pnode)q->father;
                }
            // and down...
            while (1)
                {
                _pbox b = q->getSubBox(pos);
                if (b == nullptr) { _pcurrentpeek = q; return nullptr; }
                if (b->isLeaf())  { _pcurrentpeek = b; return(&(((_pleaf)b)->get(pos))); }
                q = (_pnode)b;
                }
            }


        /**
         * Return a pointer to the object at a given position. If the value at the site was not yet
         * created, returns nullptr. This method does not create sites and is suited when drawing the
         * lattice using, for instance, the LatticeDrawer class.
         * 
         * This version is similar to the classical peek version but use an additonal hint parameter
         * which enable several simultaneous peek. The hint parameter must be set to nullptr for the
         * first call and then, the value modified after a call to peek must forwarded to the next
         * peek().
         * 
         *  Using this version of peek is perfectly threadsafe. The object can be simultaneously
         *  modified, peeked (with or without hint) or even assigned/loaded !
         * 
         * The implementation of this method is lock free hence there is zero penalty to peeking while
         * still reading/setting the object !
         *
         * @param   pos             The position to peek.
         * @param [in,out]  hint    the hint pointer. Must be set to nullptr for the first call and then
         *                          the same variable must be forwarded on each subsequent call.
         *
         * @return  nullptr if the value at that site was not yet created. A const pointer to it
         *          otherwise.
         **/
        inline const T * peek(const Pos & pos, void* & hint) const
            {
            if (hint == nullptr) { hint = _pcurrentpeek; }
            _pbox c = (_pbox)hint;
            if (c == nullptr) return nullptr;
            // check if we are at the right leaf
            if (c->isLeaf())
                {
                _pleaf p = (_pleaf)(c);
                if (p->isInBox(pos)) return(&(p->get(pos)));
                c = p->father;
                if (c == nullptr) return nullptr;
                }
            // no we go up...
            _pnode q = (_pnode)(c);
            while (!q->isInBox(pos))
                {
                if (q->father == nullptr) { hint = q; return nullptr; }
                q = (_pnode)q->father;
                }
            // and down...
            while (1)
                {
                _pbox b = q->getSubBox(pos);
                if (b == nullptr) { hint = q; return nullptr; }
                if (b->isLeaf()) { hint = b; return(&(((_pleaf)b)->get(pos))); }
                q = (_pnode)b;
                }
            }



        /**
         * get a value at a given position. Dimension 1 specialization. (const version)
         * This creates the object if it does not exist yet.
         **/
        inline const T & operator()(int64 x) const { static_assert(D == 1, "template parameter D must be 1"); return _get(Pos(x)); }


        /**
         * get a value at a given position. Dimension 2 specialization. (const version)
         * This creates the object if it does not exist yet.
         **/
        inline const T & operator()(int64 x, int64 y) const { static_assert(D == 2, "template parameter D must be 2"); return _get(Pos(x, y)); }


        /**
         * get a value at a given position. Dimension 3 specialization. (const version)
         * This creates the object if it does not exist yet.
         **/
        inline const T & operator()(int64 x, int64 y, int64 z) const { static_assert(D == 3, "template parameter D must be 3"); return _get(Pos(x, y, z)); }


        /**
         * get a value at a given position. Dimension 1 specialization.
         * This creates the object if it does not exist yet.
         **/
        inline T & operator()(int64 x) { static_assert(D == 1, "template parameter D must be 1"); return _get(Pos(x)); }


        /**
         * get a value at a given position. Dimension 2 specialization.
         * This creates the object if it does not exist yet.
         **/
        inline T & operator()(int64 x, int64 y) { static_assert(D == 2, "template parameter D must be 2"); return _get(Pos(x, y)); }


        /**
         * get a value at a given position. Dimension 3 specialization.
         * This creates the object if it does not exist yet.
         **/
        inline T & operator()(int64 x, int64 y, int64 z)  { static_assert(D == 3, "template parameter D must be 3"); return _get(Pos(x, y, z)); }


        /**
         * peek at a value at a given position. Dimension 1 specialization.
         * 
         * This method is threasafe wrt get/set : see peek() documentation for details.
         * 
         * Returns nullptr if the object at this position is not created.
         **/
        inline const T * peek(int64 x) const { static_assert(D == 1, "template parameter D must be 1"); return peek(Pos(x)); }


        /**
         * peek at a value at a given position. Dimension 2 specialization.
         * 
         * This method is threasafe wrt get/set : see peek() documentation for details.
         * 
         * Returns nullptr if the object at this position is not created.
         **/
        inline const T * peek(int64 x, int64 y) const { static_assert(D == 2, "template parameter D must be 2"); return peek(Pos(x, y)); }


        /**
         * peek at a value at a given position. Dimension 3 specialization.
         *
         * This method is threasafe wrt get/set : see peek() documentation for details.
         *
         * Returns null if the object at this position is not created.
         **/
        inline const T * peek(int64 x, int64 y, int64 z) const { static_assert(D == 3, "template parameter D must be 3"); return peek(Pos(x, y, z)); }



        /***************************************************************
        * Private section
        ***************************************************************/


    private:


        static_assert(D > 0, "template parameter D (dimension) must be non-zero");
        static_assert(R > 0, "template parameter R (radius) must be non-zero");
        static_assert(std::is_constructible<T>::value || std::is_constructible<T, Pos>::value, "The object T must either be default constructible T() or constructible with T(const Pos &)");


        /* get sub method
         * the tree is consistent at all time */
        inline T & _get(const Pos & pos) const
            {
            MTOOLS_ASSERT(_pcurrent != (_pbox)nullptr);
            _pbox c = _pcurrent;
            _updaterange(pos);
            if (c->isLeaf())
                {
                if (((_pleaf)c)->isInBox(pos)) { return(((_pleaf)c)->get(pos)); }
                MTOOLS_ASSERT(c->father != nullptr); // a leaf must always have a father
                c = c->father;
                }
            // going up...
            _pnode q = (_pnode)c;
            while (!q->isInBox(pos))
                {
                if (q->father == nullptr) { q->father = _allocateNode(q); }
                q = (_pnode)q->father;
                }
            // ...and down
            while(1)
                {
                _pbox & b = q->getSubBox(pos);
                if (b == nullptr)
                    {
                    if (q->rad == R)
                        {
                        b = _allocateLeaf(q, q->subBoxCenter(pos));
                        T & result = ((_pleaf)b)->get(pos);
                        _pcurrent = b;
                        return(result);
                        }
                    q = _allocateNode(q, q->subBoxCenter(pos), nullptr);
                    b = q;
                    }
                else
                    {
                    if (b->isLeaf()) { T & result = ((_pleaf)b)->get(pos); _pcurrent = b; return(result); }
                    q = (_pnode)b;
                    }
                }
            }


        /* update _rangemin and _rangemax */
        inline void _updaterange(const Pos & pos) const
            {
            for (size_t i = 0; i < D; i++) {if (pos[i] < _rangemin[i]) { _rangemin[i] = pos[i]; } else if (pos[i] > _rangemax[i]) { _rangemax[i] = pos[i]; } }
            }


        /* get the root of the tree */
        inline _pbox _getRoot() const
            {
            if (_pcurrent == ((_pbox)nullptr)) return nullptr;
            _pbox p = _pcurrent;
            while (p->father != nullptr) { p = p->father; }
            return p;
            }


        /* print the tree, for debug purpose only */
        std::string _printTree(_pbox p, std::string tab) const
            {
            if (p == nullptr) { return(tab + "NULLPTR\n"); }
            if (p->isLeaf())
                {
                std::string r = tab + " Leaf: center = " + p->center.toString(false) + "\n";
                return r;
                }
            std::string r = tab + " Node: center = " + p->center.toString(false) + "  Radius = " + mtools::toString(p->rad) + "\n";
            tab += "    |";
            for (size_t i = 0; i < metaprog::power<3, D>::value; ++i) { r += _printTree(((_pnode)p)->tab[i], tab); }
            return r;
            }


        /* recursive method for serialization of the tree */
        template<typename ARCHIVE> void _serializeTree(ARCHIVE & ar, _pbox p) const
            {
            if (p == nullptr) { ar & ((char)'V'); return; } // void pointer
            if (p->isLeaf())
                {
                ar & ((char)'L');
                ar & p->center;
                ar & p->rad;
                for (size_t i = 0; i < metaprog::power<(2 * R + 1), D>::value; ++i)  { ar & (((_pleaf)p)->data[i]); }
                return;
                }
            ar & ((char)'N');
            ar & p->center;
            ar & p->rad;
            for (size_t i = 0; i < metaprog::power<3, D>::value; ++i)  { _serializeTree(ar,((_pnode)p)->tab[i]); }
            return;
            }


        /* reconstruction of the object of a leaf from the constructor T(IArchive &)*/
        inline void _reconstructLeaf(IArchive & ar, T * data, Pos center, metaprog::dummy<true> dum)
            {
            for (size_t i = 0; i < metaprog::power<(2 * R + 1), D>::value; ++i)  { new(data + i) T(ar); }
            }


        /* reconstruction of the object of a leaf when no constructor from archive */
        inline void _reconstructLeaf(IArchive & ar, T * data, Pos center, metaprog::dummy<false> dum)
            {
            _createDataLeaf(data, center, metaprog::dummy<std::is_constructible<T, Pos>::value>());          // first call the default or the positionnal ctor for every object
            for (size_t i = 0; i < metaprog::power<(2 * R + 1), D>::value; ++i)  { ar &  (data[i]); }        // and then deserializes
            }


        /* recursive method for deserialization of the tree */
        template<typename ARCHIVE> _pbox _deserializeTree(ARCHIVE & ar, _pbox father)
            {
            char c;
            ar & c;
            if (c == 'V') return nullptr;
            if (c == 'L')
                {
                MTOOLS_ASSERT(father->rad == R);
                _pleaf p = _poolLeaf.allocate();
                ar & p->center;
                ar & p->rad;
                MTOOLS_ASSERT(p->rad == 1);
                p->father = father;
                _reconstructLeaf(ar, p->data, p->center, metaprog::dummy< std::is_constructible<T, IArchive>::value>());
                return p;
                }
            if (c == 'N')
                {
                _pnode p = _poolNode.allocate();
                ar & p->center;
                ar & p->rad;
                p->father = father;
                for (size_t i = 0; i < metaprog::power<3, D>::value; ++i) { p->tab[i] = _deserializeTree(ar, p); }
                return p;
                }
            MTOOLS_DEBUG(std::string("Unknown tag [") + std::string(1, c) + "]");
            throw "";
            }


        /* Release all the allocated  memory and reset the tree */
        void _destroyTree()
            {
            _pcurrentpeek = nullptr;
            _pcurrent = nullptr;
            _rangemin.clear(std::numeric_limits<int64>::max());
            _rangemax.clear(std::numeric_limits<int64>::min());
            _poolNode.destroyAll();
            if (_callDtors) { _poolLeaf.destroyAll(); } else { _poolLeaf.deallocateAll(); }
            return;
            }


        /* make a copy of the subtree starting from pg
        and set the father of the root to pere. Return the root
        of the sub tree created. */
        _pbox _copy(_pbox pg, _pbox pere)
            {
            if (pg == nullptr) { return nullptr; }
            if (pg->isLeaf())
                {
                _pleaf p = _poolLeaf.allocate();
                p->center = pg->center;
                p->rad = pg->rad;
                p->father = pere;
                for (size_t i = 0; i < metaprog::power<(2 * R + 1), D>::value; ++i) { new(p->data + i) T(((_pleaf)pg)->data[i]); }
                return p;
                }
            _pnode p = _poolNode.allocate();
            p->center = pg->center;
            p->rad = pg->rad;
            p->father = pere;
            for (size_t i = 0; i < metaprog::power<3, D>::value; ++i) { p->tab[i] = _copy(((_pnode)pg)->tab[i], p); }
            return p;
           }


        /* create the data for an elementary sub box using the T(const Pos & v) version */
        inline void _createDataLeaf(T * data, Pos pos, metaprog::dummy<true> dum) const
            {
            Pos center = pos;
            for (size_t i = 0; i < D; ++i) { pos[i] -= R; }
            for (size_t x = 0; x < metaprog::power<(2 * R + 1), D>::value; ++x)
                {
                new(data + x) T(pos);
                for (size_t i = 0; i < D; ++i)
                    {
                    if (pos[i] < (center[i] + (int64)R)) { pos[i]++;  break; }
                    pos[i] -= (2 * R);
                    }
                }
            }


        /* create the data for an elementary sub box using the default T() version */
        inline void _createDataLeaf(T * data, Pos pos, metaprog::dummy<false> dum) const { for (size_t i = 0; i < metaprog::power<(2 * R + 1), D>::value; ++i) { new(data + i) T(); } }


        /* Allocate a leaf, call constructor from above with default initialization (ie either T() or T(Pos) */
        inline _pleaf _allocateLeaf(_pbox above, const Pos & centerpos) const
            {
            _pleaf p = _poolLeaf.allocate();
            _createDataLeaf(p->data, centerpos, metaprog::dummy<std::is_constructible<T,Pos>::value>());
            p->center = centerpos;
            p->rad = 1;
            p->father = above;
            return p;
            }


        /* create the base node which centers the tree around the origin */
        inline void _createBaseNode()
            {
            MTOOLS_ASSERT(_pcurrent == (_pbox)nullptr);   // should only be called when the tree dos not exist.
            _pnode p = _poolNode.allocate();
            for (size_t i = 0; i < metaprog::power<3, D>::value; ++i) { p->tab[i] = nullptr; }
            p->center = Pos(0);
            p->rad = R;
            p->father = nullptr;
            _pcurrent = p;
            _pcurrentpeek = p;
            return;
            }


        /* Allocate a node, call constructor from above */
        inline _pnode _allocateNode(_pbox above, const Pos & centerpos, _pbox fill) const
            {
            _pnode p = _poolNode.allocate();
            for (size_t i = 0; i < metaprog::power<3, D>::value; ++i) { p->tab[i] = fill; }
            p->center = centerpos;
            p->rad = (above->rad - 1)/3;
            p->father = above;
            return p;
            }


        /* Allocate a node, call constructor from below */
        inline _pnode _allocateNode(_pbox below) const
            {
            _pnode p = _poolNode.allocate();
            for (size_t i = 0; i < metaprog::power<3, D>::value; ++i) { p->tab[i] = nullptr; }
            p->tab[(metaprog::power<3, D>::value - 1) / 2] = below;
            p->center = below->center;
            p->rad = ((below->rad == 1) ? R : (below->rad * 3 + 1));
            p->father = nullptr;
            return p;
            }


        mutable std::atomic<_pbox> _pcurrent;        // pointer to the current box
        mutable std::atomic<_pbox> _pcurrentpeek;    // pointer to the current box for peek operations
        mutable Pos   _rangemin;        // the minimal range
        mutable Pos   _rangemax;        // the maximal range
        bool _callDtors;                // should we call the destructors

        mutable SingleAllocator<internals_grid::_leaf<D, T, R>,200>  _poolLeaf;       // the two memory pools
        mutable SingleAllocator<internals_grid::_node<D, T, R>,200>  _poolNode;       //

    };


}


/* now we can include grid_factor */
#include "grid_factor.hpp"


namespace mtools
{


    template<size_t D, typename T, size_t R> template<size_t NB_SPECIAL> Grid_basic<D, T, R> & Grid_basic<D, T, R>::operator=(const Grid_factor<D, T, NB_SPECIAL, R> & G)
        {
        static_assert(std::is_copy_constructible<T>::value, "The object T must be copy constructible T(const T&) in order to use assignement operator= on the grid.");
        MTOOLS_INSURE(G._specialRange() <= 0); // there must not be any special objects. 
        _destroyTree();
        _rangemin = G._rangemin;
        _rangemax = G._rangemax;
        _pcurrent = _copy(G._getRoot(), nullptr);
        _pcurrentpeek = (_pbox)_pcurrent;
        _callDtors = G._callDtors;
        return(*this);
        }



}

/* end of file */




