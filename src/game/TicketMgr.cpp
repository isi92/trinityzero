/*
 * Copyright (C) 2005-2008 MaNGOS 
 *
 * Copyright (C) 2008 Trinity 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "TicketMgr.h"
#include "Policies/SingletonImp.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Language.h"
#include "Player.h"
INSTANTIATE_SINGLETON_1( TicketMgr );

#include "Common.h"
//#include "Log.h"
#include "ObjectAccessor.h"



GM_Ticket* TicketMgr::GetGMTicket(uint64 ticketGuid)
{
	for(GmTicketList::iterator i = GM_TicketList.begin(); i != GM_TicketList.end();)
	{
		if((*i)->guid == ticketGuid)
		{
			return (*i);
		}
		++i;
	}
	return NULL;
}

GM_Ticket* TicketMgr::GetGMTicketByPlayer(uint64 playerGuid)
{
	for(GmTicketList::iterator i = GM_TicketList.begin(); i != GM_TicketList.end();)
	{
		if((*i)->playerGuid == playerGuid && !(*i)->closed)
		{
			return (*i);
		}
		++i;
	}
	return NULL;
}

GM_Ticket* TicketMgr::GetGMTicketByName(const char* name)
{
	std::string pname = name;
	if(!normalizePlayerName(pname))
		return NULL;

	Player *plr = objmgr.GetPlayer(pname.c_str());
	if(!plr)
		return NULL;
	
	uint64 playerGuid = plr->GetGUID();

	for(GmTicketList::iterator i = GM_TicketList.begin(); i != GM_TicketList.end();)
	{
		if((*i)->playerGuid == playerGuid && !(*i)->closed)
		{
			return (*i);
		}
		++i;
	}
	return NULL;
}

void TicketMgr::AddGMTicket(GM_Ticket *ticket, bool startup)
{
	ASSERT( ticket );
	GM_TicketList.push_back(ticket);

	// save
	if(!startup)
		SaveGMTicket(ticket);
}

void TicketMgr::DeleteGMTicketPermanently(uint64 ticketGuid)
{
	for(GmTicketList::iterator i = GM_TicketList.begin(); i != GM_TicketList.end();)
	{
		if((*i)->guid == ticketGuid)
		{
			i = GM_TicketList.erase(i);
		}
		else
		{
			++i;
		}
	}

	// delete database record
	CharacterDatabase.PExecute("DELETE FROM gm_tickets WHERE guid=%u", ticketGuid);
}


void TicketMgr::LoadGMTickets()
{
	// Delete all out of object holder
	GM_TicketList.clear();
	QueryResult *result = CharacterDatabase.Query( "SELECT `guid`, `playerGuid`, `name`, `message`, `timestamp`, `closed`, `assignedto`, `comment` FROM gm_tickets WHERE deleted = '0'" );
	GM_Ticket *ticket;
	
	//ticket = NULL;
	if(!result)
		return;

	do
	{
		Field *fields = result->Fetch();
		ticket = new GM_Ticket;
		ticket->guid = fields[0].GetUInt64();
		ticket->playerGuid = fields[1].GetUInt64();
		ticket->name = fields[2].GetString();
		ticket->message = fields[3].GetString();
		ticket->timestamp = fields[4].GetUInt32();
		ticket->closed = fields[5].GetUInt16();
		ticket->assignedToGM = fields[6].GetUInt64();
		ticket->comment = fields[7].GetString();

		AddGMTicket(ticket, true);

	} while( result->NextRow() );

	sWorld.SendGMText(LANG_COMMAND_TICKETRELOAD, result->GetRowCount());

	delete result;
}

void TicketMgr::RemoveGMTicket(uint64 ticketGuid)
{
	for(GmTicketList::iterator i = GM_TicketList.begin(); i != GM_TicketList.end();)
	{
		if((*i)->guid == ticketGuid && !(*i)->closed)
		{
			(*i)->closed = true;
			SaveGMTicket((*i));
		}
		++i;
	}
}


void TicketMgr::RemoveGMTicketByPlayer(uint64 playerGuid)
{
	for(GmTicketList::iterator i = GM_TicketList.begin(); i != GM_TicketList.end();)
	{
		if((*i)->playerGuid == playerGuid && !(*i)->closed)
		{
			(*i)->closed = true;
			SaveGMTicket((*i));
		}
		++i;
	}
}

void TicketMgr::SaveGMTicket(GM_Ticket* ticket)
{
	std::stringstream ss;
	ss << "REPLACE INTO gm_tickets (`guid`, `playerGuid`, `name`, `message`, `timestamp`, `closed`, `assignedto`, `comment`) VALUES(";
	ss << ticket->guid << ", ";
	ss << ticket->playerGuid << ", '";
	ss << ticket->name << "', '";
	ss << ticket->message << "', " ;
	ss << ticket->timestamp << ", ";
	ss << ticket->closed << ", '";
	ss << ticket->assignedToGM << "', '";
	ss << ticket->comment << "');";

	CharacterDatabase.BeginTransaction();
	CharacterDatabase.Execute(ss.str().c_str());
	CharacterDatabase.CommitTransaction();
		
}

void TicketMgr::UpdateGMTicket(GM_Ticket *ticket)
{
	SaveGMTicket(ticket);
}

uint64 TicketMgr::GenerateTicketID()
{
	QueryResult *result = CharacterDatabase.Query("SELECT MAX(guid) FROM gm_tickets");
	if(result)
	{
		m_ticketid = result->Fetch()[0].GetUInt64() + 1;
		delete result;
	}

	return m_ticketid;
}