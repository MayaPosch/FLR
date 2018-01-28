/*
	flr_server.cpp - Main file of the FLR server.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
			
	2016/09/24, Maya Posch
	(c) Nyanko.ws
*/


#include <nymph.h>


#include <string>
#include <iostream>
#include <vector>
#include <csignal>
#include <map>

using namespace std;


#include <Poco/Condition.h>
#include <Poco/Thread.h>
#include <Poco/Util/IniFileConfiguration.h>
#include <Poco/AutoPtr.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/SQLite/Connector.h>
#include <Poco/Crypto/DigestEngine.h>
#include <Poco/Timestamp.h>

using namespace Poco;
using namespace Poco::Util;
using namespace Poco::Data::Keywords;


#include "../common/flr_file.h"


Data::Session* sql_session;
map<UInt32, bool> sessions;
Condition gCon;
Mutex gMutex;
//volatile sig_atomic_t gSignalTrigger = 0;


void signal_handler(int signal) {
	//gSignalTrigger = 1;
	gCon.signal();
}


// --- LOG FUNCTION ---
void logFunction(int level, string logStr) {
	cout << level << " - " << logStr << endl;
}


// --- CALLBACKS ---
// These are generic methods, with the session ID used to identify a specific session.

// --- AUTHENTICATE CALLBACK ---
NymphMessage* authenticateCallback(int session, NymphMessage* msg, void* data) {
	cout << "Received message for session: " << session << ", msg ID: " << msg->getMessageId() << endl;
	
	// Get the credentials from the message.
	string username = ((NymphString*) msg->parameters()[0])->getValue();
	string password = ((NymphString*) msg->parameters()[1])->getValue();
	
	// Prepare the return message.
	NymphMessage* returnMsg = msg->getReplyMessage();
	
	// Compare the stored credentials with the provided ones.
	Data::Statement select(*sql_session);
	string passwordHash;
	select << "SELECT password FROM users WHERE username=?",
				into (passwordHash),
				use (username);
				
	size_t rows = select.execute();
	NymphBoolean* result = new NymphBoolean(true);
	if (rows == 1) {
		// Do the comparison. First hash the password we got sent using SHA-512.
		string pwHash;
		try {
			Crypto::DigestEngine engine("SHA512");
			engine.update(password);
			//std::vector<unsigned char> digest = engine.digest();
			//pwHash = string(digest.begin(), digest.end());
			const DigestEngine::Digest &digest = engine.digest();
			pwHash = DigestEngine::digestToHex(digest);
		}
		catch (Poco::Exception &e) {
			// Log error and abort.
			returnMsg->setException(500, "Internal server error. Message digest not found.");
			return returnMsg;
		}
		
		if (pwHash != passwordHash) {
			result->setValue(false);
		}
		else {
			// Set 'authenticated flag for this session.
			sessions.insert(std::pair<UInt32, bool>(session, true));
		}
	}
	else if (rows > 1) {
		// Corrupted database or data issue. Log and return false.
		cerr << "Error: Found more than one result for username: " << username << endl;
		result->setValue(false);
	}
	else {
		// Username not found. Return false.
		cerr << "Info: username " << username << " not found." << endl;
		result->setValue(false);
	}
	
	returnMsg->setResultValue(result);
	
	return returnMsg;
}

// --- COLLECTIONS CALLBACK ---
NymphMessage* collectionsCallback(int session, NymphMessage* msg, void* data) {
	cout << "Received message for session: " << session << ", msg ID: " << msg->getMessageId() << endl;
	
	// Check that this session has been authenticated.
	NymphMessage* returnMsg = msg->getReplyMessage();
	map<UInt32, bool>::iterator it;
	it = sessions.find(session);
	if (it == sessions.end()) {
		// Return an exception.
		returnMsg->setException(401, "Unauthorised");
		return returnMsg;
	}
	
	// Obtain the list of collections registered with this FLR server.
	Data::Statement select(*sql_session);
	string name;
	select << "SELECT name FROM collections", into (name), range(0, 1);
	
	NymphArray* arr = new NymphArray;
	while (!select.done()) {
		select.execute();
		NymphString* str = new NymphString(name);
		arr->addValue(str);
	}
	
	returnMsg->setResultValue(arr);
	return returnMsg;
}


