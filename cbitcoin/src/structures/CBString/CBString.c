//
//  CBString.c
//  cbitcoin
//
//  Created by Matthew Mitchell on 16/05/2012.
//  Copyright (c) 2012 Matthew Mitchell
//  
//  This file is part of cbitcoin.
//
//  cbitcoin is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//  
//  cbitcoin is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with cbitcoin.  If not, see <http://www.gnu.org/licenses/>.

#include "CBString.h"

//  Virtual Table Store

static void * VTStore = NULL;
static int objectNum = 0;

//  Constructors

CBString * CBNewStringByCopyingCString(char * string){
	CBString * self = malloc(sizeof(*self));
	objectNum++;
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateStringVT);
	CBInitStringByCopyingCString(self,string);
	return self;
}
CBString * CBNewStringByTakingCString(char * string){
	objectNum++;
	CBString * self = malloc(sizeof(*self));
	CBAddVTToObject(CBGetObject(self), &VTStore, CBCreateStringVT);
	CBInitStringByTakingCString(self,string);
	return self;
}

//  Virtual Table Creation

CBStringVT * CBCreateStringVT(){
	CBStringVT * VT = malloc(sizeof(*VT));
	CBSetStringVT(VT);
	return VT;
}
void CBSetStringVT(CBStringVT * VT){
	CBSetObjectVT((CBObjectVT *)VT);
	((CBObjectVT *)VT)->free = (void (*)(void *))CBFreeString;
}

//  Virtual Table Getter

CBStringVT * CBGetStringVT(void * self){
	return ((CBStringVT *)(CBGetObject(self))->VT);
}

//  Object Getter

CBString * CBGetString(void * self){
	return self;
}

//  Initialisers

bool CBInitStringByCopyingCString(CBString * self,char * string){
	self->string = malloc(strlen(string));
	strcpy(self->string,string);
	if (!CBInitObject(CBGetObject(self)))
		return false;
	return true;
}
bool CBInitStringByTakingCString(CBString * self,char * string){
	self->string = string;
	if (!CBInitObject(CBGetObject(self)))
		return false;
	return true;
}

//  Destructor

void CBFreeString(CBString * self){
	CBFreeProcessString(self);
	CBFree();
}
void CBFreeProcessString(CBString * self){
	CBFreeProcessObject(CBGetObject(self));
	free(self->string);
}

//  Functions

