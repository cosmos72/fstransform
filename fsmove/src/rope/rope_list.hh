/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * rope_list.hh
 *
 *  Created on: Mar 4, 2018
 *      Author: max
 */

#ifndef FSTRANSFORM_ROPE_LIST_HH
#define FSTRANSFORM_ROPE_LIST_HH

#include "rope.hh"   // for ft_rope

FT_NAMESPACE_BEGIN

// -----------------------------------------------------------------------------
/**
 * singly-linked list of ropes
 */
class ft_rope_list
{
private:
	class ft_rope_node {
	public:
		ft_rope e;
		ft_rope_node * next;
		
		explicit FT_INLINE ft_rope_node(const ft_rope & el, ft_rope_node * tail = NULL) : e(el), next(tail)
		{ }
		
		FT_INLINE ~ft_rope_node()
		{ }
	};
        
	ft_rope_node * head;
	
	void init(const ft_rope_node * list);
	
public:
	/** default constructor. */
	FT_INLINE ft_rope_list() : head(NULL)
	{ }

	/** copy constructor. */
	ft_rope_list(const ft_rope_list & other);

	/** assignment operator */
	const ft_rope_list & operator=(const ft_rope_list & other);
        
	/** destructor */
	FT_INLINE ~ft_rope_list() {
		clear();
	}

	FT_INLINE bool empty() const {
		return head == NULL;
	}
	
	void clear();

	ft_rope & front();
	void pop_front();	
	void push_front(const ft_rope & e);
	
	FT_INLINE const ft_rope & front() const {
		return const_cast<ft_rope_list *>(this)->front();
	}

	class iterator {
	private:
		friend class ft_rope_list;
		ft_rope_node * node;
		
		explicit FT_INLINE iterator(ft_rope_node * list) : node(list)
		{ }
	public:
		/** default constructor. */
		FT_INLINE iterator() : node(NULL)
		{ }

		/** dereference. */
		FT_INLINE ft_rope & operator*() {
			return node->e;
		}
		
		/** operator-> */
		FT_INLINE ft_rope * operator->() {
			return & node->e;
		}
		/** pre-increment */
		FT_INLINE iterator & operator++() {
			node = node->next;
			return *this;
		}
		/** comparison. */
		FT_INLINE bool operator==(const iterator & other) const {
			return node == other.node;
		}
		FT_INLINE bool operator!=(const iterator & other) const {
			return node != other.node;
		}
	};
	
	class const_iterator {
	private:
		friend class ft_rope_list;
		const ft_rope_node * node;
		
		explicit FT_INLINE const_iterator(const ft_rope_node * list) : node(list)
		{ }
	public:
		/** default constructor. */
		FT_INLINE const_iterator() : node(NULL)
		{ }
		/** dereference. */
		FT_INLINE const ft_rope & operator*() {
			return node->e;
		}
		/** operator-> */
		FT_INLINE const ft_rope * operator->() {
			return & node->e;
		}
		/** pre-increment */
		FT_INLINE const_iterator & operator++() {
			node = node->next;
			return *this;
		}
		/** comparison. */
		FT_INLINE bool operator==(const const_iterator & other) const {
			return node == other.node;
		}
		FT_INLINE bool operator!=(const const_iterator & other) const {
			return node != other.node;
		}
	};
	
	iterator begin() {
		return iterator(head);
	}
	const_iterator begin() const {
		return const_iterator(head);
	}
	iterator end() {
		return iterator();
	}
	const_iterator end() const {
		return const_iterator();
	}
};

FT_NAMESPACE_END

#endif /* FSTRANSFORM_ROPE_LIST_HH */