// --- COLLECTION CALLBACK ---
NymphMessage* collectionCallback(int session, NymphMessage* msg, void* data) {
	cout << "Received message for session: " << session << ", msg ID: " << msg->getMessageId() << endl;
	
	string collection = ((NymphString*) msg->parameters()[0])->getValue();
	
	// Check that this session has been authenticated.
	NymphMessage* returnMsg = msg->getReplyMessage();
	map<UInt32, bool>::iterator it;
	it = sessions.find(session);
	if (it == sessions.end()) {
		// Return an exception.
		returnMsg->setException(401, "Unauthorised");
		return returnMsg;
	}
	
	// Obtain the list of files in a specific collection.
	Data::Statement select(*sql_session);
	string path;
	select << "SELECT collection_paths.path FROM collection_paths INNER JOIN collections ON \
				collections.id = collection_paths.collection_id WHERE collections.name=?", 
				into (path),
				use (collection),
				range(0, 1);
	
	NymphArray* arr = new NymphArray;
	while (!select.done()) {
		select.execute();
		NymphString* str = new NymphString(path);
		arr->addValue(str);
	}
	
	returnMsg->setResultValue(arr);
	return returnMsg;
}

// --- FILE REVISIONS CALLBACK ---
NymphMessage* fileRevisionsCallback(int session, NymphMessage* msg, void* data) {
	cout << "Received message for session: " << session << ", msg ID: " << msg->getMessageId() << endl;
	
	string collection = ((NymphString*) msg->parameters()[0])->getValue();
	string path = ((NymphString*) msg->parameters()[1])->getValue();
	
	// Check that this session has been authenticated.
	NymphMessage* returnMsg = msg->getReplyMessage();
	map<UInt32, bool>::iterator it;
	it = sessions.find(session);
	if (it == sessions.end()) {
		// Return an exception.
		returnMsg->setException(401, "Unauthorised");
		return returnMsg;
	}
	
	// Open the specified file and read out the revisions.
	// The file will be underneath the root folder:
	// TODO: implement permissions.
	// <root>/collection/path
	FlrFile file;
	string filePath = collection + "/" + path;
	if (!file.openFlrFile(filePath)) {
		// Error opening file. Return exception.
		returnMsg->setException(500, "Internal server error. Failed to read FLR file.");
		return returnMsg;
	}
	
	std::vector<FlrFileMeta> metas = file.revisions();
	NymphArray* arr = new NymphArray();
	for (int i = 0; i < metas.size(); ++i) {
		NymphArray* subarr = new NymphArray();
		subarr->addValue(new NymphString(metas[i].filename));
		subarr->addValue(new NymphUint64(metas[i].timestamp));
		subarr->addValue(new NymphString(metas[i].username));
		subarr->addValue(new NymphString(metas[i].message));
		arr->addValue(subarr);
	}
	
	returnMsg->setResultValue(arr);
	return returnMsg;
}

