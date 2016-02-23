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
    FreeList.h
    Free list for memory allocator
    Copyright (c) 2004-2011 Apple Inc. All rights reserved.
 */

#pragma once
#ifndef __AUTO_FREE_LIST__
#define __AUTO_FREE_LIST__

#include "Configuration.h"
#include "Definitions.h"
#include "Range.h"

namespace Auto {

    // Forward declarations
    //
    class Subzone;
    
    
    //----- FreeListNode -----//
    
    class FreeListNode : public Preallocated {
      private:
      
        FreeListNode *_prev;            // previous node or NULL for head
        FreeListNode *_next;            // next node or NULL for tail
        usword_t      _size;            // number of free bytes 
        bool          _purged;          // note:  this field must only be used for nodes > 2 quanta (see Admin::purge_free_space()).
        // WARNING: No more fields.  Must fit in a 16 byte quantum and size is
        // always stuffed in last word of free bytes.
        //usword_t    _size_again;      // at end of free block
        
        // _prev/_next pointers are bitwise complemented to make them look less like valid data.
        static FreeListNode *flip(FreeListNode *node) { return (FreeListNode *)(uintptr_t(node) ^ all_ones); }
        
      public:
      
        //
        // Constructor
        //
        FreeListNode(FreeListNode *prev, FreeListNode *next, usword_t size) {
            set_prev(prev);
            set_next(next);
            set_size(size);
            if (size >= allocate_quantum_medium) _purged = false;
        }
        
        FreeListNode() {
            // reconstruct a free list node in place
            set_prev(NULL);
            set_next(NULL);
            ASSERTION(size() == size_again());
        }
        
        //
        // Accessors
        //
        FreeListNode *prev()         const { return flip(_prev); }
        FreeListNode *next()         const { return flip(_next); }
        usword_t size()              const { return _size; }
        usword_t size_again()              { return *(usword_t *)displace(this, _size - sizeof(usword_t)); }
        void set_prev(FreeListNode *prev)  { _prev = flip(prev); }
        void set_next(FreeListNode *next)  { _next = flip(next); }
        
        //
        // The following are only used by by Admin::purge_free_space() for medium quanta nodes.
        //
        bool is_purged()             const { ASSERTION(_size >= allocate_quantum_medium); return _purged; }
        void set_purged(bool purged)       { ASSERTION(_size >= allocate_quantum_medium); _purged = purged; }

        //
        // Consistency
        //
        void validate()                    { ASSERTION(size() == size_again()); }
        
        
        //
        // address
        //
        // Return address of free block.  Some hocus pocus here.  Returns NULL
        // if the node is NULL.
        //
        void *address()      const { return (void *)this; }
        
       
        //
        // set_size
        //
        // Set size field and place size in last word of free block so that it
        // can be found to merge prior blocks.
        //
        void set_size(usword_t size) {
            // set size field
            _size = size;
            // set size at end of free block
            *(usword_t *)displace(this, _size - sizeof(usword_t)) = size;
        }
        
        
        //
        // prior_node
        //
        // Return the prior adjacent free block.
        //
        FreeListNode *prior_node() {
            // get address of last word in prior free block
            usword_t *end_size = (usword_t *)displace(this, -sizeof(usword_t));
            // return size from prior free block
            return (FreeListNode *)displace(this, -(*end_size));
        }
        
        
        //
        // next_block
        //
        // Return the next adjacent block.
        //
        void *next_block() { return displace(this, _size);  }
        
        //
        // purgeable_range
        //
        // Returns the address range of this node that can be safely passed to uncommit_memory().
        //
        Range purgeable_range() {
            Range r(align_up(displace(this, sizeof(FreeListNode))), align_down(displace(this, _size - sizeof(usword_t) - 1)));
            return r;
        }
    };
    
    
    //----- FreeList -----//
    
    class FreeList {
      private:
    
        //
        // Constructor
        //
        FreeListNode *_head;            // head of free list
        FreeListNode *_tail;            // tail of free list

      public:
      
        FreeList()
        : _head(NULL), _tail(NULL)
        {}
        
        
        //
        // Accessors
        //
        FreeListNode *head() const { return _head; }
        
        
        //
        // pop
        //
        // Pop the first node from the list.
        //
        FreeListNode *pop() {
            // get first node
            FreeListNode *node = _head;
            
            // If there is a node and there is a next node
            if (node) {
                _head = node->next();
                if (_head) {
                    // update the next node's previous
                    _head->set_prev(NULL);
                } else {
                    _tail = NULL;
                }
            }
            
            return node;
        }
        
        
        //
        // push
        //
        // Push an node onto the head of the list
        //
        void push(void *address, usword_t size) {
            // apply FreeListNode template to address
            FreeListNode *node = new(address) FreeListNode(NULL, _head, size);
            // if there is a next node its previous
            if (_head) {
                _head->set_prev(node);
            } else {
                _tail = node;
            }
            // update list
            _head = node;
        }
        
        
        //
        // append
        //
        // append a node onto the tail of the list
        //
        void append(FreeListNode *node) {
            node->set_prev(_tail);
            if (!_head) {
                _head = node;
            } else {
                _tail->set_next(node);
            }
            _tail = node;
        }
        
        
        //
        // remove
        //
        // Remove a node from the list.
        //
        void remove(FreeListNode *node) {
            FreeListNode *prev = node->prev();
            
            // if not front of list
            if (prev) {
                // link the previous node to the next
                FreeListNode *next = node->next();
                prev->set_next(next);
                // link the next node to the previous
                if (next) {
                    next->set_prev(prev);
                } else {
                    _tail = prev;
                }
            } else {
                // pop the list
                pop();
            }
        }
        
        
        //
        // insert
        //
        // Inserts a newly constructed node into an already sorted list.
        //
        void insert(void *address, usword_t size) {
            // keep list sorted.
            FreeListNode *prevNode = NULL, *nextNode = _head;
            while (nextNode != NULL) {
                if (uintptr_t(address) < uintptr_t(nextNode)) break;
                prevNode = nextNode;
                nextNode = nextNode->next();
            }
            FreeListNode *node = new(address) FreeListNode(prevNode, nextNode, size);
            if (nextNode) nextNode->set_prev(node);
            if (prevNode) prevNode->set_next(node);
            if (_head == nextNode) _head = node;
            if (_tail == prevNode) _tail = node;
        }
        
        
        //
        // reset
        //
        // Reset the free list to empty. Nodes on the list are simply dropped.
        //
        void reset() {
            _head = NULL;
            _tail = NULL;
        }
    };

};


#endif // __AUTO_FREE_LIST__
