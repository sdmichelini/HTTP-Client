//============================================================================
// Name        : webclient.cpp
// Author      : Stephen Michelini
// Version     :
// Copyright   : 
// Description : Hello World in C, Ansi-style
//============================================================================

#include "tcp_client.h"
//For gettimeofday
#include <sys/time.h>
//For tcp_info
#include <netinet/tcp.h>
//For string operations
#include <string>
//STL container for packet data
#include <vector>
//File IO
#include <fstream>
///Default HTTP Port
#define DEFAULT_HTTP_PORT 80

//HTTPS port not supported
#define HTTPS_PORT 443

//Uncomment for Debug Statements
//#define N_DEBUG

////Type of output wanted from the web_client
enum WebClientOutputType{
	///Output to a file
	kFile,
	///Output to the console
	kConsole,
	///No Output from HTTP Request
	kNone
};

/*!
 * A simple data structure to hold information for a packet such as read time and how many bytes it is
 */
struct packet_stats{
	///Size in bytes of the read
	long size;
	///Time in milliseconds of the read
	unsigned long millis;
};

class web_client: public tcp_client{
public:
	web_client();
	/*!
	 * Performs an HTTP GET request to the remote host on the port
	 *
	 * @return
	 * 	True on success, false on error
	 */
	bool performHttpRequest(std::string remoteHost, unsigned short port = DEFAULT_HTTP_PORT);

	/*!
	 * Set's which type of output is wanted from an HTTP Request
	 */
	void setRequestOutput(WebClientOutputType t){
		wOutput = t;
	}
	/*!
	 * Set's the filename of the log file
	 */
	void setResponseFile(std::string filename){
		sResponseFile = filename;
	}
	/*!
	 * Set's whether or not packet statistics should be enabled
	 */
	void setCollectPacketStats(bool wantStats){
		bCollectPacketStats = wantStats;
	}

	/*!
	 * Set's whether or not the Linux tcp_info should be obtained
	 */
	void setCollectTcpInfo(bool wantInfo){
		bCollectTcpInfo = wantInfo;
	}

	/*!
	 * Get's the configured output type for the request
	 */
	WebClientOutputType getConfiguredOutputType(){
		return wOutput;
	}

	/*!
	 * Print out the packet statistics
	 */
	void printPacketStatisitics();

	/*!
	 * Get's the last ping time to a Server
	 * Must be called after a call to a server
	 * @return
	 * 	Time to connect to server in milliseconds
	 */
	unsigned long getPingMillis();
private:
	///Packet Read Time
	struct timeval tPacketReadTime;
	///Start Connect Time
	struct timeval tStartConnectTime;
	///Finish Connect Time
	struct timeval tFinishConnectTime;
	///What type of output is wanted
	WebClientOutputType wOutput;
	///File name for Responses
	std::string sResponseFile;
	///Hold's packet information
	std::vector<packet_stats> aPacketStats;
	///Boolean to collect packet stats
	bool bCollectPacketStats;
	///Boolean to collect TCP Information
	bool bCollectTcpInfo;

};

