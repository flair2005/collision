#pragma once

#include "stdafx.h"
#include "maps.cpp"


template<typename key_t, typename index_t>
class SparseGrid {
    /*
    this class groups a set of keys using an indirect sort
    it allows for querying of unique keys,
    and getting all permutation indices for a given key

    DenseGrid class would have ndarray instead of HashMap to look up buckets
    */

public:
    typedef key_t	            key_t;
    typedef index_t		        index_t;

public:
    SparseGrid<key_t, index_t>& self;
    const ndarray<key_t>        keys;
    const index_t               n_keys;

    const ndarray<index_t>      permutation; // index array mapping the keys to lexographically sorted order
    const ndarray<index_t>      pivots;	  // boundaries between buckets of keys as viewed under permutation
    const index_t               n_groups;    // number of unique keys

    const HashMap<key_t, index_t, index_t> group_from_key; // maps key to group index

    auto get_permutation()  const { return permutation; }
    auto get_pivots()       const { return pivots; }

public:
    // constructor
    explicit SparseGrid(ndarray<key_t> keys) :
        self        (*this),
        keys        (keys),
        n_keys      (keys.size()),
        permutation (init_permutation()),
        pivots      (init_pivots()),
        n_groups    (pivots.size() - 1),
        group_from_key(       // create a map to invert the key_from_group function
            boost::combine(
                unique_keys(),
                irange(0, n_groups)
            )
        )
    {
    }

    // construct using permutation initial guess
    explicit SparseGrid(ndarray<key_t> keys, ndarray<index_t> permutation) :
        self        (*this),
        keys        (keys),
        n_keys      (keys.size()),
        permutation (init_permutation(permutation)),
        pivots      (init_pivots()),
        n_groups    (pivots.size() - 1),
        group_from_key(       // create a map to invert the key_from_group function
            boost::combine(
                unique_keys(),
                irange(0, n_groups)
            )
        )
    {
    }

private:
    // find the permutation which puts the keys in sorted order
    auto init_permutation() const {
        return self.init_permutation(irange(0, self.n_keys));
    }
    template<typename range_t>
    auto init_permutation(const range_t initial_permutation) const {
        ndarray<index_t> _permutation( list_of(self.n_keys) );
        boost::copy(initial_permutation, _permutation.begin());
        auto _keys = self.keys.range();
        // add indirection to sort
        auto lex = [&](index_t l, index_t r) {return _keys[r] > _keys[l];};
        // wow, casting permutation to raw range yield factor 3 performance in sorted case
        boost::sort(_permutation.range(), lex);
        return _permutation;
    }
    //divide the sorted keys into groups, containing keys of identical value
    ndarray<index_t> init_pivots() const {
        auto _keys = self.keys.range();
        auto start = self.permutation
            | transformed(      [&](auto i)         {return _keys[i];})
            | indexed(0)
            | adjacent_filtered([](auto a, auto b)  {return a.value() != b.value();})
            | transformed(      [](auto i)          {return (index_t)i.index();});
        std::vector<index_t> cap = {self.n_keys};
        return ndarray_from_iterable(boost::join(start, cap));
    }

    // get n-th unique key
    inline key_t key_from_group(index_t g) const {
        return self.keys[self.permutation[self.pivots[g]]];
    }
public:
    // range over each unique key in the grid, in sorted order
    auto unique_keys() const {
        return irange(0, self.n_groups)
            | transformed([&](auto g){return self.key_from_group(g);});
    }
    // return a range of the permutation indices within a key-group
    inline auto indices_from_key(const key_t key) const {
        const index_t g = self.group_from_key[key];
        return  ((g == -1) ? irange(0, 0) : irange(self.pivots[g], self.pivots[g + 1]))
                    | transformed([&](index_t i) {return self.permutation[i];});
    }
    // extra branch isnt required if the key is known to be valid
    inline auto indices_from_existing_key(const key_t key) const {
        const index_t g = self.group_from_key[key];
        return irange(self.pivots[g], self.pivots[g + 1])
                    | transformed([&](index_t i) {return self.permutation[i];});
    }

};