// --- FILE CONTENTS CALLBACK ---
NymphMessage* fileContentsCallback(int session, NymphMessage* msg, void* data) {
	cout << "Received message for session: " << session << ", msg ID: " << msg->getMessageId() << endl;
	
	string collection = ((NymphString*) msg->parameters()[0])->getValue();
	string path = ((NymphString*) msg->parameters()[1])->getValue();
	UInt32 revision = ((NymphUint32*) msg->parameters()[2])->getValue();
	bool newest = ((NymphBoolean*) msg->parameters()[3])->getValue();
	
	// Check that this session has been authenticated.
	NymphMessage* returnMsg = msg->getReplyMessage();
	map<UInt32, bool>::iterator it;
	it = sessions.find(session);
	if (it == sessions.end()) {
		// Return an exception.
		returnMsg->setException(401, "Unauthorised");
		return returnMsg;
	}
	
	// Open the specified file and read out the specified revision.
	// The file will be underneath the root folder:
	// TODO: implement permissions.
	// <root>/collection/path
	FlrFile file;
	string filePath = collection + "/" + path;
	if (!file.openFlrFile(filePath)) {
		// Error opening file. Return exception.
		returnMsg->setException(500, "Internal server error. Failed to read FLR file.");
		return returnMsg;
	}
	
	string filename;
	string dataStr;
	if (newest) {
		FlrFileMeta meta;
		file.currentFile(dataStr, meta);
		filename = meta.filename;
	}
	else {
		FlrDelta delta;
		file.fileRevision(revision, delta);
		filename = delta.meta.filename;
		dataStr = delta.data;
	}
	
	NymphArray* arr = new NymphArray();
	arr->addValue(new NymphString(filename));
	arr->addValue(new NymphString(dataStr));
	
	returnMsg->setResultValue(arr);
	return returnMsg;
}

// --- CHECK OUT CALLBACK ---
NymphMessage* checkoutCallback(int session, NymphMessage* msg, void* data) {
	cout << "Received message for session: " << session << ", msg ID: " << msg->getMessageId() << endl;
	
	string collection = ((NymphString*) msg->parameters()[0])->getValue();
	string path = ((NymphString*) msg->parameters()[1])->getValue();
	
	// Check that this session has been authenticated.
	NymphMessage* returnMsg = msg->getReplyMessage();
	map<UInt32, bool>::iterator it;
	it = sessions.find(session);
	if (it == sessions.end()) {
		// Return an exception.
		returnMsg->setException(401, "Unauthorised");
		return returnMsg;
	}
	
	// Check that the specific file hasn't been checked out already.
	// Assuming permissions match, set as checked out.
	
	NymphBoolean* result = new NymphBoolean(true);
	returnMsg->setResultValue(result);
	return returnMsg;
}

// --- CHECK IN CALLBACK ---
NymphMessage* checkinCallback(int session, NymphMessage* msg, void* data) {
	cout << "Received message for session: " << session << ", msg ID: " << msg->getMessageId() << endl;
	
	string collection = ((NymphString*) msg->parameters()[0])->getValue();
	string path = ((NymphString*) msg->parameters()[1])->getValue();
	std::vector<NymphType*> revision = ((NymphArray*) msg->parameters()[2])->getValues();
	
	// Check that this session has been authenticated.
	NymphMessage* returnMsg = msg->getReplyMessage();
	map<UInt32, bool>::iterator it;
	it = sessions.find(session);
	if (it == sessions.end()) {
		// Return an exception.
		returnMsg->setException(401, "Unauthorised");
		return returnMsg;
	}
	
	// TODO: Check that the specific file has been checked out previously by this user.
	FlrFile file;
	string filePath = collection + "/" + path;
	if (!file.openFlrFile(filePath)) {
		// Error opening file. Return exception.
		returnMsg->setException(500, "Internal server error. Failed to read FLR file.");
		return returnMsg;
	}
	
	// Debug.
	cout << "Opened FLR file.\n";
	
	FlrFileMeta meta;
	Timestamp ts;
	meta.filename = ((NymphString*) revision[0])->getValue();
	meta.username = "admin"; // TODO: read in actual username.
	meta.message = ((NymphString*) revision[1])->getValue();
	meta.timestamp = (UInt64) ts.epochMicroseconds();
	if (!file.addFileData(((NymphString*) revision[2])->getValue(), meta)) {
		// Error saving revision. Return exception.
		returnMsg->setException(500, "Internal server error. Failed to save revision.");
		return returnMsg;
	}
	
	NymphBoolean* result = new NymphBoolean(true);
	returnMsg->setResultValue(result);
	return returnMsg;
}

// ---


