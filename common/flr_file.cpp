/*
	flr_file.cpp - FLR File class definition.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
			
	2016/10/25, Maya Posch
	(c) Nyanko.ws
*/


#include "flr_file.h"

#include <sstream>
#include <cstring>
#include <iostream>

#include "lzma_util.h"


enum DataFlags {
	DATA_COMPRESSION = 0x01,
	DATA_COMPRESSION_LZMA = 0x02
};


// --- CONSTRUCTOR ---
FlrFile::FlrFile() {
	// Defaults.
	FILE = new fstream;
	IDX = new fstream;
	DNAME = new fstream;
	DATA = new fstream;
	syncedFile = false;
	syncedIdx = false;
	syncedDname = false;
	syncedData = false;
	emptyFile = true;
}


// --- DECONSTRUCTOR ---
FlrFile::~FlrFile() {
	if (FILE) {
		if(FILE->is_open()) { FILE->close(); }
		delete FILE;
	}
	
	if (IDX) {
		if (IDX->is_open()) { IDX->close(); }
		delete IDX; 
	}
	
	if (DNAME) {
		if (DNAME->is_open()) { DNAME->close(); }
		delete DNAME; 
	}
	
	if (DATA) {
		if (DATA->is_open()) { DATA->close(); }
		delete DATA; 
	}
}


// --- READ FILE DATA ---
// Read the file data from the filesystem.
bool FlrFile::readFileData() {
	File file(path + ".file");
	if (!file.exists()) {
		FlrFileMeta meta;
		meta = meta;
		meta.filename = "";
		meta.username = "";
		meta.message = "";
		meta.timestamp = 0;
		meta.delta = 0;
		fileData.meta = meta;
		
		return true;
	}
	
	FILE->open(path + ".file", ios_base::in | ios_base::binary);
	if (!FILE->good()) { FILE->close(); return false; }
	string bin = readFileContents(*FILE);
	FILE->close();
	
	cout << "Parsing FLR file...\n";
	
	// Parse the contents.
	UInt32 index = 0;
	UInt8 len;
	UInt16 len2;
	FlrFileMeta meta;
	
	string signature = bin.substr(index, 7);
	index += 7;
	if (signature != "FLRFILE") {
		// Error reading file.
		cerr << "Error reading file: missing FLRFILE signature. Got: " << signature << endl;
		return false;
	}
	
	UInt64 size = *((UInt64*) &bin[index]);
	index += 8;
	if ((index + size) != bin.length()) {
		// Error reading file.
		cerr << "Error reading file. Wrong length.\n";
		return false;
	}
	
	UInt8 version = *((UInt8*) &bin[index++]);
	if (version != 0) {
		// False version.
		cerr << "Wrong File version.\n";
		return false;
	}
	
	cout << "Finished parsing header section.\n";
	
	signature = bin.substr(index, 4);
	index += 4;
	if (signature != "NAME") {
		// Error reading file.
		cerr << "Missing NAME signature.\n";
		return false;
	}
	
	len2 = *((UInt16*) &bin[index]);
	index += 2;
	meta.filename = bin.substr(index, len2);
	index += len2;
	
	cout << "Finished parsing the NAME section.\n";
	
	signature = bin.substr(index, 4);
	index += 4;
	if (signature != "META") {
		// Error reading file.
		cerr << "Missing META signature.\n";
		return false;
	}
	
	cout << "META section found. Reading...\n";
	
	meta.timestamp = *((UInt64*) &bin[index]);
	index += 8;
	meta.delta = *((UInt32*) &bin[index]);
	index += 4;
	len = *((UInt8*) &bin[index++]);
	meta.username = bin.substr(index, len);
	index += len;
	len2 = *((UInt16*) &bin[index]);
	index += 2;
	meta.message = bin.substr(index, len2);
	index += len2;
	
	cout << "Reading the FLR data section...\n";
	
	// Read data section.
	signature = bin.substr(index, 4);
	index += 4;
	if (signature != "DATA") {
		// Error reading file.
		cerr << "Missing DATA signature.\n";
		return false;
	}
	
	UInt32 len4;
	len4 = *((UInt32*) &bin[index]);
	index += 4;
	if ((index + len4) != bin.length()) {
		// Missing data. Abort.
		cerr << "Missing data. Got " << (index + len4) << ". Expected: " << bin.length() << endl;
		return false;
	}
	
	fileData.meta = meta;
	fileData.data = bin.substr(index, len4);
	
	cout << "Finished parsing FLR file.\n";
	
	syncedFile = true;
	emptyFile = false;
	
	return true;
}


