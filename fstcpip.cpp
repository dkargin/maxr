/***************************************************************************
 *      Mechanized Assault and Exploration Reloaded Projectfile            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "fstcpip.h"
#include "menu.h"
#include "log.h"

cFSTcpIp::cFSTcpIp ( bool server )
{
	this->bServer=server;
	iNum_clients=0;
	iMax_clients=16;
	iPlayerId=-1;
	SocketSet=NULL;
	sock_server=NULL;
	NetBuffer = new sNetBuffer;
	for ( int i=0;i<8;i++ )
	{
		sock_client[i]=NULL;
	}
	iStatus=STAT_CLOSED;
	UsedIDs = new sIDList();
	WaitOKList = new sIDList();
	if(bServer)
	{
		iNextMessageID = 1;
	}
	else
	{
		iNextMessageID = -1;
	}
	bReceiveThreadFinished = true;
	FSTcpIpReceiveThread = NULL;
}

cFSTcpIp::~cFSTcpIp()
{
	FSTcpIpClose();
}


void cFSTcpIp::FSTcpIpClose()
{
	if ( FSTcpIpReceiveThread ) SDL_KillThread( FSTcpIpReceiveThread );
	if ( SocketSet ) SDLNet_FreeSocketSet ( SocketSet );
	if ( sock_server ) SDLNet_TCP_Close ( sock_server );
	for ( int i=0;i<8;i++ )
	{
		if ( sock_client[i] ) SDLNet_TCP_Close ( sock_client[i] );
	}
	iStatus=STAT_CLOSED;
	FSTcpIpReceiveThread = NULL;
}

bool cFSTcpIp::FSTcpIpCreate()
{
	// Host
	if ( bServer )
	{
		if ( SDLNet_ResolveHost ( &addr,NULL,iPort ) <0 ) // No address for server
		{
			return false;
		}
	}
	// Client
	else
	{
		if ( SDLNet_ResolveHost ( &addr,sIp.c_str(),iPort ) <0 ) // Get server at address
		{
			return false;
		}
	}
	return true;
}

bool cFSTcpIp::FSTcpIpOpen ( void )
{
	FSTcpIpCreate();
	// Host
	if ( bServer )
	{
		sock_server = SDLNet_TCP_Open ( &addr ); // open socket
		if ( sock_server == NULL )
		{
			return false;
		}
		iStatus = STAT_OPENED;
		while ( iNum_clients < iMax_clients ) // Wait for more clients
		{
			if ( iNum_clients > 0 )
				FSTcpIpReceive(); // If there is minimal one client: look for messages
			else
				SDL_Delay ( 1000 ); // else wait a second
			sock_client[iNum_clients] = SDLNet_TCP_Accept ( sock_server ); // look for new client
			if ( sock_client[iNum_clients] != NULL ) // New client has connected
			{
				iNum_clients++;
				SocketSet = SDLNet_AllocSocketSet ( iNum_clients ); // Alloc socket-set for new client
				for ( int i=0;i<=iNum_clients;i++ )
				{
					SDLNet_TCP_AddSocket ( SocketSet, sock_client[i] ); // Add clients to the socket-set
				}
				// Send next message ID to new client and generate a new one for next message
				SendNewID(iNextMessageID, iNum_clients-1);
				UsedIDs->Add ( iNextMessageID );
				iNextMessageID = GenerateNewID();
			}
		}
		iStatus=STAT_CONNECTED;
	}
	// Client
	else
	{
		sock_server = SDLNet_TCP_Open ( &addr ); // open socket
		if ( sock_server == NULL )
		{
			return false;
		}
		FSTcpIpReceive(); // Wait for new id
		iStatus=STAT_CONNECTED;
	}
	return true;
}

bool cFSTcpIp::FSTcpIpSend ( int typ,const char *msg)
{
	cNetMessage NetMessage;
	int iPartLenght;
	int iPartNum = 1;
	int iMax_PartNum;
	// Send Message parts
	iMax_PartNum = ( int )(( float ) ( strlen(msg) + 1 ) / 256) + 1; // How many parts to send?
	while(iPartNum <= iMax_PartNum)
	{
		// Set the actual message part
		if(iPartNum < iMax_PartNum)
		{
			iPartLenght = 256;
		}
		else
		{
			iPartLenght = ( int ) ( strlen(msg) - ( ( iMax_PartNum - 1 ) * 255 ) + 1 );
		}
		NetMessage.typ = typ;
		NetMessage.lenght = iPartLenght;
		memcpy ( NetMessage.msg, msg,iPartLenght );
		NetMessage.msg[iPartLenght-1]='\0';

		// Set the buffer
		NetBuffer->iID = iNextMessageID;
		UsedIDs->Add ( iNextMessageID );
		iNextMessageID = GenerateNewID();
		NetBuffer->msg = NetMessage;
		NetBuffer->iMax_parts = iMax_PartNum;
		NetBuffer->iPart = iPartNum;
		NetBuffer->iTyp = BUFF_TYP_DATA;

		// Host
		int i = 0;
		if ( bServer )
		{
			while ( sock_client[i] != NULL && i < iMax_clients )
			{
				SDLNet_TCP_Send ( sock_client[i], NetBuffer, sizeof ( sNetBuffer ) );
				char szTmp[400];
				sprintf(szTmp,"(Host)Send Data-Message: -Part: %d/%d -ID: %d -Client %d -Message: \"%s\" -Lenght: %d -Typ: %d",NetBuffer->iPart, NetBuffer->iMax_parts, NetBuffer->iID, i, NetBuffer->msg.msg, NetBuffer->msg.lenght, NetBuffer->msg.typ);
				cLog::write(szTmp, LOG_TYPE_NETWORK);
				SDL_Delay ( 1 );
				i++;
			}
		}
		// Client
		else
		{
			SDLNet_TCP_Send ( sock_server, NetBuffer, sizeof ( sNetBuffer ) );
			char szTmp[400];
			sprintf(szTmp,"(Client)Send Data-Message: -Part: %d/%d -ID: %d -Message: \"%s\" -Lenght: %d -Typ: %d",NetBuffer->iPart, NetBuffer->iMax_parts, NetBuffer->iID, NetBuffer->msg.msg, NetBuffer->msg.lenght, NetBuffer->msg.typ);
			cLog::write(szTmp, LOG_TYPE_NETWORK);
			SDL_Delay ( 1 );
		}
		// Add package to waitlist
		WaitOKList->Add(NetBuffer->iID);

		iPartNum++;
	}
	return true;
}

bool cFSTcpIp::FSTcpIpReceive()
{
	// Host
	int i = 0;
	if ( bServer )
	{
		SDLNet_CheckSockets ( SocketSet, 1000 * 1 );
		while ( i < iNum_clients )
		{
			if ( SDLNet_SocketReady ( sock_client[i] ) )
			{
				SDLNet_TCP_Recv ( sock_client[i], NetBuffer, sizeof ( sNetBuffer ) );
				// is a new messages
				if ( NetBuffer->iTyp == BUFF_TYP_DATA )
				{
					char szTmp[400];
					sprintf(szTmp,"(Host)Received Data-Message: -Part: %d/%d -ID: %d -Client %d -Message: \"%s\" -Lenght: %d -Typ: %d", NetBuffer->iPart, NetBuffer->iMax_parts, NetBuffer->iID, i, NetBuffer->msg.msg, NetBuffer->msg.lenght, NetBuffer->msg.typ);
					cLog::write(szTmp, LOG_TYPE_NETWORK);
					if ( NetBuffer->msg.typ == 107 )
						int h = 0;
					// Add message to list
					MultiPlayer->MessageList->AddNetMessage ( &NetBuffer->msg );
					// Send OK to Client
					SendOK( NetBuffer->iID, i );
					// Send New ID to Client
					SendNewID(GenerateNewID(), i);
				}
				// if an OK that messages has been reveived
				else if( NetBuffer->iTyp == BUFF_TYP_OK )
				{
					char szTmp[64];
					sprintf(szTmp,"(Host)Received OK-Message: -ID: %d -Client: %d", NetBuffer->iID, i);
					cLog::write(szTmp, LOG_TYPE_NETWORK);
					// Delete Message ID from WaitList
					for (int k = 0; k < WaitOKList->iCount; k++)
					{
						if( WaitOKList->iID[k] == NetBuffer->iID )
						{
							WaitOKList->Delete(k);
							break;
						}
					}
					// Delete Message ID from used IDs
					for (int k = 0; k < UsedIDs->iCount; k++)
					{
						if( UsedIDs->iID[k] == NetBuffer->iID )
						{
							UsedIDs->Delete(k);
							break;
						}
					}
				}
				SDL_Delay ( 1 );
			}
			i++;
		}
	}
	// Client
	else
	{
		SDLNet_TCP_Recv ( sock_server, NetBuffer, sizeof ( sNetBuffer ) );
		if ( NetBuffer->iTyp == BUFF_TYP_DATA )
		{
			char szTmp[400];
			sprintf(szTmp,"(Client)Received Data-Message: -Part: %d/%d -ID: %d -Message: \"%s\" -Lenght: %d -Typ: %d", NetBuffer->iPart, NetBuffer->iMax_parts, NetBuffer->iID, NetBuffer->msg.msg, NetBuffer->msg.lenght, NetBuffer->msg.typ);
			cLog::write(szTmp, LOG_TYPE_NETWORK);
			// Add message to list
			MultiPlayer->MessageList->AddNetMessage ( &NetBuffer->msg );
			// Send OK to Server
			SendOK( NetBuffer->iID, -1 );
		}
		// if an OK that messages has been reveived
		else if( NetBuffer->iTyp == BUFF_TYP_OK )
		{
			char szTmp[64];
			sprintf(szTmp,"(Client)Received OK-Message: -ID: %d",NetBuffer->iID);
			cLog::write(szTmp, LOG_TYPE_NETWORK);
			// Delete Message ID from WaitList
			for (int k = 0; k < WaitOKList->iCount; k++)
			{
				if( WaitOKList->iID[k] == NetBuffer->iID )
				{
					WaitOKList->Delete(k);
					break;
				}
			}
		}
		// if a newid has been received
		else if( NetBuffer->iTyp == BUFF_TYP_NEWID )
		{
			char szTmp[400];
			sprintf(szTmp,"(Client)Received NewID-Message: -ID %d -Message: \"%s\" -Lenght: %d -Typ: %d",NetBuffer->iID, NetBuffer->msg.msg, NetBuffer->msg.lenght, NetBuffer->msg.typ);
			cLog::write(szTmp, LOG_TYPE_NETWORK);
			iNextMessageID = atoi ( (char *) NetBuffer->msg.msg );
			SendOK( NetBuffer->iID, -1 );
		}
	}
	if ( !bServer || iStatus==STAT_CONNECTED )
		bReceiveThreadFinished = true;
	return true;
}

int cFSTcpIp::GenerateNewID()
{
	int iID = 1;
	for(int i = 0; i < UsedIDs->iCount; i++)
	{
		if( iID == UsedIDs->iID[i] )
		{
			iID++;
			i = 0;
		}
	}
	return iID;
}

void cFSTcpIp::SendNewID(unsigned int iNewID, int iClientNum)
{
	cNetMessage *msg;
	char szTmp[12];

	msg = new cNetMessage();
	NetBuffer->iID = iNextMessageID;
	UsedIDs->Add ( iNextMessageID );
	iNextMessageID = GenerateNewID();
	NetBuffer->iMax_parts = 1;
	NetBuffer->iPart = 1;
	NetBuffer->iTyp = BUFF_TYP_NEWID;
	sprintf(szTmp,"%d", iNewID);
	msg->lenght = ( int ) strlen(szTmp) + 1;
	memcpy ( msg->msg, szTmp, msg->lenght );
	NetBuffer->msg = *msg;

	SDLNet_TCP_Send ( sock_client[iClientNum], NetBuffer, sizeof ( sNetBuffer ) );
	char szTmp2[400];
	sprintf(szTmp2,"(Host)Send NewID-Message: -ID %d -Client %d -Message: \"%s\" -Lenght: %d",NetBuffer->iID, iClientNum, NetBuffer->msg.msg, NetBuffer->msg.lenght);
	cLog::write(szTmp2, LOG_TYPE_NETWORK);
	// Add package to waitlist
	WaitOKList->Add(NetBuffer->iID);
	SDL_Delay ( 1 );
}

void cFSTcpIp::SendOK(unsigned int iID, int iClientNum)
{
	NetBuffer->iID = iID;
	NetBuffer->iMax_parts = 1;
	NetBuffer->iPart = 1;
	NetBuffer->iTyp = BUFF_TYP_OK;

	// Client
	if(iClientNum == -1)
	{
		SDLNet_TCP_Send ( sock_server, NetBuffer, sizeof ( sNetBuffer ) );
		char szTmp[256];
		sprintf(szTmp,"(Client)Send OK-Message: -ID: %d",NetBuffer->iID);
		cLog::write(szTmp, LOG_TYPE_NETWORK);
		SDL_Delay ( 1 );
	}
	// Host
	else
	{
		SDLNet_TCP_Send ( sock_client[iClientNum], NetBuffer, sizeof ( sNetBuffer ) );
		char szTmp[256];
		sprintf(szTmp,"(Host)Send OK-Message: -ID: %d -Client: %d",NetBuffer->iID,iClientNum);
		cLog::write(szTmp, LOG_TYPE_NETWORK);
		SDL_Delay ( 1 );
	}
	// Delete Message ID from used IDs if this is server
	if( bServer ){
		for (int k = 0; k < UsedIDs->iCount; k++)
		{
			if( UsedIDs->iID[k] == iID )
			{
				UsedIDs->Delete(k);
				break;
			}
		}
	}
	return ;
}

void cFSTcpIp::SetTcpIpPort ( int iPort )
{
	this->iPort=iPort;
}

void cFSTcpIp::SetIp ( string sIp )
{
	this->sIp=sIp;
}

int cFSTcpIp::GetConnectionCount()
{
	return iNum_clients;
}

int Receive ( void * )
{
	MultiPlayer->fstcpip->bReceiveThreadFinished = false;
	MultiPlayer->fstcpip->FSTcpIpReceive();
	MultiPlayer->fstcpip->bReceiveThreadFinished = true;
	return 1;
}

int Open ( void * )
{
	MultiPlayer->fstcpip->bReceiveThreadFinished = false;
	if ( MultiPlayer->fstcpip->FSTcpIpOpen() )
	{
		MultiPlayer->fstcpip->bReceiveThreadFinished = true;
		return 1;
	}
	else
	{
		MultiPlayer->fstcpip->bReceiveThreadFinished = true;
		return 0;
	}
}
/*
void cFSTcpIp::FSTcpIpCheckResends ()
{
	for(int i = 0; WaitOKList->iCount > i; i++) // Check all Waiting Buffers
	{
		if(WaitOKList->Items[i]) // sanity check
		{
			int iTime = ( ( sNetBuffer *) WaitOKList->Items[i])->iTicks; // Get the Time since this buffer was send the last time
			if( ( iTime - SDL_GetTicks() ) > 100 )
			{
				int iClient = ( ( sNetBuffer *) WaitOKList->Items[i])->iDestClientNum; // To which client should the buffer be send
				if(iClient != -1) // To a Client
				{
					SDLNet_TCP_Send ( sock_client[iClient],  WaitOKList->Items[i], sizeof ( sNetBuffer ) );
				}
				else // To the Host
				{
					SDLNet_TCP_Send ( sock_server,  WaitOKList->Items[i], sizeof ( sNetBuffer ) );
				}
				( ( sNetBuffer *) WaitOKList->Items[i])->iTicks = SDL_GetTicks(); // Set the new Time
				char szTmp[400];
				sprintf(szTmp,"Resend buffer: -Typ: %d -Parts: %d/%d -ID: %d -Client: %d -Message: \"%s\" -Lenght: %d -Typ: %d", NetBuffer->iPart, NetBuffer->iMax_parts , NetBuffer->iTyp, NetBuffer->iID, iClient, NetBuffer->msg.msg, NetBuffer->msg.lenght, NetBuffer->msg.typ);
				cLog::write(szTmp, LOG_TYPE_NETWORK);
			}
			SDL_Delay ( 1 ) ;
		}
	}
	return ;
}*/
