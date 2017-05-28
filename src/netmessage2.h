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

#ifndef netmessage2H
#define netmessage2H

#include <memory>

#include "maxrconfig.h"
#include "utility/serialization/serialization.h"
#include "utility/serialization/textarchive.h"
#include "utility/serialization/binaryarchive.h"
#include "game/logic/gametimer.h"
#include "game/data/report/savedreport.h"
#include "ui/graphical/game/gameguistate.h"
#include "utility/color.h"
#include "maxrversion.h"
#include "mapdownload.h"
#include "game/data/player/playerbasicdata.h"

class cSavedReport;
class cSocket;

// When changing this enum, also update function enumToString(eNetMessageType value)!
enum class eNetMessageType {
	/** TCP messages generated by the connection manager:
	    Note: do not change the numerical values of the first 3 message, to make sure the game
		version check is working across different maxr versions */
	TCP_HELLO = 0,           /** sent from host to provide game version of the server (1st stage of the initial handshake) */
	TCP_WANT_CONNECT = 1,    /** sent from client to provide credentials (2nd stage of the initial handshake) */
	TCP_CONNECTED = 2,       /** sent from host, when the connection is accepted (3rd stage of the initial handshake) */
	TCP_CONNECT_FAILED,      /** local event, when server is not reachable. Sent from host, if connection is declined by host */
	TCP_CLOSE,               /** local event (generated by connection manager) to indicate a closed connection */

	ACTION,               /** the set of actions a client (AI or player) can trigger to influence the game */
	GAMETIME_SYNC_SERVER, /** sync message from server to clients */
	GAMETIME_SYNC_CLIENT, /** sync message from client to server */
	PLAYERSTATE, 
	RANDOM_SEED,          /** initialize the synchronized random generator of the models */
	REPORT,               /** chat messages and other reports for the player */
	GUI_SAVE_INFO,        /** saved reports and gui settings */
	REQUEST_GUI_SAVE_INFO,/** requests the clients to send their gui data for saving */
	RESYNC_MODEL,         /** transfer a copy of the complete modeldata to clients */
	REQUEST_RESYNC_MODEL, /** instructs the server to send a copy of the model */
	MULTIPLAYER_LOBBY,    /** messages for multiplayer lobby and game preparation menus */
	GAME_ALREADY_RUNNING, /** send by server, when a new connection is established, after the game has started */
	WANT_REJOIN_GAME,     /** send by a client to reconnect a disconnected player */
};
std::string enumToString(eNetMessageType value);

//------------------------------------------------------------------------------
class cNetMessage2
{
public:

	static std::unique_ptr<cNetMessage2> createFromBuffer(const unsigned char* data, int length);
	
	virtual ~cNetMessage2() {}

	eNetMessageType getType() const { return type; };
	std::unique_ptr<cNetMessage2> clone() const;

	virtual void serialize(cBinaryArchiveIn& archive) { serializeThis(archive); } 
	virtual void serialize(cTextArchiveIn& archive) { serializeThis(archive); }

	int playerNr;

protected:
	cNetMessage2(eNetMessageType type) : type(type), playerNr(-1) {};
private:
	template <typename T>
	void serializeThis(T& archive)
	{
		archive & type;
		archive & playerNr;
	}

	eNetMessageType type;
};

//------------------------------------------------------------------------------
class cNetMessageReport : public cNetMessage2 
{
public:
	cNetMessageReport(std::unique_ptr<cSavedReport> report) : 
		cNetMessage2(eNetMessageType::REPORT),
		report(std::move(report)) 
	{};
	cNetMessageReport() : 
		cNetMessage2(eNetMessageType::REPORT) 
	{};

	cNetMessageReport(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::REPORT)
	{
		loadThis(archive);
	}

	virtual void serialize(cBinaryArchiveIn& archive) { cNetMessage2::serialize(archive); saveThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cNetMessage2::serialize(archive); saveThis(archive); }