// --- WRITE FILE DATA ---
// Write the file data to the filesystem.
bool FlrFile::writeFileData() {
	cout << "Writing file data...\n";
	// Serialise the data.
	string out;
	string header = "FLRFILE";
	UInt8 revision = 0;
	UInt64 length = 0;
	
	UInt8 len;
	UInt16 len2;
	UInt32 len4;
	out = "NAME";
	len2 = (UInt16) fileData.meta.filename.length();
	out += string((const char*) &len2, 2);
	out += fileData.meta.filename;
	out += "META";
	out += string((const char*) &fileData.meta.timestamp, 8);
	out += string((const char*) &fileData.meta.delta, 4);
	len = (UInt8) fileData.meta.username.length();
	out += string((const char*) &len, 1);
	out += fileData.meta.username;
	len2 = (UInt16) fileData.meta.message.length();
	out += string((const char*) &len2, 2);
	out += fileData.meta.message;
	out += "DATA";
	len4 = (UInt32) fileData.data.length();
	out += string((const char*) &len4, 4);
	out += fileData.data;
	
	// Final length & header.
	length = out.length() + 1; // Revision length is 1.
	header += string((const char*) &length, 8);
	header += string((const char*) &revision, 1);
	
	// Open file for writing, truncating an existing file.
	FILE->open(path + ".file", ios_base::out | ios_base::binary | ios_base::trunc);
	FILE->write(header.data(), header.length());
	FILE->write(out.data(), out.length());
	FILE->close();
	
	return true;
}


// --- READ INDEX ---
// Read the index file from the filesystem.
bool FlrFile::readIndex() {
	cout << "Reading index file...\n";
	File idx(path + ".idx");
	if (!idx.exists()) {
		indexData.indices.clear(); // Clear any existing data.
		syncedIdx = true;
		return true;
	}
	
	IDX->open(path + ".idx", ios_base::in | ios_base::binary);
	string bin = readFileContents(*IDX);
	IDX->close();
	
	// Parse the contents.
	UInt32 index = 0;	
	string signature = bin.substr(index, 7);
	index += 7;
	if (signature != "FLRFILE") {
		// Error reading file.
		cerr << "Failed to find FLRFILE signature." << endl;
		return false;
	}
	
	UInt64 size = *((UInt64*) &bin[index]);
	index += 8;
	if ((index + size) != bin.length()) {
		// Error reading file.
		cerr << "Invalid size." << endl;
		return false;
	}
	
	UInt8 version = *((UInt8*) &bin[index++]);
	if (version != 0) {
		// False version.
		cerr << "Invalid FLR version" << endl;
		return false;
	}
	
	signature = bin.substr(index, 5);
	index += 5;
	if (signature != "INDEX") {
		// Error reading file.
		cerr << "Failed to find INDEX signature." << endl;
		return false;
	}
	
	UInt32 offsetCount = *((UInt32*) &bin[index]);
	index += 4;
	if ((offsetCount * 8) + index != bin.length()) {
		// Error parsing: incorrect length.
		cerr << "Invalid length." << endl;
		return false;
	}
	
	for (UInt32 i = 0; i < offsetCount; ++i) {
		UInt64 offset = *((UInt64*) &bin[index]);
		index += 8;
		indexData.indices.push_back(offset);
	}
	
	syncedIdx = true;
	
	return true;
}


