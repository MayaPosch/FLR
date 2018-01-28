/*
	flr_file.h - FLR File class declaration.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
			
	2016/10/25, Maya Posch
	(c) Nyanko.ws
*/


#pragma once
#ifndef FLR_FILE_H
#define FLR_FILE_H

#include <string>
#include <fstream>
#include <vector>

using namespace std;

#include <Poco/Timestamp.h>
#include <Poco/File.h>

using namespace Poco;


// Types.
struct FlrFileMeta {
	string filename;
	string username;
	string message;
	UInt64 timestamp;
	UInt32 delta;
};

struct FlrFileData {
	FlrFileMeta meta;
	string data;
};

struct FlrIndex {
	std::vector<UInt64> indices;
};

struct FlrDelta {
	FlrFileMeta meta;
	string data;
};

struct FlrDeltaName {
	UInt32 delta;
	string name;
};

struct FlrDeltaNames {
	std::vector<FlrDeltaName> names;
};
// --


class FlrFile {
	string path;
	string filedata;
	fstream* FILE;
	fstream* IDX;
	fstream* DNAME;
	fstream* DATA;
	//File file, idx, dname, data;
	FlrFileData fileData;
	FlrIndex indexData;
	FlrDeltaNames deltaNames;
	std::vector<FlrFileMeta> deltaMetas;
	bool syncedFile, syncedIdx, syncedDname, syncedData;
	bool emptyFile;
	
	bool readFileData();
	bool writeFileData();
	bool readIndex();
	bool writeIndex();
	bool copyFileDataToDelta();
	bool readDelta(UInt32 id, FlrDelta &delta);
	bool readDeltaMeta(UInt32 id, FlrFileMeta &meta);
	bool readDeltaMetas();
	bool readDeltaNames();
	bool writeDeltaNames();
	
	string readFileContents(fstream &in);
	string readBytes(istream &stream, UInt32 count);
	
public:
	FlrFile();
	~FlrFile();

	bool openFlrFile(string path);
	bool addFileData(string file, FlrFileMeta meta);
	//bool addFileName(string name, FlrFileMeta meta);
	bool currentFile(string &data, FlrFileMeta &meta);
	bool fileRevision(UInt32 id, FlrDelta &delta);
	std::vector<FlrFileMeta> revisions();
};

#endif