int main(int argc, char * argv[]) {
	//Error if not enough arguments
	if(argc < 2){
		std::cout<<"Error: Not Enough Arguments"<<std::endl;
		std::cout<<"Usage: ./webclient server/path(o) (any one of options (-f filename) -nf(not with -f) -pkt -info -ping)"<<std::endl;
		return 1;
	}
	//Make a web client
	web_client * client;
	client = new web_client();
	//Assume not printing ping stats unless changed
	bool printPing = false;
	//Assume not printing packet stats
	bool printPacketStats = false;
	//Loop through options
	for(int i = 2; i < argc; i++){
		//Check for file options
		if(strcmp(argv[i],"-f")==0){
			//error if two mutually exclusive options are attempted to be enabled
			if(client->getConfiguredOutputType() == kConsole){
				std::cout<<"Error: -nf and -f are mutually exclusive"<<std::endl;
				return 1;
			}

			//Next param should be filename
			if(argc >= i+2){
				client->setRequestOutput(kFile);
				client->setResponseFile(std::string(argv[i+1]));
				i += 2;
			}
			else{
				std::cout<<"-f Command Invalid: No Filename Specified to Write To."<<std::endl;
			}
		}
		//Check for console output
		else if(strcmp(argv[i],"-nf") == 0){
			//error if two mutually exclusive options are attempted to be enabled
			if(client->getConfiguredOutputType() == kFile){
				std::cout<<"Error: -nf and -f are mutually exclusive"<<std::endl;
				return 1;
			}
			client->setRequestOutput(kConsole);
		}
		//Check for packet stats
		else if(strcmp(argv[i],"-pkt") == 0){
			//error if two mutually exclusive options are attempted to be enabled
			if(client->getConfiguredOutputType() == kFile){
				std::cout<<"Error: -nf and -f are mutually exclusive"<<std::endl;
				return 1;
			}
			client->setRequestOutput(kConsole);
			client->setCollectPacketStats(true);
			printPacketStats = true;
		}
		//Check for ping stats
		else if(strcmp(argv[i],"-ping") == 0){
			printPing = true;
		}
		//Check for tcp_info
		else if(strcmp(argv[i],"-info")==0){
			client->setCollectTcpInfo(true);
		}
	}
	//Url
	std::string URL = std::string(argv[1]);
	//Remove HTTP
	if(URL.find("http://") != std::string::npos){
		URL = URL.substr(URL.find("http://") + 7);
	}
	//No HTTPS
	if(URL.find("https://") != std::string::npos){
		std::cout<<"Error: HTTPS is not supported in this application."<<std::endl;
		delete client;
		return 1;
	}
	//Perform Request or Die
	if(!client->performHttpRequest(URL)){
		std::cout<<"WebClient Failure. Exiting."<<std::endl;
		delete client;
		return 1;
	}
	//Print packet stats if needed
	if(printPacketStats){
		client->printPacketStatisitics();
	}
	//Print ping if needed
	if(printPing){
		std::cout<<"Request Took: "<<client->getPingMillis()<<" milliseconds."<<std::endl;
	}
	//Debug statements
#ifdef N_DEBUG
	std::cout<<"Request Took: "<<client->getPingMillis()<<" milliseconds."<<std::endl;
#endif
	//Clean-up
	delete client;
	return 0;
}

web_client::web_client(){
	//Default options
	wOutput = kNone;
	sResponseFile = "";
	bCollectPacketStats = false;
	bCollectTcpInfo = false;
}