// --- WRITE INDEX ---
// Write the index file to the filesystem.
bool FlrFile::writeIndex() {
	cout << "Writing index file...\n";
	// Serialise the data.
	string out;
	string header = "FLRFILE";
	UInt8 revision = 0;
	UInt64 length = 0;
	
	UInt32 len4;
	out = "INDEX";
	len4 = (UInt32) indexData.indices.size();
	out += string((const char*) &len4, 4);
	for (UInt32 i = 0; i < len4; ++i) {
		out += string((const char*) &(indexData.indices[i]), 8);
	}
	
	// Final length & header.
	length = out.length() + 1; // Revision length is 1.
	header += string((const char*) &length, 8);
	header += string((const char*) &revision, 1);
	
	// Open file for writing, truncating an existing file.
	IDX->open(path + ".idx", ios_base::out | ios_base::binary | ios_base::trunc);
	IDX->write(header.data(), header.length());
	IDX->write(out.data(), out.length());
	IDX->close();
	
	syncedIdx = true;
	
	return true;
}


// --- COPY FILE DATA TO DELTA ---
// Copy the file data and append it to the deltas file.
bool FlrFile::copyFileDataToDelta() {
	cout << "Copying file data to delta file...\n";
	// Take the old 'current' data and add it to the Deltas section, updating the file.
	File file(path + ".file");
	if (!file.exists()) { return true; } // Nothing to copy.
	if (!file.isFile()) { return false; } // Not a file.
	
	// Read in the current file data.
	if (!readFileData()) { 
		cerr << "Failed to read the file data.\n";
		return false; 
	}
	
	cout << "Parsing delta file...\n";
	
	// Write the current data to the deltas file.
	File data(path + ".data");
	if (!data.exists()) {
		// New deltas file, first write the header.
		DATA->open(path + ".data", ios_base::out | ios_base::binary);
		string out;
		out = "DELTAS";
		
		// Footer.
		out += "DELTAS";
		UInt32 count = 0;
		out += string((const char*) &count, 4);
		DATA->write(out.data(), out.length());
		DATA->close();
		
		cout << "Wrote initial header for new delta file.\n";
	}
	
	// (Re)open the deltas file for appending.
	// Read out the footer first before appending the new data.
	DATA->open(path + ".data", ios_base::out | 
								ios_base::in | 
								ios_base::binary);
	if (!DATA->good()) { DATA->close(); return false; }
	DATA->seekp(-10, ios_base::end); // Seek to beginning of footer.
	if (DATA->fail()) {
		// Error seeking.
		return false;
	}
	
	cout << "Finished seeking to footer.\n";
	
	string signature = readBytes(*DATA, 6);
	if (signature != "DELTAS") {
		// Signature fail. Abort.
		cerr << "Failed to find DELTAS signature. Found: " << signature << endl;
		return false;
	}
	
	UInt32 numDeltas;
	DATA->read(reinterpret_cast<char*>(&numDeltas), 4);
	
	// Seek back, then overwrite the footer with the new delta data.
	DATA->seekp(-10, ios_base::end);
	
	// Register the offset we're now at as the offset for the new delta section.
	UInt64 offset = DATA->tellp();
	
	string out;
	UInt8 len;
	UInt16 len2;
	UInt32 len4;
	out = "DELTA";
	UInt32 newDeltaId = numDeltas++;
	out += string((const char*) &newDeltaId, 4);
	out += "META";
	out += string((const char*) &fileData.meta.timestamp, 8);
	len = (UInt8) fileData.meta.username.length();
	out += string((const char*) &len, 1);
	out += fileData.meta.username;
	len2 = (UInt16) fileData.meta.message.length();
	out += string((const char*) &len2, 2);
	out += fileData.meta.message;
	out += "DATA";
	UInt32 flags;
	flags |= DATA_COMPRESSION_LZMA;
	out += string((const char*) &flags, 4);
	string cdata = compressWithLzma(fileData.data, 2);
	len4 = (UInt32) cdata.length();
	out += string((const char*) &len4, 4);
	out += cdata;
	
	// Add the new footer.
	out += "DELTAS";
	out += string((const char*) &numDeltas, 4);
	
	// Write to file, appending the new data.
	DATA->write(out.data(), out.length());
	DATA->close();
	
	syncedData = false; // TODO: maybe update the local delta metas?
	
	// Add the new offset to the index file and write it to disk as well.
	indexData.indices.push_back(offset);
	writeIndex(); // TODO: check return value.
	
	syncedData = true;
	
	return true;
}


