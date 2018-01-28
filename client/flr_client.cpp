/*
	main.cpp - FLR client CLI app main file.
	
	Revision 0
	
	Features:
			- 
			
	Notes:
			- 
			
	2016/10/02, Maya Posch
	(c) Nyanko.ws
*/

#include <nymph.h>

#include <Poco/Util/Application.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Util/OptionCallback.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/File.h>

using namespace Poco;
using namespace Poco::Util;


#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

//#include <flr_client.h>

// #define POCO_WIN32_UTF8


// --- READ FILE CONTENTS ---
string readFileContents(fstream &in) {
    return static_cast<stringstream const&>(stringstream() << in.rdbuf()).str();
}


// --- LOG FUNCTION ---
void logFunction(int level, string logStr) {
	cout << level << " - " << logStr << endl;
}


// --- FLR MAIN ---
class FlrMain : public Poco::Util::Application {
	bool _helpRequested;
	
public:
	FlrMain() {
		setUnixOptions(true);
		_helpRequested = false;
	}

protected:
    void initialize(Application& application){
        this->loadConfiguration();
        Application::initialize(application);
    }

    
	void uninitialize(){
        Application::uninitialize();
    }
	
	
	void reinitialize(Application &self) {
		Application::reinitialize(self);
	}
	

    void defineOptions(OptionSet& optionSet) {
        this->config().setString("server", "localhost");
		this->config().setString("port", "4004");
        Application::defineOptions(optionSet);

		optionSet.addOption(
			Option("help", "h", "Displays help details")
			.required(false)
			.repeatable(false)
			.callback(OptionCallback<FlrMain>(this, &FlrMain::handleHelp)));
		
        optionSet.addOption(
			Option("server", "s", "The FLR server's address")
			.required(false)
			.repeatable(false)
			.argument("<name or IP>", true)
			.callback(OptionCallback<FlrMain>(this, &FlrMain::handleOpt))
        );
		
		optionSet.addOption(
			Option("port" , "p", "The FLR server's port")
			.required(false)
			.repeatable(false)
			.argument("<integer>", true)
			.callback(OptionCallback<FlrMain>(this, &FlrMain::handleOpt))
		);
    }
	
	
	void handleHelp(const std::string& name, const std::string& value) {
		_helpRequested = true;
        HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader(
			"FLR client application.\n(c) Nyanko.ws - Maya Posch");
		helpFormatter.format(cout);
		stopOptionsProcessing();
    }
	

    void handleOpt(const std::string &name, const string &value) {
        cout << "Setting option " << name << " to " << value << endl;
        this->config().setString(name, value);
        cout << "The option is now " << this->config().getString(name) << endl;
    }