// --- MAIN ---
int main(int argc, char* argv[]) {
	// Read in configuration.
	string configFile;
	if (argc > 1) { configFile = argv[1]; }
	else { configFile = "config.ini"; }
	
	long timeout;
	int port;
	try {
		AutoPtr<IniFileConfiguration> config(new IniFileConfiguration(configFile));	
		timeout = config->getInt("NymphRPC.timeout", 5000); // 5 seconds by default.
		port = config->getInt("NymphRPC.port", 4004);
	}
	catch (Poco::Exception & e) {
		cout << "Configuration file is missing. Please create it.\n";
		exit(0);
	}
	
	// Set up SQLite link and ensure we got valid tables.
	Data::SQLite::Connector::registerConnector();
	sql_session = new Poco::Data::Session("SQLite", "flr.db");
	
	(*sql_session) << "CREATE TABLE IF NOT EXISTS users (id INT UNIQUE, username TEXT, password TEXT, \
					first_name TEXT, last_name TEXT, public_key TEXT)", now;
					
	(*sql_session) << "CREATE TABLE IF NOT EXISTS collections (id INT UNIQUE, name TEXT, path TEXT)",
						now;
					
	(*sql_session) << "CREATE TABLE IF NOT EXISTS collection_paths (collection_id INT, path TEXT)",
						now;
					
					
	// Initialise the remote client instance.
	cout << "Initialising server..." << endl;
	NymphRemoteClient::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
	
	// Register NymphRPC methods.
	// These allow clients to:
	// * Authenticate.
	// * Obtain a list of collections.
	// * Obtain the contents of a collections.
	// * Obtain the revisions of a file.
	// * Obtain a read-only copy of a specific file-revision.
	// * Check-out a file.
	// * Check-in a file.
	cout << "Registering methods..." << endl;
	std::vector<NymphTypes> parameters;
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_STRING);
	NymphMethod authenticateFunction("authenticate", parameters, NYMPH_BOOL);
	authenticateFunction.setCallback(authenticateCallback);
	NymphRemoteClient::registerMethod("authenticate", authenticateFunction);
	
	parameters.clear();
	NymphMethod collectionsFunction("collections", parameters, NYMPH_ARRAY);
	collectionsFunction.setCallback(collectionsCallback);
	NymphRemoteClient::registerMethod("collections", collectionsFunction);
	
	parameters.push_back(NYMPH_STRING);
	NymphMethod collectionFunction("collection", parameters, NYMPH_ARRAY);
	collectionFunction.setCallback(collectionCallback);
	NymphRemoteClient::registerMethod("collection", collectionFunction);
	
	parameters.push_back(NYMPH_STRING);
	NymphMethod fileRevisionsFunction("fileRevisions", parameters, NYMPH_ARRAY);
	fileRevisionsFunction.setCallback(fileRevisionsCallback);
	NymphRemoteClient::registerMethod("fileRevisions", fileRevisionsFunction);
	
	parameters.push_back(NYMPH_UINT32);
	parameters.push_back(NYMPH_BOOL);
	NymphMethod fileContentsFunction("fileContents", parameters, NYMPH_ARRAY);
	fileContentsFunction.setCallback(fileContentsCallback);
	NymphRemoteClient::registerMethod("fileContents", fileContentsFunction);
	
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_STRING);
	NymphMethod checkoutFunction("checkout", parameters, NYMPH_BOOL);
	checkoutFunction.setCallback(checkoutCallback);
	NymphRemoteClient::registerMethod("checkout", checkoutFunction);
	
	parameters.clear();
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_STRING);
	parameters.push_back(NYMPH_ARRAY);
	NymphMethod checkinFunction("checkin", parameters, NYMPH_BOOL);
	checkinFunction.setCallback(checkinCallback);
	NymphRemoteClient::registerMethod("checkin", checkinFunction);
	
	// Start NymphRPC server on port 4004 or the configured port.
	NymphRemoteClient::start(port);
	
	// Loop until the SIGINT signal has been received.
	//while (gSignalTrigger == 0) { }
	gMutex.lock();
	gCon.wait(gMutex);
	
	// Clean-up
	NymphRemoteClient::shutdown();
	
	// Wait before exiting, giving threads time to exit.
	Thread::sleep(2000); // 2 seconds.
	
	return 0;
}
