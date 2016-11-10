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

#ifndef ui_graphical_game_control_chatcommand_chatcommandH
#define ui_graphical_game_control_chatcommand_chatcommandH

#include <string>
#include <memory>

template<typename... Arguments>
class cChatCommandParser;

class cChatCommandExecutor;

class cChatCommand
{
public:
	static bool isCommand(const std::string& command);

	cChatCommand(const std::string name, const std::string description);

	const std::string& getName() const;
	const std::string& getDescription() const;

	cChatCommand& setShouldBeReported (bool value);
	bool getShouldBeReported() const;

	cChatCommand& setIsServerOnly (bool value);
	bool getIsServerOnly () const;

	template<typename NewArgument, typename... Args>
	cChatCommandParser<NewArgument> addArgument(Args&&... args);

	template<typename F>
	std::unique_ptr<cChatCommandExecutor> setAction(F function);

private:
	std::string name;
	std::string description;
	bool shouldBeReported;
	bool isServerOnly;
};

//------------------------------------------------------------------------------
template<typename NewArgument, typename... Args>
cChatCommandParser<NewArgument> cChatCommand::addArgument(Args&&... args)
{
	return cChatCommandParser<NewArgument>(cChatCommandParser<>(std::move(*this)), NewArgument(std::forward<Args>(args)...));
}

//------------------------------------------------------------------------------
template<typename F>
std::unique_ptr<cChatCommandExecutor> cChatCommand::setAction(F function)
{
	return std::make_unique<cChatCommandExecutorImpl<F>>(std::move(function), cChatCommandParser<>(std::move(*this)));
}

#endif // ui_graphical_game_control_chatcommand_chatcommandH
