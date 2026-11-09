/*
 * WebIRC / CGI:IRC Support
 * (C) Copyright 2006-.. Bram Matthys (Syzop) and the UnrealIRCd team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "unrealircd.h"

/* Module header */
ModuleHeader MOD_HEADER = {
    "webirc",
    "5.0",
    "WebIRC/CGI:IRC Support",
    "UnrealIRCd Team",
    "unrealircd-6",
};

/** The type of WEBIRC client (in practice: secure or not).
 * Do not change these numbers, they are used in S2S.
 */
typedef enum WEBIRCType {
	WEBIRC_TYPE_WEBIRC = 1,        /**< Via WEBIRC */
	WEBIRC_TYPE_WEBIRC_SECURE = 2, /**< Via WEBIRC and gateway indicated the client is securely connected */
} WEBIRCType;

typedef struct WEBIRCUser {
	WEBIRCType type; /**< One of WEBIRC_TYPE_* (secure or not)  */
	char *name;      /**< Name of the proxy { } block. NULL for remote clients on older servers. */
	char *ip;        /**< IP of the gateway. NULL for remote clients on older servers. */
	char *host;      /**< Hostname of the gateway (and otherwise the IP). NULL for remote clients on older servers. */
} WEBIRCUser;

/* Global variables */
ModDataInfo *webirc_md = NULL;

/* Forward declarations */
CMD_FUNC(cmd_webirc);
CMD_FUNC(cmd_chgip);
int webirc_local_pass(Client *client, const char *password);
const char *webirc_md_serialize(ModData *m);
void webirc_md_unserialize(const char *str, ModData *m);
void webirc_md_free(ModData *md);
int webirc_secure_connect(Client *client);
int webirc_whois(Client *client, Client *target, NameValuePrioList **list);

#define WEBIRC(x)         ((WEBIRCUser *)moddata_client(x, webirc_md).ptr)
#define IsWEBIRC(x)       (WEBIRC(x) != NULL)
#define IsWEBIRCSecure(x) (WEBIRC(x) && (WEBIRC(x)->type == WEBIRC_TYPE_WEBIRC_SECURE))

#define MSG_WEBIRC "WEBIRC"
#define MSG_CHGIP  "CHGIP"

/** Called upon module init */
MOD_INIT()
{
	ModDataInfo mreq;

	MARK_AS_OFFICIAL_MODULE(modinfo);

	memset(&mreq, 0, sizeof(mreq));
	mreq.name = "webirc";
	mreq.type = MODDATATYPE_CLIENT;
	mreq.priority = -999997;
	mreq.serialize = webirc_md_serialize;
	mreq.unserialize = webirc_md_unserialize;
	mreq.free = webirc_md_free;
	mreq.sync = MODDATA_SYNC_EARLY;
	webirc_md = ModDataAdd(modinfo->handle, mreq);
	if (!webirc_md)
	{
		config_error("could not register webirc moddata");
		return MOD_FAILED;
	}

	HookAdd(modinfo->handle, HOOKTYPE_LOCAL_PASS, 0, webirc_local_pass);
	HookAdd(modinfo->handle, HOOKTYPE_SECURE_CONNECT, 0, webirc_secure_connect);
	HookAdd(modinfo->handle, HOOKTYPE_WHOIS, 0, webirc_whois);

	CommandAdd(modinfo->handle, MSG_WEBIRC, cmd_webirc, MAXPARA, CMD_UNREGISTERED | CMD_USER);
	CommandAdd(modinfo->handle, MSG_CHGIP, cmd_chgip, MAXPARA, CMD_SERVER);

	return MOD_SUCCESS;
}

/** Called upon module load */
MOD_LOAD()
{
	return MOD_SUCCESS;
}

/** Called upon unload */
MOD_UNLOAD()
{
	return MOD_SUCCESS;
}

const char *webirc_md_serialize(ModData *m)
{
	static char buf[512];
	WEBIRCUser *w = m->ptr;

	if (!w)
		return NULL; /* not set */

	if (!w->name && !w->ip && !w->host)
		snprintf(buf, sizeof(buf), "%d", (int)w->type);
	else
		snprintf(buf, sizeof(buf), "%d;%s;%s;%s", (int)w->type,
		         w->name ? w->name : "", w->ip ? w->ip : "", w->host ? w->host : "");
	return buf;
}

