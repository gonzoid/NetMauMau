/*
 * Copyright 2014 by Heiko Schäfer <heiko@rangun.de>
 *
 * This file is part of NetMauMau.
 *
 * NetMauMau is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * NetMauMau is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with NetMauMau.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NETMAUMAU_SERVERCONNECTION_H
#define NETMAUMAU_SERVERCONNECTION_H

#if defined(HAVE_CONFIG_H) || defined(IN_IDE_PARSER)
#include "config.h"
#endif

#include "abstractconnection.h"

#define MIN_MAJOR 0
#define MIN_MINOR 2

namespace NetMauMau {

namespace Server {

class Connection : public Common::AbstractConnection {
	DISALLOW_COPY_AND_ASSIGN(Connection)
public:
	typedef enum { NONE, PLAY, CAP, REFUSED, PLAYERLIST } ACCEPT_STATE;
	typedef std::map<uint32_t, std::string, std::greater<uint32_t> > VERSIONEDMESSAGE;

	Connection(uint32_t minVer, uint16_t port = SERVER_PORT, const char *server = NULL);
	virtual ~Connection();

	virtual void connect() throw(Common::Exception::SocketException);

	int wait(timeval *tv = NULL);

	inline const PLAYERINFOS &getPlayers() const {
		return getRegisteredPlayers();
	}

	NAMESOCKFD getPlayerInfo(const std::string &name) const;

	void sendVersionedMessage(const VERSIONEDMESSAGE &vm) const
	throw(Common::Exception::SocketException);

	Connection &operator<<(const std::string &msg) throw(Common::Exception::SocketException);
	Connection &operator>>(std::string &msg) throw(Common::Exception::SocketException);

	ACCEPT_STATE accept(INFO &v, bool refuse = false) throw(Common::Exception::SocketException);

	inline void setCapabilities(const CAPABILITIES &caps) {
		m_caps = caps;
	}

	inline static uint32_t getServerVersion() {
		return MAKE_VERSION(SERVER_VERSION_MAJOR, SERVER_VERSION_MINOR);
	}

	inline uint32_t getMinClientVersion() const {
		return m_clientMinVer;
	}

	void clearPlayerPictures() const;

protected:
	virtual bool wire(SOCKET sockfd, const struct sockaddr *addr, socklen_t addrlen) const;
	virtual std::string wireError(const std::string &err) const;
	virtual void intercept() throw(Common::Exception::SocketException);

private:
	static bool isPNG(const std::string &pic);

private:
	CAPABILITIES m_caps;
	uint32_t m_clientMinVer;
};

}

}

#endif /* NETMAUMAU_SERVERCONNECTION_H */

// kate: indent-mode cstyle; indent-width 4; replace-tabs off; tab-width 4; 