    int main(const vector<string> &arguments) {
        //std::cout << "We are now in main. Option is " << this->config().getString("optionval") << endl;
		if (_helpRequested) { return Application::EXIT_OK; }
		
		// Initialise the remote client instance.
		long timeout = 5000; // 5 seconds.
		NymphRemoteServer::init(logFunction, NYMPH_LOG_LEVEL_TRACE, timeout);
		
		// Connect to the remote server.
		int handle;
		string result;
		if (!NymphRemoteServer::connect("localhost", 4004, handle, 0, result)) {
			cout << "Connecting to remote server failed: " << result << endl;
			NymphRemoteServer::disconnect(handle, result);
			NymphRemoteServer::shutdown();
			return 1;
		}
		
		// Wait for commands to execute.
		string cmd;
		cout << "FLR client\nVersion 0.1-Alpha\n(c) Maya Posch\n\nUse 'help' for assistance.\n\n";
		string collection;
		while (1) {
			cout << "> ";
			cin >> cmd;
			
			if (cmd == "help") {
				cout << endl << "Commands:\n\tauth <username> <password>\n\t";
				cout << "use <collection>\n\t";
				cout << "quit\n\t";
				cout << "colls\n\tcoll\n\t";
				cout << "filerevs <file>\n\t";
				cout << "filecontent <file>\n\t";
				cout << "checkout <file>\n\tcheckin <file> <commit message>\n\n";
			}
			else if (cmd == "use") {
				cin >> collection;
			}
			else if (cmd == "quit") {
				break;
			}
			else if (cmd == "auth") {
				cout << endl;
				
				// Call 'authentication' function.
				// Obtain username & password.
				// TODO: secure password entry.
				string user;
				string pass;
				cin >> user;
				cin >> pass;
				
				// Call remote method.
				vector<NymphType*> values;
				values.push_back(new NymphString(user));
				values.push_back(new NymphString(pass));
				NymphType* returnValue = 0;
				if (!NymphRemoteServer::callMethod(handle, "authenticate", values, returnValue, result)) {
					cout << "Error calling remote method: " << result << endl;
					continue;
					//NymphRemoteServer::disconnect(handle, result);
					//NymphRemoteServer::shutdown();
					//return 1;
				}
				
				// Check returnValue.
				if (!returnValue) {
					cout << "Error calling remote method: " << result << endl;
					continue;
				}
				
				if (returnValue->type() != NYMPH_BOOL) {
					cout << "Return value wasn't a boolean.\n";
					continue;
				}
				
				bool authRes = ((NymphBoolean*) returnValue)->getValue();
				if (authRes) {
					cout << "Authentication succeeded." << endl; 
				}
				else {
					cout << "Authentication failed." << endl;
				}
				
 				delete returnValue;
			}
			else if (cmd == "colls") {
				cout << endl;
				
				// List any available collections.
				// Call remote method.
				vector<NymphType*> values;
				NymphType* returnValue = 0;
				if (!NymphRemoteServer::callMethod(handle, "collections", values, returnValue, result)) {
					cout << "Error calling remote method: " << result << endl;
					continue;
				}
				
				// Check returnValue.
				if (!returnValue) {
					cout << "Error calling remote method: " << result << endl;
					continue;
				}
				
				if (returnValue->type() != NYMPH_ARRAY) {
					cout << "Return value wasn't an array.\n";
					continue;
				}
				
				// We should now have an array of NymphString instances.
				std::vector<NymphType*> resArr = ((NymphArray*) returnValue)->getValues();
				
				// Display each string on a separate line.
				for (int i = 0; i < resArr.size(); ++i) {
					cout << endl << i << ". ";
					if (resArr[i]->type() == NYMPH_STRING) {
						cout << ((NymphString*) resArr[i])->getValue() << endl;
					}
					else {
						cout << ">> Invalid response <<" << endl;
					}
				}
				
				delete returnValue;
			}
			else if (cmd == "coll") {
				cout << endl;
				if (collection.empty()) {
					cout << "Use 'use <collection>' to specify a collection first.";
					continue;
				}
				
				// Call remote method.
				vector<NymphType*> values;
				values.push_back(new NymphString(collection));
				NymphType* returnValue = 0;
				if (!NymphRemoteServer::callMethod(handle, "collection", values, returnValue, result)) {
					cout << "Error calling remote method: " << result << endl;
					NymphRemoteServer::disconnect(handle, result);
					NymphRemoteServer::shutdown();
					return 1;
				}
				
				// Check returnValue.
				if (!returnValue) {
					cout << endl << "Error calling remote method: " << result << endl;
					continue;
				}
				
				if (returnValue->type() != NYMPH_ARRAY) {
					cout << "Return value wasn't an array.\n";
					continue;
				}
				
				// We should now have an array of NymphString instances.
				std::vector<NymphType*> resArr = ((NymphArray*) returnValue)->getValues();
				
				// Display each string on a separate line.
				for (int i = 0; i < resArr.size(); ++i) {
					cout << endl << i << ". ";
					if (resArr[i]->type() == NYMPH_STRING) {
						cout << ((NymphString*) resArr[i])->getValue() << endl;
					}
					else {
						cout << ">> Invalid response <<" << endl;
					}
				}
				
				delete returnValue;
			}
			else if (cmd == "new") {
				cout << endl;
				
				// Create a new, empty collection.
			}
			else if (cmd == "filerevs") {
				cout << endl;
				if (collection.empty()) {
					cout << "Use 'use <collection>' to specify a collection first.";
					continue;
				}
				
				// Obtain path.
				string path;
				cin >> path;
				
				// Call remote method.
				vector<NymphType*> values;
				values.push_back(new NymphString(collection));
				values.push_back(new NymphString(path));
				NymphType* returnValue = 0;
				if (!NymphRemoteServer::callMethod(handle, "fileRevisions", values, returnValue, result)) {
					cout << "Error calling remote method: " << result << endl;
					continue;
				}
				
				// Check returnValue.
				if (!returnValue) {
					cout << endl << "Error calling remote method: " << result << endl;
					continue;
				}
				
				if (returnValue->type() != NYMPH_ARRAY) {
					cout << "Return value wasn't an array.\n";
					continue;
				}
				
				// We should now have an array of NymphArray instances
				std::vector<NymphType*> resArr = ((NymphArray*) returnValue)->getValues();
				
				// Display each string on a separate line.
				for (int i = 0; i < resArr.size(); ++i) {
					cout << endl << i << ". ";
					if (resArr[i]->type() == NYMPH_ARRAY) {
						std::vector<NymphType*> subArr = ((NymphArray*) resArr[i])->getValues();
						
						// TODO: validate array.
						// TODO: pretty formatting, limit line count.
						cout << ((NymphString*) subArr[0])->getValue() << "\t" << 
								((NymphUint64*) subArr[1])->getValue() << "\t" << 
								((NymphString*) subArr[2])->getValue() << "\t" << 
								((NymphString*) subArr[3])->getValue() << endl;
					}
					else {
						cout << ">> Invalid response <<" << endl;
					}
				}
				
				delete returnValue;
			}
			else if (cmd == "filecontent") {
				cout << endl;
				if (collection.empty()) {
					cout << "Use 'use <collection>' to specify a collection first.";
					continue;
				}
				
				// Obtain parameters
				string path;
				cin >> path;
				UInt32 revision;
				cin >> revision;
				string latest;
				cin >> latest;
				
				// Call remote method.
				vector<NymphType*> values;
				values.push_back(new NymphString(collection));
				values.push_back(new NymphString(path));
				values.push_back(new NymphUint32(0));
				values.push_back(new NymphBoolean(latest));
				NymphType* returnValue = 0;
				if (!NymphRemoteServer::callMethod(handle, "fileContents", values, returnValue, result)) {
					cout << "Error calling remote method: " << result << endl;
					continue;
				}
				
				// Check returnValue.
				if (!returnValue) {
					cout << endl << "Error calling remote method: " << result << endl;
					continue;
				}
				
				if (returnValue->type() != NYMPH_ARRAY) {
					cout << "Return value wasn't an array.\n";
					continue;
				}
				
				// We should now have an array with the filename (0) and data (1).
				std::vector<NymphType*> resArr = ((NymphArray*) returnValue)->getValues();
				
				// Save the file to disk.
				string savepath = collection + "/" + path;
				File file(savepath);
				file.createDirectories();
				fstream FILE;
				FILE.open(savepath, ios_base::binary | ios_base::out | ios_base::trunc);
				if (FILE.good()) {
					FILE.write(((NymphString*) resArr[1])->getValue().data(), ((NymphString*) resArr[1])->getValue().length());
				}
				
				FILE.close();
				
				delete returnValue;
			}
			else if (cmd == "checkout") {
				cout << endl;
				if (collection.empty()) {
					cout << "Use 'use <collection>' to specify a collection first.";
					continue;
				}
				
				// Get values.
				string path;
				cin >> path;
				
				// Call remote method.
				vector<NymphType*> values;
				values.push_back(new NymphString(collection));
				values.push_back(new NymphString(path));
				NymphType* returnValue = 0;
				if (!NymphRemoteServer::callMethod(handle, "checkout", values, returnValue, result)) {
					cout << "Error calling remote method: " << result << endl;
					continue;
				}
				
				// Check returnValue.
				if (!returnValue) {
					cout << "Error calling remote method: " << result << endl;
					continue;
				}
				
				if (returnValue->type() != NYMPH_BOOL) {
					cout << "Return value wasn't a boolean.\n";
					continue;
				}
				
				bool authRes = ((NymphBoolean*) returnValue)->getValue();
				if (authRes) {
					cout << "Check-out succeeded." << endl; 
				}
				else {
					cout << "Check-out failed." << endl;
				}
				
 				delete returnValue;
			}
			else if (cmd == "checkin") {
				cout << endl;
				if (collection.empty()) {
					cout << "Use 'use <collection>' to specify a collection first.";
					continue;
				}
				
				// Get values.
				string path;
				cin >> path;
				string msg;
				cin >> msg;
				
				// Read the file into an array.
				NymphArray* arr = new NymphArray();
				string savepath = collection + "/" + path;
				File file(savepath);
				fstream FILE;
				
				if (!file.exists()) { 
					cout << "Failed to open target file. File doesn't exist.\n\n"; 
					continue;
				}
				
				FILE.open(savepath, ios_base::binary | ios_base::in);
				if (!FILE.good()) { cout << "Failed to open target file.\n\n"; continue; }
				string data = readFileContents(FILE);
				FILE.close();
				
				size_t pos = path.find_last_of('/');
				string filename;
				if (pos == std::string::npos) { filename = path; }
				else { filename = path.substr(pos); }
				
				// TODO: perform a diff with the last revision here?
				arr->addValue(new NymphString(filename)); 	// Filename.
				arr->addValue(new NymphString(msg)); 		// Commit message.
				arr->addValue(new NymphString(data));		// File data.
				
				// Call remote method.
				vector<NymphType*> values;
				values.push_back(new NymphString(collection));
				values.push_back(new NymphString(path));
				values.push_back(arr);
				NymphType* returnValue = 0;
				if (!NymphRemoteServer::callMethod(handle, "checkin", values, returnValue, result)) {
					cout << "Error calling remote method: " << result << endl;
					continue;
				}
				
				// Check returnValue.
				if (!returnValue) {
					cout << "Error calling remote method: " << result << endl;
					continue;
				}
				
				if (returnValue->type() != NYMPH_BOOL) {
					cout << "Return value wasn't a boolean.\n";
					continue;
				}
				
				bool authRes = ((NymphBoolean*) returnValue)->getValue();
				if (authRes) {
					cout << "Check-in succeeded." << endl; 
				}
				else {
					cout << "Check-in failed." << endl;
				}
				
 				delete returnValue;
			}
			else {
				cout << endl << "Unknown command.\n";
			}
		}
		
		
		// Shutdown.
		NymphRemoteServer::disconnect(handle, result);
		NymphRemoteServer::shutdown();
		return 0;
    }
};

POCO_APP_MAIN(FlrMain)