	std::unique_ptr<cSavedReport> report;

private:
	template<typename T>
	void loadThis(T& archive)
	{
		report = cSavedReport::createFrom(archive);
	}
	template<typename T>
	void saveThis(T& archive)
	{
		archive << *report;
	}
};

//------------------------------------------------------------------------------
class cNetMessageSyncServer : public cNetMessage2
{
public:
	cNetMessageSyncServer() : cNetMessage2(eNetMessageType::GAMETIME_SYNC_SERVER) {};
	cNetMessageSyncServer(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::GAMETIME_SYNC_SERVER)
	{
		serializeThis(archive);
	}

	virtual void serialize(cBinaryArchiveIn& archive) { cNetMessage2::serialize(archive); serializeThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cNetMessage2::serialize(archive); serializeThis(archive); }

	unsigned int gameTime;
	unsigned int checksum;
	unsigned int ping;

private:
	template<typename T>
	void serializeThis(T& archive)
	{
		archive & gameTime;
		archive & checksum;
		archive & ping;
	}
};

//------------------------------------------------------------------------------
class cNetMessageSyncClient : public cNetMessage2
{
public:
	cNetMessageSyncClient() : cNetMessage2(eNetMessageType::GAMETIME_SYNC_CLIENT) {};
	cNetMessageSyncClient(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::GAMETIME_SYNC_CLIENT)
	{
		serializeThis(archive);
	}

	virtual void serialize(cBinaryArchiveIn& archive) { cNetMessage2::serialize(archive); serializeThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cNetMessage2::serialize(archive); serializeThis(archive); }

	unsigned int gameTime;

	//send debug data to server
	bool crcOK;
	unsigned int timeBuffer;
	unsigned int ticksPerFrame;
	unsigned int queueSize;
	unsigned int eventCounter;

private:
	template<typename T>
	void serializeThis(T& archive)
	{
		archive & gameTime;
		archive & crcOK;
		archive & timeBuffer;
		archive & ticksPerFrame;
		archive & queueSize;
		archive & eventCounter;
	}
};

//------------------------------------------------------------------------------
class cNetMessageRandomSeed : public cNetMessage2
{
public:
	cNetMessageRandomSeed(uint64_t seed) : 
		cNetMessage2(eNetMessageType::RANDOM_SEED),
		seed(seed) 
	{};
	cNetMessageRandomSeed(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::RANDOM_SEED)
	{
		serializeThis(archive);
	}

	virtual void serialize(cBinaryArchiveIn& archive) { cNetMessage2::serialize(archive); serializeThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cNetMessage2::serialize(archive); serializeThis(archive); }

	uint64_t seed;
private:
	template<typename T>
	void serializeThis(T& archive)
	{
		archive & seed;
	}
};

//------------------------------------------------------------------------------
class cNetMessageGUISaveInfo : public cNetMessage2
{
public:
	cNetMessageGUISaveInfo(int savingID) :
		cNetMessage2(eNetMessageType::GUI_SAVE_INFO),
		savingID(savingID),
		reports(nullptr)
	{};
	cNetMessageGUISaveInfo(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::GUI_SAVE_INFO)
	{
		loadThis(archive);
	}

	virtual void serialize(cBinaryArchiveIn& archive) { cNetMessage2::serialize(archive); saveThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cNetMessage2::serialize(archive); saveThis(archive); }

	std::shared_ptr<std::vector<std::unique_ptr<cSavedReport>>> reports;
	cGameGuiState guiState;
	std::array<std::pair<bool, cPosition>, 4> savedPositions;
	int savingID;
private:
	template<typename T>
	void loadThis(T& archive)
	{
		if (reports == nullptr)
			reports = std::make_shared<std::vector<std::unique_ptr<cSavedReport>>>();

		int size;
		archive >> size;
		reports->resize(size);
		for (auto& report : *reports)
		{
			report = cSavedReport::createFrom(archive);
		}
		archive >> guiState;
		archive >> savingID;
		archive >> savedPositions;
	}
	template<typename T>
	void saveThis(T& archive)
	{
		if (reports == nullptr)
			reports = std::make_shared<std::vector<std::unique_ptr<cSavedReport>>>();

		archive << (int)reports->size();
		for (auto& report : *reports)
		{
			archive << *report;
		}
		archive << guiState;
		archive << savingID;
		archive << savedPositions;
	}
};

