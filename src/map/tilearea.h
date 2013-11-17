/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file map/tilearea.h Handling of tile areas. */

#ifndef MAP_TILEAREA_H
#define MAP_TILEAREA_H

#include "coord.h"

/** Represents the covered area of e.g. a rail station */
struct TileArea {
	TileIndex tile; ///< The base tile of the area
	uint16 w;       ///< The width of the area
	uint16 h;       ///< The height of the area

	/** Just construct this tile area */
	TileArea() {}

	/**
	 * Construct this tile area with some set values
	 * @param tile the base tile
	 * @param w the width
	 * @param h the height
	 */
	TileArea(TileIndex tile, uint8 w, uint8 h) : tile(tile), w(w), h(h) {}

	TileArea(TileIndex start, TileIndex end);

	void Set(uint x0, uint y0, uint x1, uint y1);

	void Add(TileIndex to_add);

	void Add(const TileArea &to_add);

	/**
	 * Clears the 'tile area', i.e. make the tile invalid.
	 */
	void Clear()
	{
		this->tile = INVALID_TILE;
		this->w    = 0;
		this->h    = 0;
	}

	bool Intersects(const TileArea &ta) const;

	bool Contains(TileIndex tile) const;

	void ClampToMap();

	/**
	 * Get the center tile.
	 * @return The tile at the center, or just north of it.
	 */
	TileIndex GetCenterTile() const
	{
		return TILE_ADDXY(this->tile, this->w / 2, this->h / 2);
	}

	/**
	 * Get the tile in the area closest to a given tile.
	 * @param t The reference tile.
	 * @return The closest tile, or INVALID_TILE if the area is empty.
	 */
	TileIndex get_closest_tile(TileIndex t) const
	{
		if (this->tile == INVALID_TILE) return INVALID_TILE;

		/* clamp x coordinate */
		uint x = TileX(this->tile);
		x = ClampU(TileX(t), x, x + this->w - 1);

		/* clamp y coordinate */
		uint y = TileY(this->tile);
		y = ClampU(TileY(t), y, y + this->h - 1);

		/* return the tile of our target coordinates */
		return TileXY(x, y);
	}
};

/** Base class for tile iterators. */
class TileIterator {
protected:
	TileIndex tile; ///< The current tile we are at.

	/**
	 * Initialise the iterator starting at this tile.
	 * @param tile The tile we start iterating from.
	 */
	TileIterator(TileIndex tile) : tile(tile)
	{
	}

	/**
	 * Compute the next tile.
	 */
	virtual void Next() = 0;

public:
	/** Some compilers really like this. */
	virtual ~TileIterator()
	{
	}

	/**
	 * Get the tile we are currently at.
	 * @return The tile we are at, or INVALID_TILE when we're done.
	 */
	inline operator TileIndex () const
	{
		return this->tile;
	}

	/**
	 * Move ourselves to the next tile in the rectangle on the map.
	 */
	TileIterator& operator ++()
	{
		assert(this->tile != INVALID_TILE);
		this->Next();
		return *this;
	}

	/**
	 * Allocate a new iterator that is a copy of this one.
	 */
	virtual TileIterator *Clone() const = 0;
};

/** Iterator to iterate over a tile area (rectangle) of the map. */
class OrthogonalTileIterator : public TileIterator {
private:
	const uint w;       ///< The width of the iterated area.
	const uint rowdiff; ///< The amount to add when switching rows
	uint x;             ///< The current 'x' position in the rectangle.
	uint y;             ///< The current 'y' position in the rectangle.

protected:
	/**
	 * Move ourselves to the next tile in the rectangle on the map.
	 */
	inline void Next() OVERRIDE
	{
		assert(this->tile != INVALID_TILE);

		if (--this->x > 0) {
			this->tile++;
		} else if (--this->y > 0) {
			this->x = this->w;
			this->tile += this->rowdiff;
		} else {
			this->tile = INVALID_TILE;
		}
	}

public:
	/**
	 * Construct the iterator.
	 * @param ta Area, i.e. begin point and width/height of to-be-iterated area.
	 */
	OrthogonalTileIterator(const TileArea &ta)
		: TileIterator(ta.w == 0 || ta.h == 0 ? INVALID_TILE : ta.tile),
			w(ta.w), rowdiff(TileDiffXY(1, 1) - ta.w), x(ta.w), y(ta.h)
	{
	}

	OrthogonalTileIterator *Clone() const OVERRIDE
	{
		return new OrthogonalTileIterator(*this);
	}
};

/** Iterator to iterate over a diagonal area of the map. */
class DiagonalTileIterator : public TileIterator {
private:
	uint x;   ///< x coordinate of the current tile
	uint y;   ///< y coordinate of the current tile
	bool odd; ///< Whether this is an "odd" area
	int8 s1;  ///< Advancing a tile adds (s1,s1)
	int  s2x; ///< Advancing a row adds (s2x,s2y)
	int  s2y; ///< Advancing a row adds (s2x,s2y)
	uint w;   ///< The width of the main rectangle side
	uint n;   ///< The number of tiles left on the current row
	uint m;   ///< The number of rows left

protected:
	void Next() OVERRIDE;

public:
	DiagonalTileIterator(TileIndex begin, TileIndex end);

	DiagonalTileIterator *Clone() const OVERRIDE
	{
		return new DiagonalTileIterator(*this);
	}
};

/**
 * A loop which iterates over the tiles of a TileArea.
 * @param var The name of the variable which contains the current tile.
 *            This variable will be allocated in this \c for of this loop.
 * @param ta  The tile area to search over.
 */
#define TILE_AREA_LOOP(var, ta) for (OrthogonalTileIterator var(ta); var != INVALID_TILE; ++var)

#endif /* MAP_TILEAREA_H */