void webirc_md_unserialize(const char *str, ModData *m)
{
	WEBIRCUser *w;
	char buf[512];
	char *type, *name, *ip, *host, *p = NULL;
	int t;

	if (m->ptr)
		webirc_md_free(m);
	if (BadPtr(str))
		return; /* empty/freed */

	strlcpy(buf, str, sizeof(buf));
	type = strtoken_noskip(&p, buf, ";");
	t = atoi(type);
	if (t <= 0)
		return;

	m->ptr = w = safe_alloc(sizeof(WEBIRCUser));
	w->type = t;

	name = strtoken_noskip(&p, NULL, ";");
	ip = strtoken_noskip(&p, NULL, ";");
	host = strtoken_noskip(&p, NULL, ";");
	if (!BadPtr(name) && security_group_valid_name(name))
		safe_strdup(w->name, name);
	if (!BadPtr(ip) && is_valid_ip(ip))
		safe_strdup(w->ip, ip);
	if (!BadPtr(host) && (valid_host(host, 1) || is_valid_ip(host)))
		safe_strdup(w->host, host);
}

void webirc_md_free(ModData *md)
{
	WEBIRCUser *w = md->ptr;

	if (w)
	{
		safe_free(w->name);
		safe_free(w->host);
		safe_free(w->ip);
		safe_free(md->ptr);
	}
}

ConfigItem_proxy *find_webirc(Client *client, const char *password, ProxyType type, char **errorstr)
{
	ConfigItem_proxy *e;
	char *error = NULL;

	for (e = conf_proxy; e; e = e->next)
	{
		if ((e->type == type) && user_allowed_by_security_group(client, e->mask))
		{
			if (type == PROXY_WEBIRC)
			{
				/* Check password */
				if (!Auth_Check(client, e->auth, password))
					error = "CGI:IRC -- Invalid password";
				else
					return e; /* Found matching block, return straight away */
			} else if (type == PROXY_WEBIRC_PASS)
			{
				return e; /* The PROXY_WEBIRC_PASS type has no password checking */
			}
		}
	}

	if (error)
		*errorstr = error; /* Invalid password (this error was delayed) */
	else
		*errorstr = "CGI:IRC -- No access"; /* No match found */

	return NULL;
}

#define WEBIRC_STRING    "WEBIRC_"
#define WEBIRC_STRINGLEN (sizeof(WEBIRC_STRING) - 1)

/* Does the CGI:IRC host spoofing work */
void dowebirc(Client *client, ConfigItem_proxy *proxy, const char *ip, const char *host, const char *options)
{
	char gateway_ip[IPLEN + 1];
	char gateway_host[HOSTLEN + 1];
	WEBIRCUser *w;

	if (IsWEBIRC(client))
	{
		exit_client(client, NULL, "Double CGI:IRC request (already identified)");
		return;
	}

	strlcpy(gateway_ip, client->ip, sizeof(gateway_ip));
	if (client->local->hostp && client->local->hostp->h_name)
		strlcpy(gateway_host, client->local->hostp->h_name, sizeof(gateway_host));
	else
		strlcpy(gateway_host, client->ip, sizeof(gateway_host));

	if (host && !strcmp(ip, host))
		host = NULL; /* host did not resolve, make it NULL */

	if (!set_client_ip(client, ip))
		return; /* Failed, client exited */

	/* Update client->local->hostp */
	/* (free old) */
	if (client->local->hostp)
	{
		unreal_free_hostent(client->local->hostp);
		client->local->hostp = NULL;
	}
	/* (create new) */
	if (host && valid_host(host, 1))
		client->local->hostp = unreal_create_hostent(host, client->ip);

	/* Update sockhost */
	set_sockhost(client, client->ip);

	moddata_client(client, webirc_md).ptr = w = safe_alloc(sizeof(WEBIRCUser));
	w->type = WEBIRC_TYPE_WEBIRC;
	safe_strdup(w->name, proxy->name);
	safe_strdup(w->ip, gateway_ip);
	safe_strdup(w->host, gateway_host);

	if (options)
	{
		char optionsbuf[BUFSIZE];
		char *name, *p = NULL, *p2;
		strlcpy(optionsbuf, options, sizeof(optionsbuf));
		for (name = strtoken(&p, optionsbuf, " "); name; name = strtoken(&p, NULL, " "))
		{
			p2 = strchr(name, '=');
			if (p2)
				*p2 = '\0';
			if (!strcmp(name, "secure") && IsSecure(client))
			{
				/* The entire [client]--[webirc gw]--[server] chain is secure */
				w->type = WEBIRC_TYPE_WEBIRC_SECURE;
			}
		}
	}
}

