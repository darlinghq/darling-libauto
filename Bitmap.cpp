/*
 * Copyright (c) 2011 Apple Inc. All rights reserved.
 *
 * @APPLE_APACHE_LICENSE_HEADER_START@
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * @APPLE_APACHE_LICENSE_HEADER_END@
 */
/*
    Bitmap.cpp
    High Performance Bitmap
    Copyright (c) 2004-2011 Apple Inc. All rights reserved.
 */

#include "Definitions.h"
#include "Bitmap.h"


namespace Auto {

    //----- Bitmap -----//

    //
    // Bitmaps (bit vectors) are a fast and lightweight means of showing set
    // representation.  A single bit is used to indicate membership in the set; 1
    // indicating membership and 0 indicating exclusion.  In this representation we use
    // an array of unsigned long words. Each word represents a unit of bits_per_word members
    // numbered from the least significant bit (endian representation is not
    // important.)  Thus for any member k its word index would be equal to (k / bits_per_word) or
    // (k >> ilog2(bits_per_word)) and its bit position would be equal to (k % bits_per_word).
    //
    

    //
    // set_bits_large
    //
    // Set n bits in the bit map to 1 spanning more than one word.
    // Assumes that range spans more than one word.
    //
    void Bitmap::set_bits_large(const usword_t bp, const usword_t n) const {
        usword_t *cursor = bp_cursor(bp);                   // address of first word
        const usword_t sh = shift(bp);                      // bit shift
        
        // set first word bits
        *cursor++ |= (all_ones << sh);
        
        // calculate overflow
        signed spill = sh + n - bits_per_word;
        
        // fill in full words
        for ( ; spill >= (sword_t)bits_per_word; spill -= bits_per_word) *cursor++ = all_ones;
        
        // remaining bits
        if (spill > 0) *cursor |= (all_ones >> (bits_per_word - spill));
    }
        
    
    //
    // clear_bits_large
    //
    // Set n bits in the bit map to 0 spanning more than one word.
    // Assumes that range spans more than one word.
    //
    void Bitmap::clear_bits_large(const usword_t bp, const usword_t n) const {
        usword_t *cursor = bp_cursor(bp);                   // address of first word
        const usword_t sh = shift(bp);                      // bit shift
        
        // set first word bits
        *cursor++ &= ~(all_ones << sh);
        
        // calculate overflow
        signed spill = sh + n - bits_per_word;
        
        // fill in full words
        for ( ; spill >= (sword_t)bits_per_word; spill -= bits_per_word) *cursor++ = all_zeros;
        
        // remaining bits
        if (spill > 0) *cursor &= ~(all_ones >> (bits_per_word - spill));
    }        
    
    
    bool Bitmap::bits_are_clear_large(const usword_t bp, const usword_t n) const {
        usword_t *cursor = bp_cursor(bp);                   // address of first word
        const usword_t sh = shift(bp);                      // bit shift
        
        // check first word bits
        if (*cursor++ & (all_ones << sh)) return false;
        
        // calculate overflow
        signed spill = sh + n - bits_per_word;
        
        // fill in full words
        for ( ; spill >= (sword_t)bits_per_word; spill -= bits_per_word)
            if (*cursor++) return false;
        
        // check remaining bits
        if (spill > 0) {
            if (*cursor & (all_ones >> (bits_per_word - spill)))
                return false;
        }
        
        return true;
    }        
    
    
    //
    // count_set
    //
    // Returns the number of bits set (one) in the bit map using
    // a standard bit population algoirithm (ref. Hacker's Delight.)
    // in 64 bit word treat bits array as 32 bit array.
    //
    usword_t Bitmap::count_set() const {
        register const unsigned fives = 0x55555555;
        register const unsigned threes = 0x33333333;
        register const unsigned nibbles = 0x0f0f0f0f;
        register const unsigned bytes = 0x00ff00ff;
        register const unsigned shorts = 0x0000ffff;
        register usword_t   count = 0;
        register usword_t   size = this->size() >> sizeof(unsigned);
        register unsigned   *bits = (unsigned *)address();
        
         for (usword_t i = 0; i < size; i += 31)  {
            usword_t lim = min(size, i + 31);
            usword_t sum8 = 0;
            
            for (usword_t j = i; j < lim; j++) {
                usword_t x = bits[j];
                x = x - ((x >> 1) & fives);
                x = (x & threes) + ((x >> 2) & threes);
                x = (x + (x >> 4)) & nibbles;
                sum8 = sum8 + x;
            }
            
            usword_t y = (sum8 & bytes) + ((sum8 >> 8) & bytes);
            y = (y & shorts) + (y >> 16);
            count += y;
        }
        
        return count;
    }

    
    //
    // previous_set
    //
    // Return the bit postion of the 1 that comes at or prior to the bit position 'bp'.
    //
    usword_t Bitmap::previous_set(const usword_t bp) const {
        usword_t *cursor = bp_cursor(bp);                   // address of bit map data
        usword_t *first = bp_cursor(0);                     // first address
        usword_t sh = shift(bp);                            // bit shift
        usword_t word = 0;                                  // bit map word
        
        // read first word and eliminate following 1s if not first position
        if (sh) word = *cursor & mask(sh);

        // skip backward looking for set bit
        if (is_all_zeros(word)) {
            // skip through zeroes quickly
            cursor = skip_backward_all_zeros(cursor - 1, first);
            
            // get word if at end make sure 
            if ( cursor >= first)  word = *cursor;
            else                   return not_found;
        }
        
        // return bit position of 1
        return cursor_bp(cursor) + ilog2(word);
    }
    
    
    //
    // next_set
    //
    // Return the bit postion of the 1 that comes at or after to the bit position 'bp'.
    //
    usword_t Bitmap::next_set(const usword_t bp) const {
        usword_t *cursor = bp_cursor(bp);                   // address of bit map data
        usword_t *end = end_cursor();                       // end address
        usword_t sh = shift(bp);                            // bit shift
        
        // may be at the last word
        if (cursor >= end) return not_found;
        
        // read first bit map word
        usword_t word = *cursor;                            

        // eliminate prior 1s if not first position
        if (sh) word &= ~mask(sh);
        
        // do we need to skip some words
        if (is_all_zeros(word)) {
            // skip through zeroes quickly
            cursor = skip_all_zeros(cursor + 1, end);
            
            // get next word
            word = cursor < end ? *cursor : all_ones;
        }
        
        // return bit position of 1 (not_found if not found)
        return cursor < end ? cursor_bp(cursor) + count_trailing_zeros(word) : not_found;
    }
            
