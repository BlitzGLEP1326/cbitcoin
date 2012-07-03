#!/usr/bin/env python
#
#  structure_maker.py
#  cbitcoin
#
#  Created by Matthew Mitchell on 28/04/2012.
#  Copyright (c) 2012 Matthew Mitchell
#  
#  This file is part of cbitcoin.
#
#  cbitcoin is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#  
#  cbitcoin is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with cbitcoin.  If not, see <http://www.gnu.org/licenses/>.

#  This file makes a structure inheriting CBObject. May be improved to make structures inheriting a selected structure.

import os,sys
from datetime import datetime

name = raw_input("Enter the name of the new structure (Do not include CB): ")
author = raw_input("Enter your first and last name: ")
desc = raw_input("Enter a description for the structure: ")
d = datetime.utcnow()
# Make directory
dir = os.path.dirname(sys.argv[0]) + "/../src/structures/CBObject/CB" + name
if os.path.exists(dir):
	input = raw_input("This name conflicts with a previous name. Overwrite? (Y/N): ")
	if input != "Y":
		sys.exit()
else:
	os.makedirs(dir)
# Make files
header = open(dir + "/CB" + name + ".h","w")
source = open(dir + "/CB" + name + ".c","w")
# Header
header.write("//\n\
//  CB"+name+".h\n\
//  cbitcoin\n\
//\n\
//  Created by "+author+" on "+d.strftime("%d/%m/%Y")+".\n\
//  Copyright (c) 2012 Matthew Mitchell\n\
//  \n\
//  This file is part of cbitcoin.\n\
//\n\
//  cbitcoin is free software: you can redistribute it and/or modify\n\
//  it under the terms of the GNU General Public License as published by\n\
//  the Free Software Foundation, either version 3 of the License, or\n\
//  (at your option) any later version.\n\
//  \n\
//  cbitcoin is distributed in the hope that it will be useful,\n\
//  but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
//  GNU General Public License for more details.\n\
//  \n\
//  You should have received a copy of the GNU General Public License\n\
//  along with cbitcoin.  If not, see <http://www.gnu.org/licenses/>.\n\
\n\
/**\n\
 @file\n\
 @brief "+desc+" Inherits CBObject\n\
*/\n\
\n\
#ifndef CB"+name.upper()+"H\n\
#define CB"+name.upper()+"H\n\
\n\
//  Includes\n\
\n\
#include \"CBObject.h\"\n\
\n\
/**\n\
 @brief Structure for CB"+name+" objects. @see CB"+name+".h\n\
*/\n\
typedef struct{\n\
\tCBObject base; /**< CBObject base structure */\n\
} CB"+name+";\n\
\n\
/**\n\
 @brief Creates a new CB"+name+" object.\n\
 @returns A new CB"+name+" object.\n\
 */\n\
CB"+name+" * CBNew"+name+"(void);\n\
\n\
/**\n\
 @brief Gets a CB"+name+" from another object. Use this to avoid casts.\n\
 @param self The object to obtain the CB"+name+" from.\n\
 @returns The CB"+name+" object.\n\
 */\n\
CB"+name+" * CBGet"+name+"(void * self);\n\
\n\
/**\n\
 @brief Initialises a CB"+name+" object\n\
 @param self The CB"+name+" object to initialise\n\
 @returns true on success, false on failure.\n\
 */\n\
bool CBInit"+name+"(CB"+name+" * self);\n\
\n\
/**\n\
 @brief Frees a CB"+name+" object.\n\
 @param self The CB"+name+" object to free.\n\
 */\n\
void CBFree"+name+"(void * self);\n\
 \n\
//  Functions\n\
\n\
#endif");
# Source
source.write("//\n\
//  CB"+name+".c\n\
//  cbitcoin\n\
//\n\
//  Created by "+author+" on "+d.strftime("%d/%m/%Y")+".\n\
//  Copyright (c) 2012 Matthew Mitchell\n\
//  \n\
//  This file is part of cbitcoin.\n\
//\n\
//  cbitcoin is free software: you can redistribute it and/or modify\n\
//  it under the terms of the GNU General Public License as published by\n\
//  the Free Software Foundation, either version 3 of the License, or\n\
//  (at your option) any later version.\n\
//  \n\
//  cbitcoin is distributed in the hope that it will be useful,\n\
//  but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n\
//  GNU General Public License for more details.\n\
//  \n\
//  You should have received a copy of the GNU General Public License\n\
//  along with cbitcoin.  If not, see <http://www.gnu.org/licenses/>.\n\
\n\
//  SEE HEADER FILE FOR DOCUMENTATION \n\
\n\
#include \"CB"+name+".h\"\n\
\n\
//  Constructor\n\
\n\
CB"+name+" * CBNew"+name+"(){\n\
\tCB"+name+" * self = malloc(sizeof(*self));\n\
\tCBGetObject(self)->free = CBFree"+name+";\n\
\tCBInit"+name+"(self);\n\
\treturn self;\n\
}\n\
\n\
//  Object Getter\n\
\n\
CB"+name+" * CBGet"+name+"(void * self){\n\
\treturn self;\n\
}\n\
\n\
//  Initialiser\n\
\n\
bool CBInit"+name+"(CB"+name+" * self){\n\
\tif (!CBInitObject(CBGetObject(self)))\n\
\t\treturn false;\n\
\treturn true;\n\
}\n\
\n\
//  Destructor\n\
\n\
void CBFree"+name+"(void * self){\n\
\tCBFreeObject(self);\n\
}\n\
\n\
//  Functions\n\
\n")
# Close files
header.close()
source.close()