//------------------------------------------------------------------------------
class cNetMessageRequestGUISaveInfo : public cNetMessage2
{
public:
	cNetMessageRequestGUISaveInfo(int savingID) :
		cNetMessage2(eNetMessageType::REQUEST_GUI_SAVE_INFO),
		savingID(savingID)
	{};
	cNetMessageRequestGUISaveInfo(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::REQUEST_GUI_SAVE_INFO)
	{
		serializeThis(archive);
	};

	virtual void serialize(cBinaryArchiveIn& archive) { cNetMessage2::serialize(archive); serializeThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cNetMessage2::serialize(archive); serializeThis(archive); }

	int savingID;
private:
	template<typename T>
	void serializeThis(T& archive)
	{
		archive & savingID;
	}
};

//------------------------------------------------------------------------------
class cNetMessageResyncModel : public cNetMessage2
{
public:
	cNetMessageResyncModel(const cModel& model);
	cNetMessageResyncModel(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::RESYNC_MODEL)
	{
		serializeThis(archive);
	};
	virtual void serialize(cBinaryArchiveIn& archive) { cNetMessage2::serialize(archive); serializeThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cNetMessage2::serialize(archive); serializeThis(archive); }

	void apply(cModel& model) const;
private:
	template<typename T>
	void serializeThis(T& archive)
	{
		archive & data;
	}

	std::vector<uint8_t> data;
};

//------------------------------------------------------------------------------
class cNetMessageTcpHello : public cNetMessage2
{
public:
	cNetMessageTcpHello() :
		cNetMessage2(eNetMessageType::TCP_HELLO),
		packageVersion(PACKAGE_VERSION),
		packageRev(PACKAGE_REV)
	{};
	cNetMessageTcpHello(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::TCP_HELLO)
	{
		serializeThis(archive);
	};

	virtual void serialize(cBinaryArchiveIn& archive) { cNetMessage2::serialize(archive); serializeThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cNetMessage2::serialize(archive); serializeThis(archive); }

	std::string packageVersion;
	std::string packageRev;

private:
	template<typename T>
	void serializeThis(T& archive)
	{
		archive & packageVersion;
		archive & packageRev;
	}
};


//------------------------------------------------------------------------------
class cNetMessageTcpWantConnect : public cNetMessage2
{
public:
	cNetMessageTcpWantConnect() :
		cNetMessage2(eNetMessageType::TCP_WANT_CONNECT),
		ready(false),
		packageVersion(PACKAGE_VERSION),
		packageRev(PACKAGE_REV),
		socket(nullptr)
	{};
	cNetMessageTcpWantConnect(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::TCP_WANT_CONNECT)
	{
		serializeThis(archive);
	};

	virtual void serialize(cBinaryArchiveIn& archive) { cNetMessage2::serialize(archive); serializeThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cNetMessage2::serialize(archive); serializeThis(archive); }

	std::string playerName;
	cRgbColor playerColor;
	bool ready;
	std::string packageVersion;
	std::string packageRev;

	const cSocket* socket;

private:
	template<typename T>
	void serializeThis(T& archive)
	{
		archive & playerName;
		archive & playerColor;
		archive & ready;
		archive & packageVersion;
		archive & packageRev;
		// socket is not serialized
	}
};

//------------------------------------------------------------------------------
class cNetMessageTcpConnected : public cNetMessage2
{
public:
	cNetMessageTcpConnected(int playerNr) :
		cNetMessage2(eNetMessageType::TCP_CONNECTED),
		playerNr(playerNr),
		packageVersion(PACKAGE_VERSION),
		packageRev(PACKAGE_REV)
	{};
	cNetMessageTcpConnected(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::TCP_CONNECTED)
	{
		serializeThis(archive);
	};

