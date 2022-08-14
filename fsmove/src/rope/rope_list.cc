/*
 * fstransform - transform a file-system to another file-system type,
 *               preserving its contents and without the need for a backup
 *
 * Copyright (C) 2011-2012 Massimiliano Ghilardi
 *
 *     This program is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 2 of the License, or
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
 * rope_list.cc
 *
 *  Created on: Mar 3, 2018
 *      Author: max
 */

#include "../first.hh"

#include <stdexcept>     // for std::out_of_range


#include "rope_list.hh"  // for ft_rope_list

FT_NAMESPACE_BEGIN

/** copy constructor. */
ft_rope_list::ft_rope_list(const ft_rope_list & other) : head(NULL)
{
	init(other.head);
}

/** assignment operator */
const ft_rope_list & ft_rope_list::operator=(const ft_rope_list & other)
{
	if (head != other.head) {
		clear();
		init(other.head);
	}
	return *this;
}

void ft_rope_list::clear()
{
	ft_rope_node * next;
	while (head != NULL) {
		next = head->next;
		delete head;
		head = next;
	}
}

void ft_rope_list::init(const ft_rope_node * list)
{
	head = NULL;
	ft_rope_node ** dst = &head;
	while (list != NULL) {
		*dst = new ft_rope_node(list->e);
		dst = & (*dst)->next;
		list = list->next;
	}
}

ft_rope & ft_rope_list::front()
{
	ft_rope_node * node = head;
	if (node == NULL)
		throw std::out_of_range("ft_rope_list::front(): list is empty");
	return node->e;
}

void ft_rope_list::pop_front()
{
	ft_rope_node * node = head;
	if (node == NULL)
		throw std::out_of_range("ft_rope_list::pop_front(): list is empty");
	head = node->next;
	delete node;
}

void ft_rope_list::push_front(const ft_rope & e)
{
	ft_rope_node * node = new ft_rope_node(e, head);
	head = node;
}

FT_NAMESPACE_END
