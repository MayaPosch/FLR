/*
	flr_collection.h - FLR Collection class declaration.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
			
	2016/10/25, Maya Posch
	(c) Nyanko.ws
*/


#pragma once
#ifndef FLR_COLLECTION_H
#define FLR_COLLECTION_H

#include <string>

using namespace std;


class FlrCollection {
	//
	
public:
	FlrCollection();
	~FlrCollection();
	
	bool openFlrCollection(string path);
};

#endif
