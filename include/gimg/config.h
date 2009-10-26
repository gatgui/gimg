/*

Copyright (C) 2009  Gaetan Guidet

This file is part of gimg.

gimg is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or (at
your option) any later version.

gimg is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
USA.

*/

#ifndef __gimg_config_h_
#define __gimg_config_h_

#ifndef GIMG_STATIC
# ifdef _WIN32
#   ifdef GIMG_EXPORTS
#     define GIMG_API __declspec(dllexport)
#   else
#     define GIMG_API __declspec(dllimport)
#   endif
# else
#   define GIMG_API
# endif
#else
# define GIMG_API
#endif

#ifdef _MSC_VER
# pragma warning(disable: 4702)
#endif

#include <iostream>
#include <string>
#include <vector>

#endif