    //
    // fetch_clear_bits_atomic
    //
    // fetch and clear multiple bits at a time
    // bp is the index of the first bit to fetch, max is the first index *not* to be fetched
    // returns a word containing the fetched bits, and by reference in count the number of valid bits in the return
    // returned bits start at bit 0, and bits higher than count are zeroed
    //
    usword_t Bitmap::AtomicCursor::fetch_clear_bits_atomic(const usword_t bp, const usword_t max, usword_t &count) {            
        usword_t result_shift = bp & mask(bits_per_word_log2);
        count = bits_per_word - result_shift;
        if (count > max - bp)
            count = max - bp;
        usword_t result_mask = mask(count);
        usword_t result = _bitmap.atomic_and_return_orig(~(result_mask << result_shift), _bitmap.bp_cursor(bp));
        result = (result >> result_shift) & result_mask;
        return result;
    }
    
    //
    // next_set_bit
    //
    // Returns the index of a set bit from the bitmap range. If no bits are set, returns -1.
    // The returned value is the offset from the start index passed to the constructor.
    // So if 5 is passed as the start bit to the constructor and bit 8 is the first set bit in the bitmap,
    // the first callto next_set_bit() will return 3. (This is done because the cursor is used
    // to enumerate subzone quanta, which are zero based.)
    //
    usword_t Bitmap::AtomicCursor::next_set_bit() {
        usword_t result;
        
        // May still have some valid bits from the last time. If they are zero then adjust index.
        if (_copied_bits == 0) {
            _index += _valid_bits;
        }
        
        // if there are no nonzero _copied_bits, this loop scans a word at a time until some nonzero bits are found
        if (_copied_bits == 0 && _index < _max_index) {
            ASSERTION((_index % bits_per_word) == 0);
            usword_t *start = _bitmap.bp_cursor(_index);
            usword_t *end = _bitmap.bp_cursor(_max_index);
            usword_t *cursor = _bitmap.skip_all_zeros(start, end);
            _index = _bitmap.cursor_bp(cursor);
            if (_index < _max_index) {
                // at this point we fetch either some nonzero bits or a partial word at the end of the range
                _copied_bits = fetch_clear_bits_atomic(_index, _max_index, _valid_bits);
            }
        }
        
        // there are either nonzero bits in _copied_bits or we are done
        if (_copied_bits != 0) {
            usword_t shift = count_trailing_zeros(_copied_bits);
            result = _index + shift - _offset;
            // Need to shift _copied_bits by (shift+1). But if shift+1 == bits_per_word then the bitwise shift is undefined.
            // So just shift by shift and again by 1 rather than introduce a test/branch.
            _copied_bits >>= shift;
            _copied_bits >>= 1;
            shift++;
            _index += shift;
            _valid_bits -= shift;
        } else {
            // no nonzero bits, so we are done
            result = (usword_t)-1;
            _index = _max_index;
        }
        return result;
    }
};