bool web_client::performHttpRequest(std::string remoteHost, unsigned short port){
#ifdef N_DEBUG
	std::cout<<"Performing HTTP GET request on: "<<remoteHost<<" on port: "<<port<<std::endl;
#endif
	//We don't support https
	if(port == HTTPS_PORT){
		std::cout<<"Error: HTTPS is not supported by this client."<<std::endl;
		return false;
	}
	//Get the Host
	//Find the /
	size_t pos = remoteHost.find('/');
	std::string host;
	std::string path;
	if(pos == std::string::npos){
		//No Path
		host = remoteHost;
		path = "";
	}
	else{
		host = remoteHost.substr(0, pos);
		path = remoteHost.substr(pos + 1);
	}
	//Get the start connect time
	gettimeofday(&tStartConnectTime,NULL);
	//Connect or error
	if(!this->connectToServer((char*)host.c_str(),port)){
		std::cout<<"WebClient Error: Connecting to Remote Host: "<<remoteHost<<" on port: "<<port<<std::endl;
		return false;
	}
	//End Connect Time
	gettimeofday(&tFinishConnectTime,NULL);



	//Now make the HTTP Request
	std::string request = "GET /"+path+" HTTP/1.0\r\n";
	//Add host line
	request += "Host: "+host+"\r\n\r\n";

#ifdef N_DEBUG
	std::cout<<request<<std::endl;
#endif
	//Send the HTTP request to server
	if(!this->writeToSocket((char*)request.c_str(),request.size())){
		//Error
		std::cout<<"WebClient Error: Writing HTTP Request"<<std::endl;
		this->closeSocket();
		return false;
	}
	//Hold's the page data
	std::string response;
	//Now Read back results
	char buf[MAX_BUFFER_SIZE];
	//Received count
	long rc=this->readFromSocket(buf,MAX_BUFFER_SIZE);
	while(rc > 0){
		//turn response to string
		buf[rc] = 0x00;
		response += std::string(buf);
		//Collect Packet Stats
		if(bCollectPacketStats){
			packet_stats p;
			p.size = rc;
			//Get the current time
			gettimeofday(&tPacketReadTime, NULL);
			//Base the time off of when we started connecting
			p.millis=((tPacketReadTime.tv_sec-tStartConnectTime.tv_sec) * (unsigned long)1000) + ((tPacketReadTime.tv_usec-tStartConnectTime.tv_usec) / 1000);
			//Add it to our stat container
			aPacketStats.push_back(p);
		}

		//Clear buffer before reading it
		memset(buf,0,sizeof(buf));
		rc = this->readFromSocket(buf,MAX_BUFFER_SIZE);
	}

	//Debug
#ifdef N_DEBUG
	std::cout<<response<<std::endl;
#else
	if(wOutput == kConsole){
		std::cout<<response<<std::endl;
	}
	else if(wOutput == kFile){
		//Output stream for file
		std::ofstream outputStream;
		//Open the output file stream and overwrite existing contents
		//File I/O sample used from http://www.cplusplus.com/reference/fstream/ofstream/open/
		outputStream.open(sResponseFile.c_str(), std::ofstream::out | std::ofstream::trunc);

		outputStream<<response<<"\n";
		//Close when finished
		outputStream.close();
	}
#endif
	//Collect TCP Info
	//Only works on Linux
	//Program built on Mac and tested then moved to Linux
#ifndef __APPLE__
	if(bCollectTcpInfo){
	  std::cout<<"collecting tcp_info"<<std::endl;
		struct tcp_info socketInfo;
		socklen_t tcp_info_length = sizeof(socketInfo);
		if(getsockopt(lSocket, SOL_TCP, TCP_INFO, (void *)&socketInfo, &tcp_info_length) < 0){
			std::cout<<"error getting tcp info\n";
		}
		else{
			std::cout<<"TCP Info RTT: "<<socketInfo.tcpi_rtt<<" RTT Variance: "<<socketInfo.tcpi_rttvar<<std::endl;
		}
	}
	
#endif
	//Clean-up
	this->closeSocket();
	return true;
}

unsigned long web_client::getPingMillis(){
	//Microseconds
	long micros = tFinishConnectTime.tv_usec - tStartConnectTime.tv_usec;
	//Seconds
	long seconds = tFinishConnectTime.tv_sec - tStartConnectTime.tv_sec;
	//Milliseconds
	unsigned long millis = (seconds * (unsigned long)1000) + (micros / 1000);
	return millis;
}

void web_client::printPacketStatisitics(){
	//Packet stats not enabled
	if(!bCollectPacketStats){
		std::cout<<"Error: printPacketStatistics(). Statistics not enabled please use the -pkt option"<<std::endl;
		return;
	}
	//No stats
	if(aPacketStats.size()==0){
		std::cout<<"Error: printPacketStatistics(). Did Server write back any data?"<<std::endl;
		return;
	}
	//Loop through displaying stats
	for(unsigned int i = 0; i < aPacketStats.size(); i++){
		std::cout<<"Packet "<< i + 1 <<" - Time: "<<aPacketStats[i].millis<<" Bytes: "<<aPacketStats[i].size<<std::endl;
	}
}
