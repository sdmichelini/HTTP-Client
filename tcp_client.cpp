/*
 * tcp_client.cpp
 *	Implementation of a TCP Client
 *  Created on: Nov 4, 2014
 *      Author: sdmichelini
 */
#include "tcp_client.h"
#include <fstream>

tcp_client::tcp_client()
{
	///Init all variables
	///Socket is zero because we have none
	lSocket = 0;
	///We are not connected
	bConnected = false;
}

bool tcp_client::connectToServer(char * remoteIP, unsigned short port)
{
	//Get the IP of the remote server
	//Using example from P and D
	//Pointer to our hostnet
	struct hostent * pHostnet;
	//Remote Socket Info
	struct sockaddr_in sSocketInfo;
	//Resolve the host name
	pHostnet = gethostbyname(remoteIP);
	//Error if we fail to find an IP for host name
	if(!pHostnet)
	{
		std::cout<<"Error: Could not resolve host name: "<<remoteIP<<std::endl;
		return false;
	}
	//Zero out the socket memory
	//Not using bzero here because it is unsupported in some BSD implementations such as Cygwin
	memset(&sSocketInfo,0,sizeof(sSocketInfo));

	//Now copy over socket information
	memcpy((char*)&sSocketInfo.sin_addr, pHostnet->h_addr, pHostnet->h_length);
	//IPv4
	sSocketInfo.sin_family = AF_INET;
	//Convert from little endian to big endian for network
	sSocketInfo.sin_port = htons(port);
	//Create the socket
	if((lSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		std::cout<<"Error: Could not create socket."<<std::endl;
		return false;
	}

	//Connect to the remote host
	if(connect(lSocket,(struct sockaddr *)&sSocketInfo,sizeof(sSocketInfo)) < 0)
	{

		std::cout<<"Error: Could not connect to IP:"<<remoteIP<<" on port: "<<port<<std::endl;
		return false;
	}
	//If we get here It must be connected
	bConnected = true;
	return true;
}

void tcp_client::closeSocket()
{
	//Don't call close twice
	if(bConnected)
	{
		bConnected = false;
		close(lSocket);
		lSocket = 0;
	}
}

bool tcp_client::writeToSocket(char * message, unsigned long size)
{

	//Don't send anything if the size is zero or we are not connected
	if(size==0||!bConnected)
	{
		std::cout<<"Error: Message size is 0 or socket not connected"<<std::endl;
		return false;
	}
	//Will return the bytes sent over the network
	size_t bytesSent = send(lSocket, message, size, 0);
	//Check we sent proper amount of bytes
	if(bytesSent!=size)
	{
#ifdef TCP_CLIENT_DEBUG
		std::cout<<"Error: Writing to Socket"<<std::endl;
#endif
		return false;
	}
	return true;

}

long tcp_client::readFromSocket(char * array, unsigned long maxSize)
{
	//Don't perform a read if we are not connected or we are requesting a message of 0 bytes
	if(maxSize==0||!bConnected)
	{
		return false;
	}
	//Limit to MAX_BUFFER_SIZE
	if(maxSize > MAX_BUFFER_SIZE)
	{
		maxSize = MAX_BUFFER_SIZE;
	}
	//Read from Socket
	long rc = recv(lSocket, array, maxSize, 0);

	//Error on socket
	if(rc < 0)
	{
		std::cout<<"Socket Error on Read"<<std::endl;
	}
	//No data received
	else if(rc == 0)
	{
#ifdef TCP_CLIENT_DEBUG
		std::cout<<"No Data Received from Socket"<<std::endl;
#endif
	}
	return rc;
}

tcp_client::~tcp_client()
{
	//Clean-Up
	closeSocket();
}



