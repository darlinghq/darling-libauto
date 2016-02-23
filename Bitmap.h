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
    Bitmap.h
    High Performance Bitmap
    Copyright (c) 2004-2011 Apple Inc. All rights reserved.
 */

#pragma once
#ifndef __AUTO_BITMAP__
#define __AUTO_BITMAP__

#include "Definitions.h"
#include "Range.h"


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
    
    class Bitmap : public Range {

     private:         

        //
        // index
        //
        // Returns the word index of a specified bit.
        //
        static inline const usword_t index(const usword_t bp) { return bp >> bits_per_word_log2; }
        
        
        //
        // shift
        //
        // Returns the shift of a specified bit.
        //
        static inline const usword_t shift(const usword_t bp) { return bp & bits_mask; }
        
        
        //
        // cursor_bp
        //
        // Returns the bit position of the specified cursor.
        //
        inline usword_t cursor_bp(const usword_t *cursor) const { return ((uintptr_t)cursor - (uintptr_t)address()) << bits_per_byte_log2; }
        
        
        //
        // end_cursor
        //
        // Returns the end of the bits as a word address.
        //
        inline usword_t *end_cursor() const { return (usword_t *)Range::end(); }
        
        
     public:
     
        //
        // Constructors
        //
        Bitmap() {}
        Bitmap(usword_t n, void *bits) {
            // keep aligned for vector operations
            ASSERTION(!((uintptr_t)bits & mask(bytes_per_quad_log2)) && !(bytes_needed(n) & mask(bytes_per_quad_log2)));
            set_range(bits, bytes_needed(n));
        }
        
        
        //
        // bp_cursor
        //
        // Returns the word address containing a specified bit.
        //
        inline usword_t *bp_cursor(const usword_t bp) const {
            return (usword_t *)displace(address(), (bp >> (bits_per_word_log2 - bytes_per_word_log2)) & ~mask(bytes_per_word_log2));
        }
        
        
        //
        // size_in_bits
        //
        // Return the number of bits in the bitmap.
        //
        inline usword_t size_in_bits() const { return size() << bits_per_byte_log2; }
        
        
        //
        // initialize
        //
        // Set up the bit map for use.
        //
        inline void initialize(usword_t n, void *bits) { set_range(bits, bytes_needed(n)); }
        

        //
        // bytes_needed
        //
        // Returns the number of bytes needed to represent 'n' bits.
        //
        static inline const usword_t bytes_needed(usword_t n) { return partition2(n, bits_per_word_log2) << bytes_per_word_log2; }
        
        
        //
        // bit
        //
        // Returns the state of a bit in the bit map.
        //
        inline usword_t bit(const usword_t bp) const { return (*bp_cursor(bp) >> shift(bp)) & 1L; }

        
        //
        // set_bit
        //
        // Set a bit in the bit map to 1.
        //
        inline void set_bit(const usword_t bp) const {
            usword_t *cursor = bp_cursor(bp);
            *cursor |= (1L << shift(bp));
        }

        
        //
        // set_bits_large
        //
        // Set n bits in the bit map to 1 spanning more than one word.
        // Assumes that range spans more than one word.
        //
        void set_bits_large(const usword_t bp, const usword_t n) const;
        

        //
        // set_bits
        //
        // Set bits in the bit map to 1.
        //
        inline void set_bits(const usword_t bp, const usword_t n) const {
            const usword_t sh = shift(bp);                  // bit shift
            
            if ((sh + n) > bits_per_word) {
                set_bits_large(bp, n);
            } else {
                usword_t m = mask(n);                       // mask of n bits
                *bp_cursor(bp) |= (m << sh);                // set bits to 1
            }
        }

        //
        // clear_bit
        //
        // Set a bit in the bit map to 0.
        //
        inline void clear_bit(const usword_t bp) const {
            usword_t *cursor = bp_cursor(bp);
            *cursor &= ~(1L << shift(bp));
        }
        
        
        //
        // clear_bits_large
        //
        // Set n bits in the bit map to 0 spanning more than one word.
        // Assumes that range spans more than one word.
        //
        void clear_bits_large(const usword_t bp, const usword_t n) const; 
               
        //
        // clear_bits
        //
        // Set n bits in the bit map to 0.
        //
        inline void clear_bits(const usword_t bp, const usword_t n) const {
            const usword_t sh = shift(bp);                  // bit shift
            
            if ((sh + n) > bits_per_word) {
                clear_bits_large(bp, n);
            } else {
                usword_t m = mask(n);                       // mask of n bits
                *bp_cursor(bp) &= ~(m << sh);               // set bits to 0
            }
        }
        
        //
        // bits_are_clear_large
        //
        // Checks to see if a range of bits, spanning more than one word, are all 0.
        //
        bool bits_are_clear_large(const usword_t bp, const usword_t n) const;
        
        
        //
        // bits_are_clear
        //
        inline bool bits_are_clear(const usword_t bp, const usword_t n) const {
            const usword_t sh = shift(bp);                  // bit shift

            if ((sh + n) > bits_per_word) {
                return bits_are_clear_large(bp, n);
            } else {
                usword_t m = mask(n);                       // mask of n bits
                return (*bp_cursor(bp) & (m << sh)) == 0;   // see if bits are 0
            }
        }
        

        //
        // skip_all_zeros
        //
        // Rapidly skips through words of all zeros.
        //
        static inline usword_t *skip_all_zeros(usword_t *cursor, usword_t *end) {
            // near end address (allows multiple reads)
            usword_t *near_end = end - 4;
            
            // skip through as many all zeros as we can
            while (cursor < near_end) {
                // prefetch four words
                usword_t word0 = cursor[0];
                usword_t word1 = cursor[1];
                usword_t word2 = cursor[2];
                usword_t word3 = cursor[3];

                // assume they are all filled with zeros
                cursor += 4;

                // backtrack if we find out otherwise
                if (!is_all_zeros(word0)) { cursor -= 4; break; }
                if (!is_all_zeros(word1)) { cursor -= 3; break; }
                if (!is_all_zeros(word2)) { cursor -= 2; break; }
                if (!is_all_zeros(word3)) { cursor -= 1; break; }
            }
            
            // finish off the rest
            while (cursor < end) {
                usword_t word = *cursor++;
                if (!is_all_zeros(word)) { cursor--; break; }
            }

            return cursor;
        }
        
        
        //
        // skip_backward_all_zeros
        //
        // Rapidly skips through words of all zeros.
        //
        static inline usword_t *skip_backward_all_zeros(usword_t *cursor, usword_t *first) {
            // near first address (allows multiple reads)
            usword_t *near_first = first + 3;
            
            // skip through as many all zeros as we can
            while (near_first <= cursor) {
                // prefetch four words
                usword_t word0 = cursor[0];
                usword_t word1 = cursor[-1];
                usword_t word2 = cursor[-2];
                usword_t word3 = cursor[-3];

                // assume they are all filled with zeros
                cursor -= 4;

                // backtrack if we find out otherwise
                if (!is_all_zeros(word0)) { cursor += 4; break; }
                if (!is_all_zeros(word1)) { cursor += 3; break; }
                if (!is_all_zeros(word2)) { cursor += 2; break; }
                if (!is_all_zeros(word3)) { cursor += 1; break; }
            }
            
            // finish off the rest
            while (first <= cursor) {
                usword_t word = *cursor--;
                if (!is_all_zeros(word)) { cursor++; break; }
            }

            return cursor;
        }
 
 
        //
        // count_set
        // 
        // Returns the number of bits set (one) in the bit map using
        // a standard bit population algoirithm.
        //
        usword_t count_set() const ;


        //
        // previous_set
        //
        // Return the bit postion of the 1 that comes at or prior to the bit position 'bp'.
        //
        usword_t previous_set(const usword_t bp) const ;
        
        
        //
        // next_set
        //
        // Return the bit postion of the 1 that comes at or after to the bit position 'bp'.
        //
        usword_t next_set(const usword_t bp) const;
        
       
        // ***** Atomic bitmap operations

        //
        // atomic_compare_and_swap_word
        //
        // This is the basis for all the Bitmap atomic operations
        //
        static inline bool atomic_compare_and_swap_word(usword_t old_value, usword_t new_value, volatile usword_t *addr) {
            ASSERTION(((uintptr_t)addr & mask(bytes_per_word_log2)) == 0);
            return auto_atomic_compare_and_swap(old_value, new_value, addr);
        }
        
        //
        // atomic_and_return_orig
        //
        // Perform an atomic and operation and return the original value
        //
        static usword_t atomic_and_return_orig(usword_t mask, volatile usword_t *addr) {
            usword_t old_value;
            do {
                old_value = *addr;
            } while (!atomic_compare_and_swap_word(old_value, old_value & mask, addr));
            return old_value;
        }
        
        //
        // atomic_or_return_orig
        //
        // Perform an atomic or operation and return the original value
        //
        static usword_t atomic_or_return_orig(usword_t mask, volatile usword_t *addr) {
            usword_t old_value;
            do {
                old_value = *addr;
            } while (!atomic_compare_and_swap_word(old_value, old_value | mask, addr));
            return old_value;
        }
        
        //
        // set_bit_atomic
        //
        // Set a bit in the bit map to 1 atomically.
        //
        inline void set_bit_atomic(const usword_t bp) const {
            test_set_bit_atomic(bp);
        }
        
        //
        // test_and_set_bit_atomic
        //
        // Test a bit, and set it atomically if it hasn't bee set yet. Returns old bit value.
        //
        inline bool test_set_bit_atomic(const usword_t bp) const {
            usword_t bit = 1L << shift(bp);
            volatile usword_t *cursor = bp_cursor(bp);
            usword_t old = *cursor;
            if ((old & bit)==0) {
                old = atomic_or_return_orig(bit, cursor);
            }
            return (old & bit) != 0; // return false if the bit was set by this call
        }
        
        //
        // clear_bit_atomic
        //
        // Set a bit in the bit map to 0 atomically.
        //
        inline void clear_bit_atomic(const usword_t bp) const {
            test_clear_bit_atomic(bp);
        }
        
        
        //
        // test_and_clear_bit
        //
        // Test a bit, and clear it if was set. Returns old bit value.
        //
        inline bool test_clear_bit_atomic(const usword_t bp) const {
            usword_t bit = 1L << shift(bp);
            volatile usword_t *cursor = bp_cursor(bp);
            usword_t old = *cursor;
            if ((old & bit)!=0) {
                old = atomic_and_return_orig(~bit, cursor);
            }
            return (old & bit)!=0; // return true if the bit was cleared by this call
        }
        
        
        //----- AtomicCursor -----//
        
        //
        // AtomicCursor provides a specialized way of doing test and clear searching of a range in a bitmap.
        // It returns the indices for bits which are found to be set, clearing the bits as a side effect.
        // The bitmap is accessed using atomic operations, but a word at a time. A word worth of bits is fetched
        // into a local buffer which is then accessed using nonatomic operations.
        //
        class AtomicCursor {
            Bitmap &_bitmap;        // the bitmap being examined
            usword_t _index;        // the "current bit" - the value that will be returned from next_set_bit() if the bit is set
            usword_t _offset;       // arbitrary offset that is subtracted from values returned by next_set_bit() (adjusts for subzone quanum bias
            usword_t _max_index;    // end point of the scan. the last possible return from next_set_bit() is _max_index-1
            usword_t _copied_bits;  // bits copied out of the bitmap
            usword_t _valid_bits;   // number of valid bits in _copied_bits
            
        public:
            
            //
            // Constructor
            //
            // arguments are the bitmap, starting index, and number of bits to enumerate
            AtomicCursor(Bitmap &b, usword_t start, usword_t length) : _bitmap(b), _index(start), _offset(start), _max_index(start + length) {
                // Perform the first fetch, which may not be word aligned. 
                // This lets us assume _index is word aligned when we fetch in next_set_bit().
                _copied_bits = fetch_clear_bits_atomic(_index, _max_index, _valid_bits);
            };
            
            //
            // fetch_clear_bits_atomic
            //
            // fetch and clear multiple bits at a time
            // bp is the index of the first bit to fetch, max is the first index *not* to be fetched
            // returns a word containing the fetched bits, and by reference in count the number of valid bits in the return
            // returned bits start at bit 0, and bits higher than count are zeroed
            //
            usword_t fetch_clear_bits_atomic(const usword_t bp, const usword_t max, usword_t &count);   
            
            //
            // next_set_bit
            //
            // Returns the index of a set bit from the bitmap range. If no bits are set, returns -1.
            // The returned value is the offset from the start index passed to the constructor.
            // So if 5 is passed as the start bit to the constructor and bit 8 is the first set bit in the bitmap,
            // the first callto next_set_bit() will return 3. (This is done because the cursor is used
            // to enumerate subzone quanta, which are zero based.)
            //
            usword_t next_set_bit();
        };    
    };


};


#endif // __AUTO_BITMAP__