// --- READ DELTA ---
// Read the data for a specific file delta.
bool FlrFile::readDelta(UInt32 id, FlrDelta &delta) {
	// First check that we have a synced index, else force a read.
	if (!syncedIdx) {
		if (!readIndex()) { 
			return false; 
		}
	}
	
	// Check that the delta exists and obtain the offset if it does.
	if (id >= indexData.indices.size()) {
		// Invalid iD.
		return false;
	}
	
	UInt64 offset = indexData.indices[id];
	
	// Open the delta file and seek to the offset we got.
	DATA->open(path + ".data", ios_base::in | ios_base::binary);
	if (readBytes(*DATA, 6) != "DELTAS") {
		// Error parsing.
		cerr << "Failed to find DELTAS signature." << endl;
		DATA->close();
		return false;
	}
	
	DATA->seekp(offset);
	
	// Read in the delta.
	FlrFileMeta meta;
	if (readBytes(*DATA, 5) != "DELTA") {
		// Error parsing.
		cerr << "Failed to find DELTA signature." << endl;
		DATA->close();
		return false;
	}
	
	meta.delta = *((UInt32*) readBytes(*DATA, 4).data());
	if (meta.delta != id) {
		// Error parsing.
		DATA->close();
		return false;
	}
	
	if (readBytes(*DATA, 4) != "META") {
		// Error parsing.
		cerr << "Failed to find META signature." << endl;
		DATA->close();
		return false;
	}
	
	meta.timestamp = (UInt64) readBytes(*DATA, 8).data();
	UInt8 len = *((UInt8*) readBytes(*DATA, 1).data());
	meta.username = readBytes(*DATA, len);
	UInt16 len2 = *((UInt16*) readBytes(*DATA, 2).data());
	meta.message = readBytes(*DATA, len2);
	
	if (readBytes(*DATA, 4) != "DATA") {
		// Error parsing.
		cerr << "Failed to find DATA signature." << endl;
		DATA->close();
		return false;
	}
	
	UInt32 flags = *((UInt32*) readBytes(*DATA, 4).data());
	UInt64 size = *((UInt64*) readBytes(*DATA, 8).data());
	delta.data = decompressWithLzma(readBytes(*DATA, size));
	
	DATA->close();
	
	// Get filename for this delta.
	if (!syncedDname) {
		if (!readDeltaNames()) {
			return false;
		}
	}
	
	// Check the 'deltaNames.names' vector.
	// The first index (0) is the oldest entry. Continue checking the next entry until the delta ID
	// is at least equal to or newer than the found ID.
	// Once the found ID is larger than our delta ID, we have found the right filename.
	meta.filename = "";
	for (int i = 0; i < deltaNames.names.size(); ++i) {
		if (deltaNames.names[i].delta >= meta.delta) {
			if (deltaNames.names[i].delta < meta.delta) {
				meta.filename = deltaNames.names[i].name;
			}
			else { break; }
		}
	}
	
	if (meta.filename.empty()) { meta.filename = fileData.meta.filename; }
	
	delta.meta = meta;
	
	return true;
}