	virtual void serialize(cBinaryArchiveIn& archive) { cNetMessage2::serialize(archive); serializeThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cNetMessage2::serialize(archive); serializeThis(archive); }

	int playerNr;
	std::string packageVersion;
	std::string packageRev;
	
private:
	template<typename T>
	void serializeThis(T& archive)
	{
		archive & playerNr;
		archive & packageVersion;
		archive & packageRev;
	}
};

//------------------------------------------------------------------------------
class cNetMessageTcpConnectFailed : public cNetMessage2
{
public:
	cNetMessageTcpConnectFailed(const std::string& reason = "") :
		cNetMessage2(eNetMessageType::TCP_CONNECT_FAILED),
		reason(reason)
	{};
	cNetMessageTcpConnectFailed(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::TCP_CONNECT_FAILED)
	{
		serializeThis(archive);
	};

	virtual void serialize(cBinaryArchiveIn& archive) { cNetMessage2::serialize(archive); serializeThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cNetMessage2::serialize(archive); serializeThis(archive); }
	
	std::string reason;
private:
	template<typename T>
	void serializeThis(T& archive)
	{
		archive & reason;
	}
};

//------------------------------------------------------------------------------
class cNetMessageTcpClose : public cNetMessage2
{
public:
	cNetMessageTcpClose(int playerNr) :
		cNetMessage2(eNetMessageType::TCP_CLOSE),
		playerNr(playerNr)
	{};
	// no serialization needed, because this is a local event

	int playerNr;
};

//------------------------------------------------------------------------------
class cNetMessageRequestResync : public cNetMessage2
{
public:
	cNetMessageRequestResync(int playerToSync = -1, int saveNumberForGuiInfo = -1) :
		cNetMessage2(eNetMessageType::REQUEST_RESYNC_MODEL),
		playerToSync(playerToSync),
		saveNumberForGuiInfo(saveNumberForGuiInfo)
	{};
	cNetMessageRequestResync(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::REQUEST_RESYNC_MODEL)
	{
		serializeThis(archive);
	};

	virtual void serialize(cBinaryArchiveIn& archive) { cNetMessage2::serialize(archive); serializeThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cNetMessage2::serialize(archive); serializeThis(archive); }

	int playerToSync;         // playerNr who will receive the data. -1 for all connected players
	int saveNumberForGuiInfo; // number of save game file, from which gui info will be loaded. -1 disables loading gui data

private:
	template<typename T>
	void serializeThis(T& archive)
	{
		archive & playerToSync;
		archive & saveNumberForGuiInfo;
	}
};

//------------------------------------------------------------------------------
class cNetMessageGameAlreadyRunning : public cNetMessage2
{
public:
	cNetMessageGameAlreadyRunning(const cModel& model);
	cNetMessageGameAlreadyRunning(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::GAME_ALREADY_RUNNING)
	{
		serializeThis(archive);
	};

	virtual void serialize(cBinaryArchiveIn& archive) { cNetMessage2::serialize(archive); serializeThis(archive); }
	virtual void serialize(cTextArchiveIn& archive)   { cNetMessage2::serialize(archive); serializeThis(archive); }

	std::string mapName;
	uint32_t mapCrc;
	std::vector<cPlayerBasicData> playerList;

private:
	template<typename T>
	void serializeThis(T& archive)
	{
		archive & mapName;
		archive & mapCrc;
		archive & playerList;
	}
};

//------------------------------------------------------------------------------
class cNetMessageWantRejoinGame : public cNetMessage2
{
public:
	cNetMessageWantRejoinGame() :
		cNetMessage2(eNetMessageType::WANT_REJOIN_GAME)
	{};
	cNetMessageWantRejoinGame(cBinaryArchiveOut& archive) :
		cNetMessage2(eNetMessageType::WANT_REJOIN_GAME)
	{};

};


#endif //netmessage2H