/** Normalize an IP address string (eg "::FFFF:1.2.3.4" vs "::ffff:1.2.3.4").
 * @returns 1 on success (normalized IP in buf), 0 if not a valid IP.
 */
static int webirc_normalize_ip(const char *ip, char *buf, size_t buflen)
{
	char raw[16];
	int af = strchr(ip, ':') ? AF_INET6 : AF_INET;

	if (inet_pton(af, ip, raw) != 1)
		return 0;
	return inetntop(af, raw, buf, buflen) ? 1 : 0;
}

/** Find the named proxy { } block of type webirc */
static ConfigItem_proxy *find_webirc_by_name(const char *name)
{
	ConfigItem_proxy *e;

	for (e = conf_proxy; e; e = e->next)
		if ((e->type == PROXY_WEBIRC) && e->name && !strcmp(e->name, name))
			return e;
	return NULL;
}

/** Update the realhost, cloaked host and (if not +t) the vhost
 * of a user after an IP change. Used for both local and remote users.
 * Caller must have called userhost_save_current() before, and must
 * call userhost_changed() afterwards.
 */
static void webirc_update_hosts(Client *client, const char *realhost)
{
	strlcpy(client->user->realhost, realhost, sizeof(client->user->realhost));
	make_cloakedhost(client, client->user->realhost, client->user->cloakedhost, sizeof(client->user->cloakedhost));
	/* A vhost set by SETHOST/CHGHOST/vhost block (+t) stays.
	 * Otherwise the vhost is the cloaked host, which has now changed.
	 */
	if (!IsSetHost(client) || !client->user->virthost)
		safe_strdup(client->user->virthost, client->user->cloakedhost);
}

/** Send updated synced moddata that depends on the IP (currently: geoip) */
static void webirc_broadcast_ip_moddata(Client *client)
{
	ModDataInfo *mdi = findmoddata_byname("geoip", MODDATATYPE_CLIENT);

	if (mdi && mdi->sync)
		broadcast_md_client(mdi, client, &moddata_client(client, mdi));
}

/** Post-registration WEBIRC: the gateway tells us the IP of the
 * user behind it has changed (eg. a web user who came back from
 * a different network to a still-connected session).
 * Only allowed if the proxy { } block has allow-ip-update yes;
 * This is a ZenIRCd extension.
 */