// --- READ DELTA META ---
// --- Read the delta meta info from the filesystem for a specific revision.
bool FlrFile::readDeltaMeta(UInt32 id, FlrFileMeta &meta) {
	// First check that we have a synced index, else force a read.
	if (!syncedIdx) {
		if (!readIndex()) { 
			return false;
		}
	}
	
	// Check that the delta exists and obtain the offset if it does.
	if (id >= indexData.indices.size()) {
		// Invalid iD.
		return false;
	}
	
	UInt64 offset = indexData.indices[id];
	
	// Open the delta file and seek to the offset we got.
	DATA->open(path + ".data", ios_base::in | ios_base::binary);
	if (!DATA->good()) {
		DATA->close();
		return false;
	}
	
	if (readBytes(*DATA, 6) != "DELTAS") {
		// Error parsing.
		cerr << "Failed to find DELTAS signature." << endl;
		DATA->close();
		return false;
	}
	
	DATA->seekp(offset);
	
	// Read in the delta.
	if (readBytes(*DATA, 5) != "DELTA") {
		// Error parsing.
		cerr << "Failed to find DELTA signature." << endl;
		DATA->close();
		return false;
	}
	
	meta.delta = *((UInt32*) readBytes(*DATA, 4).data());
	if (meta.delta != id) {
		// Error parsing.
		cerr << "Delta ID and provided ID did not match." << endl;
		DATA->close();
		return false;
	}
	
	if (readBytes(*DATA, 4) != "META") {
		// Error parsing.
		cerr << "Failed to find META signature." << endl;
		DATA->close();
		return false;
	}
	
	meta.timestamp = *((UInt64*) readBytes(*DATA, 8).data());
	UInt8 len = *((UInt8*) readBytes(*DATA, 1).data());
	meta.username = readBytes(*DATA, len);
	UInt16 len2 = *((UInt16*) readBytes(*DATA, 2).data());
	meta.message = readBytes(*DATA, len2);
	
	DATA->close();
	
	// Get filename for this delta.
	if (!syncedDname) {
		if (!readDeltaNames()) {
			return false;
		}
	}
	
	// Check the 'deltaNames.names' vector.
	// The first index (0) is the oldest entry. Continue checking the next entry until the delta ID
	// is at least equal to or newer than the found ID.
	// Once the found ID is larger than our delta ID, we have found the right filename.
	meta.filename = "";
	for (int i = 0; i < deltaNames.names.size(); ++i) {
		if (deltaNames.names[i].delta >= meta.delta) {
			if (deltaNames.names[i].delta < meta.delta) {
				meta.filename = deltaNames.names[i].name;
			}
			else { break; }
		}
	}
	
	if (meta.filename.empty()) { meta.filename = fileData.meta.filename; }
	
	return true;
}


// --- READ DELTA METAS ---
// Read all delta metas from the filesystem.
bool FlrFile::readDeltaMetas() {
	// First check that we have a synced index, else force a read.
	if (!syncedIdx) {
		if (!readIndex()) { 
			return false;
		}
	}
		
	// Cycle through the offsets, reading the meta data for each delta.
	// Open the delta file to enable us to seek to the offsets.
	DATA->open(path + ".data", ios_base::in | ios_base::binary);
	if (!DATA->good()) { DATA-> close(); return false; }
	if (readBytes(*DATA, 6) != "DELTAS") {
		// Error parsing.
		cerr << "Failed to find DELTAS signature." << endl;
		DATA->close();
		return false;
	}
	
	// Synchronise the delta names data.
	if (!syncedDname) {
		if (!readDeltaNames()) {
			DATA->close();
			return false;
		}
	}
	
	for (int i = 0; i < indexData.indices.size(); ++i) {
		UInt64 offset = indexData.indices[i];
	
		DATA->seekp(offset);
		
		// Read in the delta.
		if (readBytes(*DATA, 5) != "DELTA") {
			// Error parsing.
		cerr << "Failed to find DELTA signature." << endl;
			DATA->close();
			return false;
		}
		
		UInt32 delta = *((UInt32*) readBytes(*DATA, 4).data());
		if (delta != i) {
			// Error parsing.
		cerr << "Delta ID doesn't match sequence ID." << endl;
			DATA->close();
			return false;
		}
		
		if (readBytes(*DATA, 4) != "META") {
			// Error parsing.
		cerr << "Failed to find META signature." << endl;
			DATA->close();
			return false;
		}
		
		FlrFileMeta meta;
		meta.timestamp = *((UInt64*) readBytes(*DATA, 8).data());
		UInt8 len = *((UInt8*) readBytes(*DATA, 1).data());
		meta.username = readBytes(*DATA, len);
		UInt16 len2 = *((UInt16*) readBytes(*DATA, 2).data());
		meta.message = readBytes(*DATA, len2);
		
		// Get filename for this delta.
		// Check the 'deltaNames.names' vector.
		// The first index (0) is the oldest entry. Continue checking the next entry until the delta ID
		// is at least equal to or newer than the found ID.
		// Once the found ID is larger than our delta ID, we have found the right filename.
		meta.filename = "";
		for (int i = 0; i < deltaNames.names.size(); ++i) {
			if (deltaNames.names[i].delta >= delta) {
				if (deltaNames.names[i].delta < delta) {
					meta.filename = deltaNames.names[i].name;
				}
				else { break; }
			}
		}
		
		if (meta.filename.empty()) { meta.filename = fileData.meta.filename; }
		
		deltaMetas.push_back(meta);
	}
	
	DATA->close();
	syncedData = true;
	
	return true;
}