static void webirc_ip_update(Client *client, const char *password, const char *ip, const char *host)
{
	WEBIRCUser *w = WEBIRC(client);
	ConfigItem_proxy *e;
	char newip[IPLEN + 1];
	char oldip[IPLEN + 1];
	char oldhost[HOSTLEN + 1];

	if (!w || !w->name)
	{
		sendto_one(client, NULL, ":%s FAIL WEBIRC NOT_WEBIRC :This connection was not made through a named WebIRC gateway", me.name);
		return;
	}

	e = find_webirc_by_name(w->name);
	if (!e || !e->allow_ip_update)
	{
		sendto_one(client, NULL, ":%s FAIL WEBIRC IP_UPDATE_DISABLED :IP updates are not enabled for this WebIRC gateway", me.name);
		return;
	}

	if (!Auth_Check(client, e->auth, password))
	{
		unreal_log(ULOG_WARNING, "webirc", "WEBIRC_IP_UPDATE_BAD_PASSWORD", client,
		           "WebIRC gateway $webirc_gateway sent an IP update for $client.details with an invalid password",
		           log_data_string("webirc_gateway", w->name));
		sendto_one(client, NULL, ":%s FAIL WEBIRC INVALID_PASSWORD :Invalid WebIRC password", me.name);
		return;
	}

	if (!webirc_normalize_ip(ip, newip, sizeof(newip)))
	{
		sendto_one(client, NULL, ":%s FAIL WEBIRC INVALID_IP %s :Invalid IP address", me.name, ip);
		return;
	}

	if (host && (!strcmp(host, ip) || !valid_host(host, 1)))
		host = NULL; /* did not resolve, or bogus: use the IP */

	if (!strcmp(newip, client->ip) && !strcmp(host ? host : client->ip, client->user->realhost))
	{
		sendto_one(client, NULL, ":%s NOTE WEBIRC IP_UNCHANGED %s :IP address is unchanged", me.name, client->ip);
		return;
	}

	strlcpy(oldip, client->ip, sizeof(oldip));
	strlcpy(oldhost, client->user->realhost, sizeof(oldhost));

	userhost_save_current(client);

	/* This runs HOOKTYPE_IP_CHANGE: Z-lines, blacklist (DNSBL),
	 * connect-flood, geoip, reputation, known-users...
	 */
	if (!set_client_ip(client, newip) || IsDeadSocket(client))
		return; /* Client is being killed */

	/* Update client->local->hostp */
	if (client->local->hostp)
	{
		unreal_free_hostent(client->local->hostp);
		client->local->hostp = NULL;
	}
	if (host)
	{
		client->local->hostp = unreal_create_hostent(host, client->ip);
		set_sockhost(client, host);
	}
	/* (else: set_client_ip() already set sockhost to the IP) */

	webirc_update_hosts(client, client->local->sockhost);

	unreal_log(ULOG_INFO, "webirc", "WEBIRC_IP_UPDATE", client,
	           "WebIRC gateway $webirc_gateway updated the IP of $client.details from $old_ip ($old_host) to $client.ip ($client.user.realhost)",
	           log_data_string("webirc_gateway", w->name),
	           log_data_string("old_ip", oldip),
	           log_data_string("old_host", oldhost));

	/* Re-check K-Lines/G-Lines and shuns against the new IP/host.
	 * (Z-Lines were already checked from the HOOKTYPE_IP_CHANGE hook)
	 */
	if (find_tkline_match(client, 0))
		return; /* Client killed */
	find_shun(client);

	/* Tell local users in common channels (CHGHOST or rejoin) */
	userhost_changed(client);

	/* Tell other servers. Servers that don't know CHGIP ignore it
	 * (stock UnrealIRCd), they keep showing the old IP/host.
	 */
	sendto_server(client, 0, 0, NULL, ":%s CHGIP %s %s %s",
	              me.id, client->id, encode_ip(client->ip), client->user->realhost);
	webirc_broadcast_ip_moddata(client);

	sendto_one(client, NULL, ":%s NOTE WEBIRC IP_UPDATED %s %s :IP address updated",
	           me.name, client->ip, client->user->realhost);
}

/** Server-to-server: IP (and realhost) of a user changed.
 * :<sid> CHGIP <uid> <base64 ip> <realhost>
 * This is a ZenIRCd extension. The cloaked host is recalculated locally,
 * just like with UID. It is server-sourced because the command parser
 * looks up commands by the type of the source (a user source would
 * only find CMD_USER commands).
 */
CMD_FUNC(cmd_chgip)
{
	Client *target;
	const char *ip;
	char raw[16];
	char newip[IPLEN + 1];
	int af;

	if ((parc < 4) || BadPtr(parv[3]))
		return;

	target = find_user(parv[1], NULL);
	if (!target || MyConnect(target) || (target->direction != client->direction))
		return; /* Unknown, our own, or from the wrong direction: ignore */

	if (!(ip = decode_ip(parv[2])) || !valid_host(parv[3], 0))
	{
		unreal_log(ULOG_ERROR, "link", "BAD_CHGIP", client,
		           "Server link $server sent a CHGIP for $target with a bad IP or hostname: $bad_ip $bad_host",
		           log_data_client("server", client->direction),
		           log_data_client("target", target),
		           log_data_string("bad_ip", parv[2]),
		           log_data_string("bad_host", parv[3]));
		return;
	}

	/* Don't use set_client_ip() here: that runs HOOKTYPE_IP_CHANGE,
	 * which is meant for local clients (bans, DNSBL, connect-flood..).
	 */
	af = strchr(ip, ':') ? AF_INET6 : AF_INET;
	if ((inet_pton(af, ip, raw) != 1) || !inetntop(af, raw, newip, sizeof(newip)))
		return; /* cannot happen, decode_ip() validated it */

	userhost_save_current(target);

	memcpy(target->rawip, raw, sizeof(target->rawip));
	if (af == AF_INET6)
		SetIPV6(target);
	else
		ClearIPV6(target);
	safe_strdup(target->ip, newip);

	webirc_update_hosts(target, parv[3]);

	userhost_changed(target);

	sendto_server(client, 0, 0, NULL, ":%s CHGIP %s %s %s",
	              client->id, target->id, parv[2], target->user->realhost);
}

/* WEBIRC <pass> "cgiirc" <hostname> <ip> [:option1 [option2...]]
 * After registration (ZenIRCd extension, needs proxy::allow-ip-update):
 * the same syntax updates the IP of the user. Options are ignored then.
 */
CMD_FUNC(cmd_webirc)
{
	const char *ip, *host, *password, *options;
	ConfigItem_proxy *e;
	char *error = NULL;

	if ((parc < 5) || BadPtr(parv[4]))
	{
		sendnumeric(client, ERR_NEEDMOREPARAMS, "WEBIRC");
		return;
	}

	password = parv[1];
	host = !DONT_RESOLVE ? parv[3] : parv[4];
	ip = parv[4];
	options = parv[5]; /* can be NULL */

	if (IsUser(client))
	{
		webirc_ip_update(client, password, ip, host);
		return;
	}

	/* Check if allowed host */
	e = find_webirc(client, password, PROXY_WEBIRC, &error);
	if (!e)
	{
		exit_client(client, NULL, error);
		return;
	}

	/* And do our job.. */
	dowebirc(client, e, ip, host, options);
}

int webirc_local_pass(Client *client, const char *password)
{
	if (!strncmp(password, WEBIRC_STRING, WEBIRC_STRINGLEN))
	{
		char buf[512];
		char *ip, *host;
		ConfigItem_proxy *e;
		char *error = NULL;

		/* Work on a copy as we may trash it */
		strlcpy(buf, password, sizeof(buf));
		e = find_webirc(client, NULL, PROXY_WEBIRC_PASS, &error);
		if (e)
		{
			/* Ok now we got that sorted out, proceed:
			 * Syntax: WEBIRC_<ip>_<resolvedhostname>
			 * The <resolvedhostname> has been checked ip->host AND host->ip by CGI:IRC itself
			 * already so we trust it.
			 */
			ip = buf + WEBIRC_STRINGLEN;
			host = strchr(ip, '_');
			if (!host)
			{
				exit_client(client, NULL, "Invalid CGI:IRC IP received");
				return HOOK_DENY;
			}
			*host++ = '\0';

			dowebirc(client, e, ip, host, NULL);
			return HOOK_DENY;
		}
		/* fallthrough if not in webirc block.. */
	}

	return HOOK_CONTINUE; /* not webirc */
}

/** Called from register_user() right after setting user +z */
int webirc_secure_connect(Client *client)
{
	/* Remove secure mode (-z) if the WEBIRC gateway did not ensure
	 * us that their [client]--[webirc gateway] connection is also
	 * secure (eg: using https)
	 */
	if (IsWEBIRC(client) && IsSecureConnect(client) && !IsWEBIRCSecure(client))
		client->umodes &= ~UMODE_SECURE;
	return 0;
}

/** Show the WEBIRC gateway in WHOIS */
int webirc_whois(Client *client, Client *target, NameValuePrioList **list)
{
	WhoisConfigDetails policy = whois_get_policy(client, target, "webirc");
	WEBIRCUser *w = WEBIRC(target);
	const char *ip = "*", *host = "*";

	if (!w || (policy <= WHOIS_CONFIG_DETAILS_NONE))
		return 0;

	if (policy == WHOIS_CONFIG_DETAILS_FULL)
	{
		if (w->ip)
			ip = w->ip;
		if (w->host)
			host = w->host;
	}

	if (w->name)
	{
		add_nvplist_numeric_fmt(list, 40001, "webirc", client, RPL_WHOISGATEWAY,
		                        "%s %s %s :is connecting via WebIRC gateway %s",
		                        target->name, host, ip, w->name);
	} else
	{
		/* Servers older than 6.2.8 didn't communicate name/ip/host in S2S: */
		add_nvplist_numeric(list, 40001, "webirc", client, RPL_WHOISGATEWAY,
		                    target->name, host, ip, "is connecting via a WebIRC gateway");
	}
	return 0;
}