// --- READ DELTA NAMES ---
// Read the delta names (dnames) data from the filesystem.
bool FlrFile::readDeltaNames() {
	File dname(path + ".dname");
	if (!dname.exists()) {
		deltaNames.names.clear(); // Clear any existing data.
		syncedDname = true;
		
		return true;
	}
	
	DNAME->open(path + ".dname", ios_base::in | ios_base::binary);
	if (!DNAME->good()) {
		DNAME->close();
		return false;
	}
	
	string bin = readFileContents(*DNAME);
	DNAME->close();
	
	// Parse the contents.
	UInt32 index = 0;	
	UInt16 len2;
	string signature = bin.substr(index, 7);
	index += 7;
	if (signature != "FLRFILE") {
		cerr << "Failed to find FLRFILE signature." << endl;
		// Error reading file.
		return false;
	}
	
	UInt64 size = *((UInt64*) &bin[index]);
	index += 8;
	if ((index + size) != bin.length()) {
		// Error reading file.
		cerr << "Invalid size." << endl;
		return false;
	}
	
	UInt8 version = *((UInt8*) &bin[index++]);
	if (version != 0) {
		// False version.
		return false;
	}
	
	signature = bin.substr(index, 6);
	index += 6;
	if (signature != "DNAMES") {
		// Error reading file.
		cerr << "Failed to find DNAMES signature." << endl;
		return false;
	}
	
	UInt32 nameCount = *((UInt32*) &bin[index]);
	index += 4;
	
	// Clear the existing data.
	deltaNames.names.clear();
	
	// Read in the new values.
	for (UInt32 i = 0; i < nameCount; ++i) {
		signature = bin.substr(index, 5);
		if (signature != "DNAME") {
			// Error while parsing
			cerr << "Failed to find DNAME signature." << endl;
			return false;
		}
		
		FlrDeltaName name;
		name.delta = *((UInt32*) &bin[index]);
		index += 4;
		len2 = *((UInt16*) &bin[index]);
		name.name = bin.substr(index, len2);
		index += len2;
		deltaNames.names.push_back(name);
	}
	
	syncedDname = true;
	
	return true;
}


// --- WRITE DELTA NAMES ---
bool FlrFile::writeDeltaNames() {
	// Serialise the data.
	string out;
	string header = "FLRFILE";
	UInt8 revision = 0;
	UInt64 length = 0;
	
	UInt32 len4;
	UInt16 len2;
	out = "DNAMES";
	len4 = (UInt32) deltaNames.names.size();
	out += string((const char*) &len4, 4);
	for (UInt32 i = 0; i < len4; ++i) {
		out += "DNAME";
		FlrDeltaName &name = deltaNames.names[i];
		out += string((const char*) &name.delta, 4);
		len2 = name.name.length();
		out += string((const char*) &len2, 4);
		out += name.name;
	}
	
	// Final length & header.
	length = out.length() + 1; // Revision length is 1.
	header += string((const char*) &length, 8);
	header += string((const char*) &revision, 1);
	
	// Open file for writing, truncating an existing file.
	DNAME->open(path + ".dname", ios_base::out | ios_base::binary | ios_base::trunc);
	DNAME->write(header.data(), header.length());
	DNAME->write(out.data(), out.length());
	DNAME->close();
	
	syncedDname = true;
	
	return true;
}


// --- READ FILE CONTENTS ---
string FlrFile::readFileContents(fstream &in) {
    return static_cast<stringstream const&>(stringstream() << in.rdbuf()).str();
}


// --- READ BYTES ---
// Read the specified number of 
string FlrFile::readBytes(istream &stream, uint32_t count) {
    vector<char> result(count);
    stream.read(&result[0], count);

    return string(&result[0], &result[count]);
}


// --- OPEN FLR FILE ---
// Initialise the File instances for the provided path.
bool FlrFile::openFlrFile(string path) {
	this->path = path;
	
	/*try {
		file = File(path + ".file");
		idx = File(path + ".idx");
		dname = File(path + ".dname");
		data = File(path + ".data");
	}
	catch (...) {
		cerr << "Caught exception opening FLR files.\n";
	}*/
	
	// If the files exist, load the current meta data for this file.
	File file (path + ".file");
	if (file.exists() && file.isFile()) {
		emptyFile = false;
		cout << "Reading FLR file...\n";
		return readFileData();
	}
	else {
		emptyFile = true;
	}
	
	return true;
}


// --- ADD FILE DATA ---
// Update the 'current' file data, archiving the previous version.
bool FlrFile::addFileData(string file, FlrFileMeta meta) {
	// Copy the old data to the deltas file.
	if (!copyFileDataToDelta()) {
		cerr << "Copying file data to delta file failed.\n";
		return false;
	}
	
	// Check for a filename change.
	if (emptyFile) {
		// No file data yet.
		meta.delta = 0;
		fileData.meta = meta;
		fileData.data = file;
		emptyFile = false;
	}	
	else if (meta.filename != fileData.meta.filename) {
		cout << "Filename changed. Was: " << fileData.meta.filename << 
				". New: " << meta.filename << endl;
		FlrDeltaName dname;
		fileData.meta.filename = meta.filename;
		dname.name = meta.filename;
		
		dname.delta = ++fileData.meta.delta;
		
		deltaNames.names.push_back(dname);
		if (!writeDeltaNames()) { return false; }
		
		// Set the data for the new file.
		fileData.meta.delta++;
		fileData.meta.username = meta.username;
		fileData.meta.message = meta.message;
		fileData.meta.timestamp = meta.timestamp;
		fileData.data = file;
	}
	else {
		// Set the data for the new file.
		fileData.meta.delta++;
		fileData.meta.username = meta.username;
		fileData.meta.message = meta.message;
		fileData.meta.timestamp = meta.timestamp;
		fileData.data = file;
	}
	
	// Write new 'current' file.
	return writeFileData();
}



// --- ADD FILE NAME ---
// Update the 'current' name of the file.
/* bool FlrFile::addFileName(string name, FlrFileMeta meta) {
	// Add the old filename to the delta list and write it.
	if (!syncedFile) { readFileData(); }
	
	FlrDeltaName dname;
	if (deltaMetas.empty()) { dname.delta = 0; }
	else { dname.delta = deltaMetas.back().
	dname.name = fileData.meta.filename;
	deltaNames.names.push_back(dname);
	if (!writeDeltaNames()) { return false; }
	
	// Write new 
} */


// --- CURRENT FILE ---
bool FlrFile::currentFile(string &data, FlrFileMeta &meta) {
	if (!syncedFile) { readFileData(); }
	
	data = fileData.data;
	meta = fileData.meta;
	
	return true;
}


// --- FILE REVISION ---
bool FlrFile::fileRevision(UInt32 id, FlrDelta &delta) {
	return readDelta(id, delta);
}


// --- REVISIONS ---
vector<FlrFileMeta> FlrFile::revisions() {
	if (!syncedData) { readDeltaMetas(); }
	
	return deltaMetas;
}
