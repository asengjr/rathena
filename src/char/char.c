// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include <sys/types.h>

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "../common/strlib.h"
#include "../common/core.h"
#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/mmo.h"
#include "../common/db.h"
#include "../common/version.h"
#include "../common/lock.h"
#include "../common/showmsg.h"
#include "../common/malloc.h"

#include "char.h"
#include "inter.h"
#include "int_pet.h"
#include "int_homun.h"
#include "int_guild.h"
#include "int_party.h"
#include "int_storage.h"
#ifdef ENABLE_SC_SAVING
#include "int_status.h"
#endif

#ifndef TXT_SQL_CONVERT
struct mmo_map_server{
	long ip;
	short port;
	int users;
	unsigned short map[MAX_MAP_PER_SERVER];
} server[MAX_MAP_SERVERS];
int server_fd[MAX_MAP_SERVERS];

int login_fd, char_fd;
char userid[24];
char passwd[24];
char server_name[20];
char wisp_server_name[NAME_LENGTH] = "Server";
char login_ip_str[128];
in_addr_t login_ip;
int login_port = 6900;
char char_ip_str[128];
in_addr_t char_ip;
char bind_ip_str[128];
in_addr_t bind_ip;
int char_port = 6121;
int char_maintenance;
int char_new;
int char_new_display;
int email_creation = 0; // disabled by default
#endif
char char_txt[1024]="save/athena.txt";
char backup_txt[1024]="save/backup.txt"; //By zanetheinsane
char friends_txt[1024]="save/friends.txt"; // davidsiaw
#ifndef TXT_SQL_CONVERT
char backup_txt_flag = 0; // The backup_txt file was created because char deletion bug existed. Now it's finish and that take a lot of time to create a second file when there are a lot of characters. => option By [Yor]
char unknown_char_name[1024] = "Unknown";
char char_log_filename[1024] = "log/char.log";
char db_path[1024]="db";

// Advanced subnet check [LuzZza]
struct _subnet {
	long subnet;
	long mask;
	long char_ip;
	long map_ip;
} subnet[16];

int subnet_count = 0;

int name_ignoring_case = 0; // Allow or not identical name for characters but with a different case by [Yor]
int char_name_option = 0; // Option to know which letters/symbols are authorised in the name of a character (0: all, 1: only those in char_name_letters, 2: all EXCEPT those in char_name_letters) by [Yor]
//The following are characters that are trimmed regardless because they cause confusion and problems on the servers. [Skotlex]
#define TRIM_CHARS "\032\t\x0A\x0D "
char char_name_letters[1024] = ""; // list of letters/symbols authorised (or not) in a character name. by [Yor]

int log_char = 1;	// loggin char or not [devil]
int log_inter = 1;	// loggin inter or not [devil]

struct char_session_data{
	int account_id, login_id1, login_id2, sex;
	int found_char[9];
	char email[40]; // e-mail (default: a@a.com) by [Yor]
	time_t connect_until_time; // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
};

#define AUTH_FIFO_SIZE 256
struct {
	int account_id, char_id, login_id1, login_id2, ip, char_pos, delflag, sex;
	time_t connect_until_time; // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
} auth_fifo[AUTH_FIFO_SIZE];
int auth_fifo_pos = 0;

int check_ip_flag = 1; // It's to check IP of a player between char-server and other servers (part of anti-hacking system)
static int online_check = 1; //If one, it won't let players connect when their account is already registered online and will send the relevant map server a kick user request. [Skotlex]

int char_id_count = START_CHAR_NUM;
struct character_data *char_dat;

int char_num, char_max;
int max_connect_user = 0;
int gm_allow_level = 99;
int autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
int save_log = 1;
int start_zeny = 500;
int start_weapon = 1201;
int start_armor = 2301;
int guild_exp_rate = 100;

//Custom limits for the fame lists. [Skotlex]
int fame_list_size_chemist = MAX_FAME_LIST;
int fame_list_size_smith = MAX_FAME_LIST;
int fame_list_size_taekwon = MAX_FAME_LIST;

// Char-server-side stored fame lists [DracoRPG]
struct fame_list smith_fame_list[MAX_FAME_LIST];
struct fame_list chemist_fame_list[MAX_FAME_LIST];
struct fame_list taekwon_fame_list[MAX_FAME_LIST];

// Initial position (it's possible to set it in conf file)
struct point start_point = { 0, 53, 111};

struct gm_account *gm_account = NULL;
int GM_num = 0;

// online players by [Yor]
char online_txt_filename[1024] = "online.txt";
char online_html_filename[1024] = "online.html";
int online_sorting_option = 0; // sorting option to display online players in online files
int online_display_option = 1; // display options: to know which columns must be displayed
int online_refresh_html = 20; // refresh time (in sec) of the html file in the explorer
int online_gm_display_min_level = 20; // minimum GM level to display 'GM' when we want to display it

//These are used to aid the map server in identifying valid clients. [Skotlex]
static int max_account_id = DEFAULT_MAX_ACCOUNT_ID, max_char_id = DEFAULT_MAX_CHAR_ID;

struct online_char_data {
	int account_id;
	int char_id;
	short server;
	unsigned waiting_disconnect :1;
};

struct dbt *online_char_db; //Holds all online characters.

time_t update_online; // to update online files when we receiving information from a server (not less than 8 seconds)

int console = 0;

//------------------------------
// Writing function of logs file
//------------------------------
int char_log(char *fmt, ...) {
	if(log_char)
	{
		FILE *logfp;
		va_list ap;
		time_t raw_time;
		char tmpstr[2048];

		va_start(ap, fmt);

		logfp = fopen(char_log_filename, "a");
		if (logfp) {
			if (fmt[0] == '\0') // jump a line if no message
				fprintf(logfp, RETCODE);
			else {
				time(&raw_time);
				strftime(tmpstr, 24, "%d-%m-%Y %H:%M:%S", localtime(&raw_time));
				sprintf(tmpstr + 19, ": %s", fmt);
				vfprintf(logfp, tmpstr, ap);
			}
			fclose(logfp);
		}
		va_end(ap);
	}
	return 0;
}

//----------------------------------------------------------------------
// Determine if an account (id) is a GM account
// and returns its level (or 0 if it isn't a GM account or if not found)
//----------------------------------------------------------------------
int isGM(int account_id) {
	int i;

	for(i = 0; i < GM_num; i++)
		if (gm_account[i].account_id == account_id)
			return gm_account[i].level;
	return 0;
}

//Search character data from the aid/cid givem
struct mmo_charstatus* search_character(int aid, int cid) {
	int i;
	for (i = 0; i < char_num; i++) {
		if (char_dat[i].status.char_id == cid && char_dat[i].status.account_id == aid)
			return &char_dat[i].status;
	}
	return NULL;
}
	
struct mmo_charstatus* search_character_byname(char* character_name)
{
	int i = search_character_index(character_name);
	if (i == -1) return NULL;
	return &char_dat[i].status;
}

//----------------------------------------------
// Search an character id
//   (return character index or -1 (if not found))
//   If exact character name is not found,
//   the function checks without case sensitive
//   and returns index if only 1 character is found
//   and similar to the searched name.
//----------------------------------------------
int search_character_index(char* character_name) {
	int i, quantity, index;

	quantity = 0;
	index = -1;
	for(i = 0; i < char_num; i++) {
		// Without case sensitive check (increase the number of similar character names found)
		if (stricmp(char_dat[i].status.name, character_name) == 0) {
			// Strict comparison (if found, we finish the function immediatly with correct value)
			if (strcmp(char_dat[i].status.name, character_name) == 0)
				return i;
			quantity++;
			index = i;
		}
	}
	// Here, the exact character name is not found
	// We return the found index of a similar account ONLY if there is 1 similar character
	if (quantity == 1)
		return index;

	// Exact character name is not found and 0 or more than 1 similar characters have been found ==> we say not found
	return -1;
}

//-------------------------------------
// Return character name with the index
//-------------------------------------
char * search_character_name(int index) {

	if (index >= 0 && index < char_num)
		return char_dat[index].status.name;

	return unknown_char_name;
}

// Searches if the given character is online, and returns the fd of the
// map-server it is connected to.
int search_character_online(int aid, int cid)
{
	//Look for online character.
	struct online_char_data* character;
	character = idb_get(online_char_db, aid);
	if(character &&
		character->char_id == cid &&
		character->server > -1) 
		return server_fd[character->server];
	return -1;
}
static void * create_online_char_data(DBKey key, va_list args) {
	struct online_char_data* character;
	character = aCalloc(1, sizeof(struct online_char_data));
	character->account_id = key.i;
	character->char_id = -1;
  	character->server = -1;
	return character;
}

static int chardb_waiting_disconnect(int tid, unsigned int tick, int id, int data);

//-------------------------------------------------
// Set Character online/offline [Wizputer]
//-------------------------------------------------

void set_char_online(int map_id, int char_id, int account_id) {
	struct online_char_data* character;

	if ( char_id != 99 && (max_account_id < account_id || max_char_id < char_id))
	{	//Notify map-server of the new max IDs [Skotlex]
		if (account_id > max_account_id)
			max_account_id = account_id;
		if (char_id > max_char_id)
			max_char_id = char_id;
		mapif_send_maxid(max_account_id, max_char_id);
	}
	character = idb_ensure(online_char_db, account_id, create_online_char_data);
	if (online_check && character->char_id != -1 && character->server > -1 && character->server != map_id)
	{
		//char == 99 <- Character logging in, so someone has logged in while one
		//char is still on map-server, so kick him out, but don't print "error"
		//as this is normal behaviour. [Skotlex]
		if (char_id != 99)
			ShowNotice("set_char_online: Character %d:%d marked in map server %d, but map server %d claims to have (%d:%d) online!\n",
				character->account_id, character->char_id, character->server, map_id, account_id, char_id);
		mapif_disconnectplayer(server_fd[character->server], character->account_id, character->char_id, 2);
	}
	character->waiting_disconnect = 0;
	character->char_id = (char_id==99)?-1:char_id;
	character->server = (char_id==99)?-1:map_id;

	if (login_fd <= 0 || session[login_fd]->eof)
		return;
	WFIFOHEAD(login_fd, 6);
	WFIFOW(login_fd,0) = 0x272b;
	WFIFOL(login_fd,2) = account_id;
	WFIFOSET(login_fd,6);

	//printf ("set online\n");
}
void set_char_offline(int char_id, int account_id) {
	struct online_char_data* character;

	if ((character = idb_get(online_char_db, account_id)) != NULL)
	{	//We don't free yet to avoid aCalloc/aFree spamming during char change. [Skotlex]
		character->char_id = -1;
		character->server = -1;
		character->waiting_disconnect = 0;
	}
	if (login_fd <= 0 || session[login_fd]->eof)
		return;
	WFIFOHEAD(login_fd, 6);
	WFIFOW(login_fd,0) = 0x272c;
	WFIFOL(login_fd,2) = account_id;
	WFIFOSET(login_fd,6);

}

static int char_db_setoffline(DBKey key, void* data, va_list ap) {
	struct online_char_data* character = (struct online_char_data*)data;
	int server = va_arg(ap, int);
	if (server == -1) {
		character->char_id = -1;
		character->server = -1;
		character->waiting_disconnect = 0;	
	} else if (character->server == server)
		character->server = -2; //In some map server that we aren't connected to.
	return 0;
}

static int char_db_kickoffline(DBKey key, void* data, va_list ap) {
	struct online_char_data* character = (struct online_char_data*)data;
	int server = va_arg(ap, int);

	if (server > -1 && character->server != server)
		return 0;

	//Kick out any connected characters, and set them offline as appropiate.
	if (character->server > -1)
		mapif_disconnectplayer(server_fd[character->server],
			character->account_id, character->char_id, 1);
	else if (!character->waiting_disconnect)
		set_char_offline(character->char_id, character->account_id);
	else return 0;
	return 1;
}

void set_all_offline(int id) {
	if (id < 0)
		ShowNotice("Sending all users offline.\n");
	else
		ShowNotice("Sending users of map-server %d offline.\n", id);
	online_char_db->foreach(online_char_db,char_db_kickoffline,id);

	if (id >= 0 || login_fd <= 0 || session[login_fd]->eof)
		return;
	//Tell login-server to also mark all our characters as offline.
	WFIFOHEAD(login_fd, 2);
	WFIFOW(login_fd,0) = 0x2737;
	WFIFOSET(login_fd,2);
}

/*---------------------------------------------------
  Make a data line for friends list
 --------------------------------------------------*/

int mmo_friends_list_data_str(char *str, struct mmo_charstatus *p) {
	int i;
	char *str_p = str;
	str_p += sprintf(str_p, "%d", p->char_id);

	for (i=0;i<MAX_FRIENDS;i++){
		if (p->friends[i].account_id > 0 && p->friends[i].char_id > 0 && p->friends[i].name[0])
			str_p += sprintf(str_p, ",%d,%d,%s", p->friends[i].account_id, p->friends[i].char_id, p->friends[i].name);
	}

	str_p += '\0';

	return 0;
}

//-------------------------------------------------
// Function to create the character line (for save)
//-------------------------------------------------
int mmo_char_tostr(char *str, struct mmo_charstatus *p, struct global_reg *reg, int reg_num) {
	int i,j;
	char *str_p = str;

	/* We shouldn't need this anymore... [Skotlex]
	// on multi-map server, sometimes it's posssible that last_point become void. (reason???) We check that to not lost character at restart.
	if (!p->last_point.map) {
		p->last_point.map = mapindex_name2id(MAP_PRONTERA);
		p->last_point.x = 273;
		p->last_point.y = 354;
	}
	*/
	str_p += sprintf(str_p,
		"%d\t%d,%d\t%s\t%d,%d,%d\t%u,%u,%d" //Up to Zeny field
		"\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d" //Up to Skill Point
		"\t%d,%d,%d\t%d,%d,%d,%d" //Up to hom id
		"\t%d,%d,%d\t%d,%d,%d,%d,%d" //Up to head bottom
		"\t%d,%d,%d\t%d,%d,%d" //last point + save point
		",%d,%d,%d,%d,%d\t",	//Family info
		p->char_id, p->account_id, p->char_num, p->name, //
		p->class_, p->base_level, p->job_level,
		p->base_exp, p->job_exp, p->zeny,
		p->hp, p->max_hp, p->sp, p->max_sp,
		p->str, p->agi, p->vit, p->int_, p->dex, p->luk,
		p->status_point, p->skill_point,
		p->option, p->karma, p->manner,	//
		p->party_id, p->guild_id, p->pet_id, p->hom_id,
		p->hair, p->hair_color, p->clothes_color,
		p->weapon, p->shield, p->head_top, p->head_mid, p->head_bottom,
		p->last_point.map, p->last_point.x, p->last_point.y, //
		p->save_point.map, p->save_point.x, p->save_point.y,
		p->partner_id,p->father,p->mother,p->child,p->fame);
	for(i = 0; i < MAX_MEMOPOINTS; i++)
		if (p->memo_point[i].map) {
			str_p += sprintf(str_p, "%d,%d,%d", p->memo_point[i].map, p->memo_point[i].x, p->memo_point[i].y);
		}
	*(str_p++) = '\t';

	for(i = 0; i < MAX_INVENTORY; i++)
		if (p->inventory[i].nameid) {
			str_p += sprintf(str_p,"%d,%d,%d,%d,%d,%d,%d",
				p->inventory[i].id,p->inventory[i].nameid,p->inventory[i].amount,p->inventory[i].equip,
				p->inventory[i].identify,p->inventory[i].refine,p->inventory[i].attribute);
			for(j=0; j<MAX_SLOTS; j++)
				str_p += sprintf(str_p,",%d",p->inventory[i].card[j]);
			str_p += sprintf(str_p," ");
		}
	*(str_p++) = '\t';

	for(i = 0; i < MAX_CART; i++)
		if (p->cart[i].nameid) {
			str_p += sprintf(str_p,"%d,%d,%d,%d,%d,%d,%d",
				p->cart[i].id,p->cart[i].nameid,p->cart[i].amount,p->cart[i].equip,
				p->cart[i].identify,p->cart[i].refine,p->cart[i].attribute);
			for(j=0; j<MAX_SLOTS; j++)
				str_p += sprintf(str_p,",%d",p->cart[i].card[j]);
			str_p += sprintf(str_p," ");
		}
	*(str_p++) = '\t';

	for(i = 0; i < MAX_SKILL; i++)
		if (p->skill[i].id && p->skill[i].flag != 1) {
			str_p += sprintf(str_p, "%d,%d ", p->skill[i].id, (p->skill[i].flag == 0) ? p->skill[i].lv : p->skill[i].flag-2);
		}
	*(str_p++) = '\t';

	for(i = 0; i < reg_num; i++)
		if (reg[i].str[0])
			str_p += sprintf(str_p, "%s,%s ", reg[i].str, reg[i].value);
	*(str_p++) = '\t';

	*str_p = '\0';
	return 0;
}
#endif //TXT_SQL_CONVERT
//-------------------------------------------------------------------------
// Function to set the character from the line (at read of characters file)
//-------------------------------------------------------------------------
int mmo_char_fromstr(char *str, struct mmo_charstatus *p, struct global_reg *reg, int *reg_num) {
	char tmp_str[3][128]; //To avoid deleting chars with too long names.
	int tmp_int[256];
	unsigned int tmp_uint[2]; //To read exp....
	int next, len, i, j;

	// initilialise character
	memset(p, '\0', sizeof(struct mmo_charstatus));
	
// Char structure of version 1500 (homun + mapindex maps)
	if (sscanf(str, "%d\t%d,%d\t%127[^\t]\t%d,%d,%d\t%u,%u,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%d,%d,%d\t%d,%d,%d,%d,%d,%d,%d,%d%n",
		&tmp_int[0], &tmp_int[1], &tmp_int[2], tmp_str[0],
		&tmp_int[3], &tmp_int[4], &tmp_int[5],
		&tmp_uint[0], &tmp_uint[1], &tmp_int[8],
		&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		&tmp_int[19], &tmp_int[20],
		&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		&tmp_int[24], &tmp_int[25], &tmp_int[26], &tmp_int[44],
		&tmp_int[27], &tmp_int[28], &tmp_int[29],
		&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		&tmp_int[45], &tmp_int[35], &tmp_int[36],
		&tmp_int[46], &tmp_int[37], &tmp_int[38], &tmp_int[39], 
		&tmp_int[40], &tmp_int[41], &tmp_int[42], &tmp_int[43], &next) != 48)
	{
	tmp_int[44] = 0; //Hom ID.
// Char structure of version 1488 (fame field addition)
	if (sscanf(str, "%d\t%d,%d\t%127[^\t]\t%d,%d,%d\t%u,%u,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%127[^,],%d,%d\t%127[^,],%d,%d,%d,%d,%d,%d,%d%n",
		&tmp_int[0], &tmp_int[1], &tmp_int[2], tmp_str[0],
		&tmp_int[3], &tmp_int[4], &tmp_int[5],
		&tmp_uint[0], &tmp_uint[1], &tmp_int[8],
		&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		&tmp_int[19], &tmp_int[20],
		&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		&tmp_int[24], &tmp_int[25], &tmp_int[26],
		&tmp_int[27], &tmp_int[28], &tmp_int[29],
		&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		tmp_str[1], &tmp_int[35], &tmp_int[36],
		tmp_str[2], &tmp_int[37], &tmp_int[38], &tmp_int[39], 
		&tmp_int[40], &tmp_int[41], &tmp_int[42], &tmp_int[43], &next) != 47)
	{
	tmp_int[43] = 0; //Fame
// Char structure of version 1363 (family data addition)
	if (sscanf(str, "%d\t%d,%d\t%127[^\t]\t%d,%d,%d\t%u,%u,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%127[^,],%d,%d\t%127[^,],%d,%d,%d,%d,%d,%d%n",
		&tmp_int[0], &tmp_int[1], &tmp_int[2], tmp_str[0], //
		&tmp_int[3], &tmp_int[4], &tmp_int[5],
		&tmp_uint[0], &tmp_uint[1], &tmp_int[8],
		&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		&tmp_int[19], &tmp_int[20],
		&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		&tmp_int[24], &tmp_int[25], &tmp_int[26],
		&tmp_int[27], &tmp_int[28], &tmp_int[29],
		&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		tmp_str[1], &tmp_int[35], &tmp_int[36], //
		tmp_str[2], &tmp_int[37], &tmp_int[38], &tmp_int[39], 
		&tmp_int[40], &tmp_int[41], &tmp_int[42], &next) != 46)
	{
	tmp_int[40] = 0; // father
	tmp_int[41] = 0; // mother
	tmp_int[42] = 0; // child
// Char structure version 1008 (marriage partner addition)
	if (sscanf(str, "%d\t%d,%d\t%127[^\t]\t%d,%d,%d\t%u,%u,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%127[^,],%d,%d\t%127[^,],%d,%d,%d%n",
		&tmp_int[0], &tmp_int[1], &tmp_int[2], tmp_str[0], //
		&tmp_int[3], &tmp_int[4], &tmp_int[5],
		&tmp_uint[0], &tmp_uint[1], &tmp_int[8],
		&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		&tmp_int[19], &tmp_int[20],
		&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		&tmp_int[24], &tmp_int[25], &tmp_int[26],
		&tmp_int[27], &tmp_int[28], &tmp_int[29],
		&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		tmp_str[1], &tmp_int[35], &tmp_int[36], //
		tmp_str[2], &tmp_int[37], &tmp_int[38], &tmp_int[39], &next) != 43)
	{
	tmp_int[39] = 0; // partner id
// Char structure version 384 (pet addition)
	if (sscanf(str, "%d\t%d,%d\t%127[^\t]\t%d,%d,%d\t%u,%u,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%127[^,],%d,%d\t%127[^,],%d,%d%n",
		&tmp_int[0], &tmp_int[1], &tmp_int[2], tmp_str[0], //
		&tmp_int[3], &tmp_int[4], &tmp_int[5],
		&tmp_uint[0], &tmp_uint[1], &tmp_int[8],
		&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		&tmp_int[19], &tmp_int[20],
		&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		&tmp_int[24], &tmp_int[25], &tmp_int[26],
		&tmp_int[27], &tmp_int[28], &tmp_int[29],
		&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		tmp_str[1], &tmp_int[35], &tmp_int[36], //
		tmp_str[2], &tmp_int[37], &tmp_int[38], &next) != 42)
	{
	tmp_int[26] = 0; // pet id
// Char structure of a version 1 (original data structure)
	if (sscanf(str, "%d\t%d,%d\t%127[^\t]\t%d,%d,%d\t%u,%u,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%d,%d,%d\t%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		"\t%127[^,],%d,%d\t%127[^,],%d,%d%n",
		&tmp_int[0], &tmp_int[1], &tmp_int[2], tmp_str[0], //
		&tmp_int[3], &tmp_int[4], &tmp_int[5],
		&tmp_uint[0], &tmp_uint[1], &tmp_int[8],
		&tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		&tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		&tmp_int[19], &tmp_int[20],
		&tmp_int[21], &tmp_int[22], &tmp_int[23], //
		&tmp_int[24], &tmp_int[25], //
		&tmp_int[27], &tmp_int[28], &tmp_int[29],
		&tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		tmp_str[1], &tmp_int[35], &tmp_int[36], //
		tmp_str[2], &tmp_int[37], &tmp_int[38], &next) != 41)
	{
		ShowError("Char-loading: Unrecognized character data version, info lost!\n");
		ShowDebug("Character info: %s\n", str);
		return 0;
	}
	}	// Char structure version 384 (pet addition)
	}	// Char structure version 1008 (marriage partner addition)
	}	// Char structure of version 1363 (family data addition)
	}	// Char structure of version 1488 (fame field addition)
	//Convert save data from string to integer for older formats
		tmp_int[45] = mapindex_name2id(tmp_str[1]);
		tmp_int[46] = mapindex_name2id(tmp_str[2]);
	}	// Char structure of version 1500 (homun + mapindex maps)

	memcpy(p->name, tmp_str[0], NAME_LENGTH-1); //Overflow protection [Skotlex]
	p->char_id = tmp_int[0];
	p->account_id = tmp_int[1];
	p->char_num = tmp_int[2];
	p->class_ = tmp_int[3];
/* Unneeded unless you are running a real old character database now.
	//Temporal fix until all chars are reverted from peco-flying-class to 
	//normal classes. [Skotlex]
	switch (p->class_) {
		case JOB_KNIGHT2: //Job_Knight2
			p->class_ = JOB_KNIGHT;
			break;
		case JOB_CRUSADER2: //Job_Crusader2
			p->class_ = JOB_CRUSADER;
			break;
		case JOB_LORD_KNIGHT2: //Job_Lord_Knight2
			p->class_	= JOB_LORD_KNIGHT;
			break;
		case JOB_PALADIN2: //Job_Paladin2
			p->class_ = JOB_PALADIN;
			break;
		case JOB_BABY_KNIGHT2: //Job_Baby_Knight2
			p->class_ = JOB_BABY_KNIGHT;
			break;
		case JOB_BABY_CRUSADER2: //Job_Baby_Crusader2
			p->class_ = JOB_BABY_CRUSADER;
			break;
		case JOB_STAR_GLADIATOR2: //Job_Star_Gladiator2
			p->class_ = JOB_STAR_GLADIATOR;
			break;
	}
*/
	p->base_level = tmp_int[4];
	p->job_level = tmp_int[5];
	p->base_exp = tmp_uint[0];
	p->job_exp = tmp_uint[1];
	p->zeny = tmp_int[8];
	p->hp = tmp_int[9];
	p->max_hp = tmp_int[10];
	p->sp = tmp_int[11];
	p->max_sp = tmp_int[12];
	p->str = tmp_int[13];
	p->agi = tmp_int[14];
	p->vit = tmp_int[15];
	p->int_ = tmp_int[16];
	p->dex = tmp_int[17];
	p->luk = tmp_int[18];
	p->status_point = tmp_int[19] > USHRT_MAX ? USHRT_MAX : tmp_int[19];
	p->skill_point = tmp_int[20] > USHRT_MAX ? USHRT_MAX : tmp_int[20];
	p->option = tmp_int[21];
	p->karma = tmp_int[22];
	p->manner = tmp_int[23];
	p->party_id = tmp_int[24];
	p->guild_id = tmp_int[25];
	p->pet_id = tmp_int[26];
	p->hair = tmp_int[27];
	p->hair_color = tmp_int[28];
	p->clothes_color = tmp_int[29];
	p->weapon = tmp_int[30];
	p->shield = tmp_int[31];
	p->head_top = tmp_int[32];
	p->head_mid = tmp_int[33];
	p->head_bottom = tmp_int[34];
	p->last_point.x = tmp_int[35];
	p->last_point.y = tmp_int[36];
	p->save_point.x = tmp_int[37];
	p->save_point.y = tmp_int[38];
	p->partner_id = tmp_int[39];
	p->father = tmp_int[40];
	p->mother = tmp_int[41];
	p->child = tmp_int[42];
	p->fame = tmp_int[43];
	p->hom_id = tmp_int[44];
	p->last_point.map = tmp_int[45];
	p->save_point.map = tmp_int[46];

#ifndef TXT_SQL_CONVERT
	// Some checks
	for(i = 0; i < char_num; i++) {
		if (char_dat[i].status.char_id == p->char_id) {
			ShowError(CL_RED"mmmo_auth_init: a character has an identical id to another.\n");
			ShowError("               character id #%d -> new character not readed.\n", p->char_id);
			ShowError("               Character saved in log file."CL_RESET"\n");
			return -1;
		} else if (strcmp(char_dat[i].status.name, p->name) == 0) {
			ShowError(CL_RED"mmmo_auth_init: a character name already exists.\n");
			ShowError("               character name '%s' -> new character not read.\n", p->name);
			ShowError("               Character saved in log file."CL_RESET"\n");
			return -2;
		}
	}

	if (strcmpi(wisp_server_name, p->name) == 0) {
		ShowWarning("mmo_auth_init: ******WARNING: character name has wisp server name.\n");
		ShowWarning("               Character name '%s' = wisp server name '%s'.\n", p->name, wisp_server_name);
		ShowWarning("               Character readed. Suggestion: change the wisp server name.\n");
		char_log("mmo_auth_init: ******WARNING: character name has wisp server name: Character name '%s' = wisp server name '%s'." RETCODE,
		          p->name, wisp_server_name);
	}
#endif //TXT_SQL_CONVERT
	if (str[next] == '\n' || str[next] == '\r')
		return 1;	// 新規データ

	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		//mapindex memo format
		if (sscanf(str+next, "%d,%d,%d%n", &tmp_int[2], &tmp_int[0], &tmp_int[1], &len) != 3)
		{	//Old string-based memo format.
			if (sscanf(str+next, "%[^,],%d,%d%n", tmp_str[0], &tmp_int[0], &tmp_int[1], &len) != 3)
				return -3;
			tmp_int[2] = mapindex_name2id(tmp_str[0]);
		}
		if (i < MAX_MEMOPOINTS)
	  	{	//Avoid overflowing (but we must also read through all saved memos)
			p->memo_point[i].x = tmp_int[0];
			p->memo_point[i].y = tmp_int[1];
			p->memo_point[i].map = tmp_int[2];
		}
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		if(sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d%[0-9,-]%n",
		      &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		      &tmp_int[4], &tmp_int[5], &tmp_int[6], tmp_str[0], &len) == 8)
		{
			p->inventory[i].id = tmp_int[0];
			p->inventory[i].nameid = tmp_int[1];
			p->inventory[i].amount = tmp_int[2];
			p->inventory[i].equip = tmp_int[3];
			p->inventory[i].identify = tmp_int[4];
			p->inventory[i].refine = tmp_int[5];
			p->inventory[i].attribute = tmp_int[6];

			for(j = 0; j < MAX_SLOTS && tmp_str[0] && sscanf(tmp_str[0], ",%d%[0-9,-]",&tmp_int[0], tmp_str[0]) > 0; j++)
				p->inventory[i].card[j] = tmp_int[0];

			next += len;
			if (str[next] == ' ')
				next++;
		} else // invalid structure
			return -4;
	}
	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		if(sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d%[0-9,-]%n",
		      &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		      &tmp_int[4], &tmp_int[5], &tmp_int[6], tmp_str[0], &len) == 8)
		{
			p->cart[i].id = tmp_int[0];
			p->cart[i].nameid = tmp_int[1];
			p->cart[i].amount = tmp_int[2];
			p->cart[i].equip = tmp_int[3];
			p->cart[i].identify = tmp_int[4];
			p->cart[i].refine = tmp_int[5];
			p->cart[i].attribute = tmp_int[6];
			
			for(j = 0; j < MAX_SLOTS && tmp_str && sscanf(tmp_str[0], ",%d%[0-9,-]",&tmp_int[0], tmp_str[0]) > 0; j++)
				p->cart[i].card[j] = tmp_int[0];
			
			next += len;
			if (str[next] == ' ')
				next++;
		} else // invalid structure
			return -5;
	}

	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		if (sscanf(str + next, "%d,%d%n", &tmp_int[0], &tmp_int[1], &len) != 2)
			return -6;
		p->skill[tmp_int[0]].id = tmp_int[0];
		p->skill[tmp_int[0]].lv = tmp_int[1];
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	for(i = 0; str[next] && str[next] != '\t' && str[next] != '\n' && str[next] != '\r'; i++) { // global_reg実装以前のathena.txt互換のため一応'\n'チェック
		if (sscanf(str + next, "%[^,],%[^ ] %n", reg[i].str, reg[i].value, &len) != 2) { 
			// because some scripts are not correct, the str can be "". So, we must check that.
			// If it's, we must not refuse the character, but just this REG value.
			// Character line will have something like: nov_2nd_cos,9 ,9 nov_1_2_cos_c,1 (here, ,9 is not good)
			if (str[next] == ',' && sscanf(str + next, ",%[^ ] %n", reg[i].value, &len) == 1) 
				i--;
			else
				return -7;
		}
		next += len;
		if (str[next] == ' ')
			next++;
	}
	*reg_num = i;

	return 1;
}
//---------------------------------
// Function to read friend list
//---------------------------------

int parse_friend_txt(struct mmo_charstatus *p)
{
	char line[1024], temp[1024];
	int pos = 0, count = 0, next;
	int i,len;
	FILE *fp;

	// Open the file and look for the ID
	fp = fopen(friends_txt, "r");

	if(fp == NULL)
		return -1;
	
	while(fgets(line, sizeof(line)-1, fp)) {

		if(line[0] == '/' && line[1] == '/')
			continue;
		if (sscanf(line, "%d%n",&i, &pos) < 1 || i != p->char_id)
			continue; //Not this line...
		//Read friends
		len = strlen(line);
		next = pos;
		for (count = 0; next < len && count < MAX_FRIENDS; count++)
		{ //Read friends.
			if (sscanf(line+next, ",%d,%d,%23[^,^\n]%n",&p->friends[count].account_id,&p->friends[count].char_id, p->friends[count].name, &pos) < 3)
			{	//Invalid friend?
				memset(&p->friends[count], 0, sizeof(p->friends[count]));
				break;
			}
			next+=pos;
			//What IF the name contains a comma? while the next field is not a 
			//number, we assume it belongs to the current name. [Skotlex]
			//NOTE: Of course, this will fail if someone sets their name to something like
			//Bob,2005 but... meh, it's the problem of parsing a text file (encasing it in "
			//won't do as quotes are also valid name chars!)
			while(next < len && sscanf(line+next, ",%23[^,^\n]%n", temp, &pos) > 0)
			{
				if (atoi(temp)) //We read the next friend, just continue.
					break;
				//Append the name.
				next+=pos;
				i = strlen(p->friends[count].name);
				if (i + strlen(temp) +1 < NAME_LENGTH)
				{
					p->friends[count].name[i] = ',';
					strcpy(p->friends[count].name+i+1, temp);
				}
			} //End Guess Block
		} //Friend's for.
		break; //Found friends.
	}
	fclose(fp);
	return count;
}
#ifndef TXT_SQL_CONVERT
//---------------------------------
// Function to read characters file
//---------------------------------
int mmo_char_init(void) {
	char line[65536];
	int ret, line_count;
	FILE *fp;

	char_max = 256;
	char_dat = (struct character_data*)aCalloc(sizeof(struct character_data) * 256, 1);
	if (!char_dat) {
		ShowFatalError("out of memory: mmo_char_init (calloc of char_dat).\n");
		exit(1);
	}
	char_num = 0;

	fp = fopen(char_txt, "r");

	if (fp == NULL) {
		ShowError("Characters file not found: %s.\n", char_txt);
		char_log("Characters file not found: %s." RETCODE, char_txt);
		char_log("Id for the next created character: %d." RETCODE, char_id_count);
		return 0;
	}

	line_count = 0;
	while(fgets(line, sizeof(line)-1, fp)) {
		int i, j;
		line_count++;

		if (line[0] == '/' && line[1] == '/')
			continue;
		line[sizeof(line)-1] = '\0';

		j = 0;
		if (sscanf(line, "%d\t%%newid%%%n", &i, &j) == 1 && j > 0) {
			if (char_id_count < i)
				char_id_count = i;
			continue;
		}

		if (char_num >= char_max) {
			char_max += 256;
			char_dat = (struct character_data*)aRealloc(char_dat, sizeof(struct character_data) * char_max);
			if (!char_dat) {
				ShowFatalError("Out of memory: mmo_char_init (realloc of char_dat).\n");
				char_log("Out of memory: mmo_char_init (realloc of char_dat)." RETCODE);
				exit(1);
			}
		}

		ret = mmo_char_fromstr(line, &char_dat[char_num].status, char_dat[char_num].global, &char_dat[char_num].global_num);

		// Initialize friends list
		parse_friend_txt(&char_dat[char_num].status);  // Grab friends for the character

		if (ret > 0) { // negative value or zero for errors
			if (char_dat[char_num].status.char_id >= char_id_count)
				char_id_count = char_dat[char_num].status.char_id + 1;
			char_num++;
		} else {
			ShowError("mmo_char_init: in characters file, unable to read the line #%d.\n", line_count);
			ShowError("               -> Character saved in log file.\n");
			switch (ret) {
			case -1:
				char_log("Duplicate character id in the next character line (character not readed):" RETCODE);
				break;
			case -2:
				char_log("Duplicate character name in the next character line (character not readed):" RETCODE);
				break;
			case -3:
				char_log("Invalid memo point structure in the next character line (character not readed):" RETCODE);
				break;
			case -4:
				char_log("Invalid inventory item structure in the next character line (character not readed):" RETCODE);
				break;
			case -5:
				char_log("Invalid cart item structure in the next character line (character not readed):" RETCODE);
				break;
			case -6:
				char_log("Invalid skill structure in the next character line (character not readed):" RETCODE);
				break;
			case -7:
				char_log("Invalid register structure in the next character line (character not readed):" RETCODE);
				break;
			default: // 0
				char_log("Unabled to get a character in the next line - Basic structure of line (before inventory) is incorrect (character not readed):" RETCODE);
				break;
			}
			char_log("%s", line);
		}
	}
	fclose(fp);

	if (char_num == 0) {
		ShowNotice("mmo_char_init: No character found in %s.\n", char_txt);
		char_log("mmo_char_init: No character found in %s." RETCODE, char_txt);
	} else if (char_num == 1) {
		ShowStatus("mmo_char_init: 1 character read in %s.\n", char_txt);
		char_log("mmo_char_init: 1 character read in %s." RETCODE, char_txt);
	} else {
		ShowStatus("mmo_char_init: %d characters read in %s.\n", char_num, char_txt);
		char_log("mmo_char_init: %d characters read in %s." RETCODE, char_num, char_txt);
	}

	char_log("Id for the next created character: %d." RETCODE, char_id_count);

	return 0;
}

//---------------------------------------------------------
// Function to save characters in files (speed up by [Yor])
//---------------------------------------------------------
void mmo_char_sync(void) {
	char line[65536],f_line[1024];
	int i, j, k;
	int lock;
	FILE *fp,*f_fp;
	//int *id = (int *) aMalloc(sizeof(int) * char_num);
	CREATE_BUFFER(id, int, char_num);

	// Sorting before save (by [Yor])
	for(i = 0; i < char_num; i++) {
		id[i] = i;
		for(j = 0; j < i; j++) {
			if ((char_dat[i].status.account_id < char_dat[id[j]].status.account_id) ||
			    // if same account id, we sort by slot.
			    (char_dat[i].status.account_id == char_dat[id[j]].status.account_id &&
			     char_dat[i].status.char_num < char_dat[id[j]].status.char_num)) {
				for(k = i; k > j; k--)
					id[k] = id[k-1];
				id[j] = i; // id[i]
				break;
			}
		}
	}

	// Data save
	fp = lock_fopen(char_txt, &lock);
	if (fp == NULL) {
		ShowWarning("Server can't not save characters.\n");
		char_log("WARNING: Server can't not save characters." RETCODE);
	} else {
		for(i = 0; i < char_num; i++) {
			// create only once the line, and save it in the 2 files (it's speeder than repeat twice the loop and create twice the line)
			mmo_char_tostr(line, &char_dat[id[i]].status, char_dat[id[i]].global, char_dat[id[i]].global_num); // use of sorted index
				fprintf(fp, "%s" RETCODE, line);
		}
		fprintf(fp, "%d\t%%newid%%" RETCODE, char_id_count);
		lock_fclose(fp, char_txt, &lock);
	}

	// Data save (backup)
	if (backup_txt_flag) { // The backup_txt file was created because char deletion bug existed. Now it's finish and that take a lot of time to create a second file when there are a lot of characters. => option By [Yor]
		fp = lock_fopen(backup_txt, &lock);
		if (fp == NULL) {
			ShowWarning("Server can't not create backup of characters file.\n");
			char_log("WARNING: Server can't not create backup of characters file." RETCODE);
			//aFree(id); // free up the memory before leaving -.- [Ajarn]
			DELETE_BUFFER(id);
			return;
		}
		for(i = 0; i < char_num; i++) {
			// create only once the line, and save it in the 2 files (it's speeder than repeat twice the loop and create twice the line)
			mmo_char_tostr(line, &char_dat[id[i]].status,char_dat[id[i]].global, char_dat[id[i]].global_num); // use of sorted index
				fprintf(fp, "%s" RETCODE, line);
		}
		fprintf(fp, "%d\t%%newid%%" RETCODE, char_id_count);
		lock_fclose(fp, backup_txt, &lock);
	}

	// Friends List data save (davidsiaw)
	f_fp = lock_fopen(friends_txt, &lock);
	for(i = 0; i < char_num; i++) {
		mmo_friends_list_data_str(f_line, &char_dat[id[i]].status);
		fprintf(f_fp, "%s" RETCODE, f_line);
	}

	lock_fclose(f_fp, friends_txt, &lock);

	//aFree(id);
	DELETE_BUFFER(id);

	return;
}

//----------------------------------------------------
// Function to save (in a periodic way) datas in files
//----------------------------------------------------
int mmo_char_sync_timer(int tid, unsigned int tick, int id, int data) {
	if (save_log)
		ShowInfo("Saving all files...\n");
	mmo_char_sync();
	inter_save();
	return 0;
}

//-----------------------------------
// Function to create a new character
//-----------------------------------
int make_new_char(int fd, unsigned char *dat) {
	int i;
	struct char_session_data *sd;
	char name[NAME_LENGTH];
	
	sd = (struct char_session_data*)session[fd]->session_data;

	// remove control characters from the name
	strncpy(name, dat, NAME_LENGTH);
	name[NAME_LENGTH-1] = '\0'; //Trunc name to max possible value (23)
	
	trim(name,TRIM_CHARS); //Trim character name. [Skotlex]
	
	//check name != main chat nick [LuzZza]
	if(strcmpi(name, main_chat_nick) == 0) {
		char_log("Create char failed (%d): this nick (%s) reserved for mainchat messages." RETCODE,
			sd->account_id, name);
		return -1;
	}	
	
	if (remove_control_chars((unsigned char *)name)) {
		char_log("Make new char error (control char received in the name): (connection #%d, account: %d)." RETCODE,
		         fd, sd->account_id);
		return -1;
	}

	// check lenght of character name
	if (strlen(name) < 4) {
		char_log("Make new char error (character name too small): (connection #%d, account: %d, name: '%s')." RETCODE,
		         fd, sd->account_id, dat);
		return -1;
	}

	// Check Authorised letters/symbols in the name of the character
	if (char_name_option == 1) { // only letters/symbols in char_name_letters are authorised
		for (i = 0;  i < NAME_LENGTH && name[i]; i++)
			if (strchr(char_name_letters, name[i]) == NULL) {
				char_log("Make new char error (invalid letter in the name): (connection #%d, account: %d), name: %s, invalid letter: %c." RETCODE,
				         fd, sd->account_id, name, name[i]);
				return -1;
			}
	} else if (char_name_option == 2) { // letters/symbols in char_name_letters are forbidden
		for (i = 0;  i < NAME_LENGTH && name[i]; i++)
			if (strchr(char_name_letters, name[i]) != NULL) {
				char_log("Make new char error (invalid letter in the name): (connection #%d, account: %d), name: %s, invalid letter: %c." RETCODE,
				         fd, sd->account_id, dat, dat[i]);
				return -1;
			}
	} // else, all letters/symbols are authorised (except control char removed before)

	if (dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29] != 5*6 || // stats
	    dat[30] >= 9 || // slots (dat[30] can not be negativ)
	    dat[33] <= 0 || dat[33] >= 24 || // hair style
	    dat[31] >= 9) { // hair color (dat[31] can not be negativ)
		char_log("Make new char error (invalid values): (connection #%d, account: %d) slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d" RETCODE,
		         fd, sd->account_id, dat[30], dat, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);
		return -1;
	}

	// check individual stat value
	for(i = 24; i <= 29; i++) {
		if (dat[i] < 1 || dat[i] > 9) {
			char_log("Make new char error (invalid stat value: not between 1 to 9): (connection #%d, account: %d) slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d" RETCODE,
			         fd, sd->account_id, dat[30], dat, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);
			return -1;
		}
	} // now we know that every stat has proper value but we have to check if str/int agi/luk vit/dex pairs are correct

	if( ((dat[24]+dat[27]) > 10) || ((dat[25]+dat[29]) > 10) || ((dat[26]+dat[28]) > 10) ) {
		if (log_char) {
			char_log("Make new char error (invalid stat value): (connection #%d, account: %d) slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d" RETCODE,
			         fd, sd->account_id, dat[30], dat, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);
			return -1;
		}
	} // now when we have passed all stat checks

	for(i = 0; i < char_num; i++) {
		if ((name_ignoring_case != 0 && strncmp(char_dat[i].status.name, name, NAME_LENGTH) == 0) ||
			(name_ignoring_case == 0 && strncmpi(char_dat[i].status.name, name, NAME_LENGTH) == 0)) {
			char_log("Make new char error (name already exists): (connection #%d, account: %d) slot %d, name: %s (actual name of other char: %d), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d." RETCODE,
			         fd, sd->account_id, dat[30], dat, char_dat[i].status.name, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);
			return -1;
		}
		if (char_dat[i].status.account_id == sd->account_id && char_dat[i].status.char_num == dat[30]) {
			char_log("Make new char error (slot already used): (connection #%d, account: %d) slot %d, name: %s (actual name of other char: %d), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d." RETCODE,
			         fd, sd->account_id, dat[30], dat, char_dat[i].status.name, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);
			return -1;
		}
	}

	if (strcmp(wisp_server_name, name) == 0) {
		char_log("Make new char error (name used is wisp name for server): (connection #%d, account: %d) slot %d, name: %s (actual name of other char: %d), stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d." RETCODE,
		         fd, sd->account_id, dat[30], name, char_dat[i].status.name, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);
		return -1;
	}

	if (char_num >= char_max) {
		char_max += 256;
		char_dat = (struct character_data*)aRealloc(char_dat, sizeof(struct character_data) * char_max);
		if (!char_dat) {
			ShowFatalError("Out of memory: make_new_char (realloc of char_dat).\n");
			char_log("Out of memory: make_new_char (realloc of char_dat)." RETCODE);
			exit(1);
		}
	}

	char_log("Creation of New Character: (connection #%d, account: %d) slot %d, character Name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d." RETCODE,
	         fd, sd->account_id, dat[30], name, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);

	memset(&char_dat[i], 0, sizeof(struct character_data));

	char_dat[i].status.char_id = char_id_count++;
	char_dat[i].status.account_id = sd->account_id;
	char_dat[i].status.char_num = dat[30];
	strcpy(char_dat[i].status.name,name);
	char_dat[i].status.class_ = 0;
	char_dat[i].status.base_level = 1;
	char_dat[i].status.job_level = 1;
	char_dat[i].status.base_exp = 0;
	char_dat[i].status.job_exp = 0;
	char_dat[i].status.zeny = start_zeny;
	char_dat[i].status.str = dat[24];
	char_dat[i].status.agi = dat[25];
	char_dat[i].status.vit = dat[26];
	char_dat[i].status.int_ = dat[27];
	char_dat[i].status.dex = dat[28];
	char_dat[i].status.luk = dat[29];
	char_dat[i].status.max_hp = 40 * (100 + char_dat[i].status.vit) / 100;
	char_dat[i].status.max_sp = 11 * (100 + char_dat[i].status.int_) / 100;
	char_dat[i].status.hp = char_dat[i].status.max_hp;
	char_dat[i].status.sp = char_dat[i].status.max_sp;
	char_dat[i].status.status_point = 0;
	char_dat[i].status.skill_point = 0;
	char_dat[i].status.option = 0;
	char_dat[i].status.karma = 0;
	char_dat[i].status.manner = 0;
	char_dat[i].status.party_id = 0;
	char_dat[i].status.guild_id = 0;
	char_dat[i].status.hair = dat[33];
	char_dat[i].status.hair_color = dat[31];
	char_dat[i].status.clothes_color = 0;
	char_dat[i].status.inventory[0].nameid = start_weapon; // Knife
	char_dat[i].status.inventory[0].amount = 1;
	char_dat[i].status.inventory[0].equip = 0x02;
	char_dat[i].status.inventory[0].identify = 1;
	char_dat[i].status.inventory[1].nameid = start_armor; // Cotton Shirt
	char_dat[i].status.inventory[1].amount = 1;
	char_dat[i].status.inventory[1].equip = 0x10;
	char_dat[i].status.inventory[1].identify = 1;
	char_dat[i].status.weapon = 1;
	char_dat[i].status.shield = 0;
	char_dat[i].status.head_top = 0;
	char_dat[i].status.head_mid = 0;
	char_dat[i].status.head_bottom = 0;
	memcpy(&char_dat[i].status.last_point, &start_point, sizeof(start_point));
	memcpy(&char_dat[i].status.save_point, &start_point, sizeof(start_point));
	char_num++;

	mmo_char_sync();
	return i;
}

//----------------------------------------------------
// This function return the name of the job (by [Yor])
//----------------------------------------------------
char * job_name(int class_) {
	switch (class_) {
	case JOB_NOVICE:    return "Novice";
	case JOB_SWORDMAN:    return "Swordsman";
	case JOB_MAGE:    return "Mage";
	case JOB_ARCHER:    return "Archer";
	case JOB_ACOLYTE:    return "Acolyte";
	case JOB_MERCHANT:    return "Merchant";
	case JOB_THIEF:    return "Thief";
	case JOB_KNIGHT:    return "Knight";
	case JOB_PRIEST:    return "Priest";
	case JOB_WIZARD:    return "Wizard";
	case JOB_BLACKSMITH:   return "Blacksmith";
	case JOB_HUNTER:   return "Hunter";
	case JOB_ASSASSIN:   return "Assassin";
	case JOB_KNIGHT2:   return "Peco-Knight";
	case JOB_CRUSADER:   return "Crusader";
	case JOB_MONK:   return "Monk";
	case JOB_SAGE:   return "Sage";
	case JOB_ROGUE:   return "Rogue";
	case JOB_ALCHEMIST:   return "Alchemist";
	case JOB_BARD:   return "Bard";
	case JOB_DANCER:   return "Dancer";
	case JOB_CRUSADER2:   return "Peco-Crusader";
	case JOB_WEDDING:   return "Wedding";
	case JOB_SUPER_NOVICE:   return "Super Novice";
	case JOB_GUNSLINGER: return "Gunslinger";
	case JOB_NINJA: return "Ninja";
	case JOB_XMAS: return "Christmas";
	case JOB_NOVICE_HIGH: return "Novice High";
	case JOB_SWORDMAN_HIGH: return "Swordsman High";
	case JOB_MAGE_HIGH: return "Mage High";
	case JOB_ARCHER_HIGH: return "Archer High";
	case JOB_ACOLYTE_HIGH: return "Acolyte High";
	case JOB_MERCHANT_HIGH: return "Merchant High";
	case JOB_THIEF_HIGH: return "Thief High";
	case JOB_LORD_KNIGHT: return "Lord Knight";
	case JOB_HIGH_PRIEST: return "High Priest";
	case JOB_HIGH_WIZARD: return "High Wizard";
	case JOB_WHITESMITH: return "Whitesmith";
	case JOB_SNIPER: return "Sniper";
	case JOB_ASSASSIN_CROSS: return "Assassin Cross";
	case JOB_LORD_KNIGHT2: return "Peko Knight";
	case JOB_PALADIN: return "Paladin";
	case JOB_CHAMPION: return "Champion";
	case JOB_PROFESSOR: return "Professor";
	case JOB_STALKER: return "Stalker";
	case JOB_CREATOR: return "Creator";
	case JOB_CLOWN: return "Clown";
	case JOB_GYPSY: return "Gypsy";
	case JOB_PALADIN2: return "Peko Paladin";
	case JOB_BABY: return "Baby Novice";
	case JOB_BABY_SWORDMAN: return "Baby Swordsman";
	case JOB_BABY_MAGE: return "Baby Mage";
	case JOB_BABY_ARCHER: return "Baby Archer";
	case JOB_BABY_ACOLYTE: return "Baby Acolyte";
	case JOB_BABY_MERCHANT: return "Baby Merchant";
	case JOB_BABY_THIEF: return "Baby Thief";
	case JOB_BABY_KNIGHT: return "Baby Knight";
	case JOB_BABY_PRIEST: return "Baby Priest";
	case JOB_BABY_WIZARD: return "Baby Wizard";
	case JOB_BABY_BLACKSMITH: return "Baby Blacksmith";
	case JOB_BABY_HUNTER: return "Baby Hunter";
	case JOB_BABY_ASSASSIN: return "Baby Assassin";
	case JOB_BABY_KNIGHT2: return "Baby Peco Knight";
	case JOB_BABY_CRUSADER: return "Baby Crusader";
	case JOB_BABY_MONK: return "Baby Monk";
	case JOB_BABY_SAGE: return "Baby Sage";
	case JOB_BABY_ROGUE: return "Baby Rogue";
	case JOB_BABY_ALCHEMIST: return "Baby Alchemist";
	case JOB_BABY_BARD: return "Baby Bard";
	case JOB_BABY_DANCER: return "Baby Dancer";
	case JOB_BABY_CRUSADER2: return "Baby Peco Crusader";
	case JOB_SUPER_BABY: return "Super Baby";
	case JOB_TAEKWON: return "Taekwon";
	case JOB_STAR_GLADIATOR: return "Star Gladiator";
	case JOB_STAR_GLADIATOR2: return "Flying Star Gladiator";
	case JOB_SOUL_LINKER: return "Soul Linker";
	}
	return "Unknown Job";
}

static int create_online_files_sub(DBKey key, void* data, va_list va)
{
	struct online_char_data *character;
	int* players;
	int *id;
	int j,k,l;
	character = (struct online_char_data*) data;
	players = va_arg(va, int*);
	id = va_arg(va, int*);
	
	// check if map-server is online
	if (character->server == -1 || character->char_id == -1) { //Character not currently online.
		return -1;
	}
	
	j = character->server;
	if (server_fd[j] < 0) {
		server[j].users = 0;
		return -1;
	}
	// search position of character in char_dat and sort online characters.
	for(j = 0; j < char_num; j++) {
		if (char_dat[j].status.char_id != character->char_id)
			continue;
		id[*players] = j;
		// use sorting option
		switch (online_sorting_option) {
		case 1: // by name (without case sensitive)
			for(k = 0; k < *players; k++)
				if (stricmp(char_dat[j].status.name, char_dat[id[k]].status.name) < 0 ||
					// if same name, we sort with case sensitive.
					(stricmp(char_dat[j].status.name, char_dat[id[k]].status.name) == 0 &&
					 strcmp(char_dat[j].status.name, char_dat[id[k]].status.name) < 0)) {
					for(l = *players; l > k; l--)
						id[l] = id[l-1];
					id[k] = j; // id[*players]
					break;
				}
			break;
		case 2: // by zeny
			for(k = 0; k < *players; k++)
				if (char_dat[j].status.zeny < char_dat[id[k]].status.zeny ||
					// if same number of zenys, we sort by name.
					(char_dat[j].status.zeny == char_dat[id[k]].status.zeny &&
					 stricmp(char_dat[j].status.name, char_dat[id[k]].status.name) < 0)) {
					for(l = *players; l > k; l--)
						id[l] = id[l-1];
					id[k] = j; // id[*players]
					break;
				}
			break;
		case 3: // by base level
			for(k = 0; k < *players; k++)
				if (char_dat[j].status.base_level < char_dat[id[k]].status.base_level ||
					// if same base level, we sort by base exp.
					(char_dat[j].status.base_level == char_dat[id[k]].status.base_level &&
					 char_dat[j].status.base_exp < char_dat[id[k]].status.base_exp)) {
					for(l = *players; l > k; l--)
						id[l] = id[l-1];
					id[k] = j; // id[*players]
					break;
				}
			break;
		case 4: // by job (and job level)
			for(k = 0; k < *players; k++)
				if (char_dat[j].status.class_ < char_dat[id[k]].status.class_ ||
					// if same job, we sort by job level.
					(char_dat[j].status.class_ == char_dat[id[k]].status.class_ &&
					 char_dat[j].status.job_level < char_dat[id[k]].status.job_level) ||
					// if same job and job level, we sort by job exp.
					(char_dat[j].status.class_ == char_dat[id[k]].status.class_ &&
					 char_dat[j].status.job_level == char_dat[id[k]].status.job_level &&
					 char_dat[j].status.job_exp < char_dat[id[k]].status.job_exp)) {
					for(l = *players; l > k; l--)
						id[l] = id[l-1];
					id[k] = j; // id[*players]
					break;
				}
			break;
		case 5: // by location map name
		{
			const char *map1, *map2;
			map1 = mapindex_id2name(char_dat[j].status.last_point.map);
			
			for(k = 0; k < *players; k++) {
				map2 = mapindex_id2name(char_dat[id[k]].status.last_point.map);
				if (!map1 || !map2 || //Avoid sorting if either one failed to resolve.
					stricmp(map1, map2) < 0 ||
					// if same map name, we sort by name.
					(stricmp(map1, map2) == 0 &&
					 stricmp(char_dat[j].status.name, char_dat[id[k]].status.name) < 0)) {
					for(l = *players; l > k; l--)
						id[l] = id[l-1];
					id[k] = j; // id[*players]
					break;
				}
			}
		}
		break;
		default: // 0 or invalid value: no sorting
			break;
		}
	(*players)++;
	break;
	}
	return 0;
}
//-------------------------------------------------------------
// Function to create the online files (txt and html). by [Yor]
//-------------------------------------------------------------
void create_online_files(void) {
	unsigned int k, j; // for loop with strlen comparing
	int i, l; // for loops
	int players;    // count the number of players
	FILE *fp;       // for the txt file
	FILE *fp2;      // for the html file
	char temp[256];      // to prepare what we must display
	time_t time_server;  // for number of seconds
	struct tm *datetime; // variable for time in structure ->tm_mday, ->tm_sec, ...
	int id[4096];

	if (online_display_option == 0) // we display nothing, so return
		return;

	// Get number of online players, id of each online players, and verify if a server is offline
	players = 0;
	online_char_db->foreach(online_char_db, create_online_files_sub, &players, &id);

	// write files
	fp = fopen(online_txt_filename, "w");
	if (fp != NULL) {
		fp2 = fopen(online_html_filename, "w");
		if (fp2 != NULL) {
			// get time
			time(&time_server); // get time in seconds since 1/1/1970
			datetime = localtime(&time_server); // convert seconds in structure
			strftime(temp, sizeof(temp), "%d %b %Y %X", datetime); // like sprintf, but only for date/time (05 dec 2003 15:12:52)
			// write heading
			fprintf(fp2, "<HTML>\n");
			fprintf(fp2, "  <META http-equiv=\"Refresh\" content=\"%d\">\n", online_refresh_html); // update on client explorer every x seconds
			fprintf(fp2, "  <HEAD>\n");
			fprintf(fp2, "    <TITLE>Online Players on %s</TITLE>\n", server_name);
			fprintf(fp2, "  </HEAD>\n");
			fprintf(fp2, "  <BODY>\n");
			fprintf(fp2, "    <H3>Online Players on %s (%s):</H3>\n", server_name, temp);
			fprintf(fp, "Online Players on %s (%s):\n", server_name, temp);
			fprintf(fp, "\n");

			for (i = 0; i < players; i++) {
				// if it's the first player
				if (i == 0) {
					j = 0; // count the number of characters for the txt version and to set the separate line
					fprintf(fp2, "    <table border=\"1\" cellspacing=\"1\">\n");
					fprintf(fp2, "      <tr>\n");
					if ((online_display_option & 1) || (online_display_option & 64)) {
						fprintf(fp2, "        <td><b>Name</b></td>\n");
						if (online_display_option & 64) {
							fprintf(fp, "Name                          "); // 30
							j += 30;
						} else {
							fprintf(fp, "Name                     "); // 25
							j += 25;
						}
					}
					if ((online_display_option & 6) == 6) {
						fprintf(fp2, "        <td><b>Job (levels)</b></td>\n");
						fprintf(fp, "Job                 Levels "); // 27
						j += 27;
					} else if (online_display_option & 2) {
						fprintf(fp2, "        <td><b>Job</b></td>\n");
						fprintf(fp, "Job                "); // 19
						j += 19;
					} else if (online_display_option & 4) {
						fprintf(fp2, "        <td><b>Levels</b></td>\n");
						fprintf(fp, " Levels "); // 8
						j += 8;
					}
					if (online_display_option & 24) { // 8 or 16
						fprintf(fp2, "        <td><b>Location</b></td>\n");
						if (online_display_option & 16) {
							fprintf(fp, "Location     ( x , y ) "); // 23
							j += 23;
						} else {
							fprintf(fp, "Location     "); // 13
							j += 13;
						}
					}
					if (online_display_option & 32) {
						fprintf(fp2, "        <td ALIGN=CENTER><b>zenys</b></td>\n");
						fprintf(fp, "          Zenys "); // 16
						j += 16;
					}
					fprintf(fp2, "      </tr>\n");
					fprintf(fp, "\n");
					for (k = 0; k < j; k++)
						fprintf(fp, "-");
					fprintf(fp, "\n");
				}
				fprintf(fp2, "      <tr>\n");
				// get id of the character (more speed)
				j = id[i];
				// displaying the character name
				if ((online_display_option & 1) || (online_display_option & 64)) { // without/with 'GM' display
					strcpy(temp, char_dat[j].status.name);
					l = isGM(char_dat[j].status.account_id);
					if (online_display_option & 64) {
						if (l >= online_gm_display_min_level)
							fprintf(fp, "%-24s (GM) ", temp);
						else
							fprintf(fp, "%-24s      ", temp);
					} else
						fprintf(fp, "%-24s ", temp);
					// name of the character in the html (no < >, because that create problem in html code)
					fprintf(fp2, "        <td>");
					if ((online_display_option & 64) && l >= online_gm_display_min_level)
						fprintf(fp2, "<b>");
					for (k = 0; k < strlen(temp); k++) {
						switch(temp[k]) {
						case '<': // <
							fprintf(fp2, "&lt;");
							break;
						case '>': // >
							fprintf(fp2, "&gt;");
							break;
						default:
							fprintf(fp2, "%c", temp[k]);
							break;
						};
					}
					if ((online_display_option & 64) && l >= online_gm_display_min_level)
						fprintf(fp2, "</b> (GM)");
					fprintf(fp2, "</td>\n");
				}
				// displaying of the job
				if (online_display_option & 6) {
					char * jobname = job_name(char_dat[j].status.class_);
					if ((online_display_option & 6) == 6) {
						fprintf(fp2, "        <td>%s %d/%d</td>\n", jobname, char_dat[j].status.base_level, char_dat[j].status.job_level);
						fprintf(fp, "%-18s %3d/%3d ", jobname, char_dat[j].status.base_level, char_dat[j].status.job_level);
					} else if (online_display_option & 2) {
						fprintf(fp2, "        <td>%s</td>\n", jobname);
						fprintf(fp, "%-18s ", jobname);
					} else if (online_display_option & 4) {
						fprintf(fp2, "        <td>%d/%d</td>\n", char_dat[j].status.base_level, char_dat[j].status.job_level);
						fprintf(fp, "%3d/%3d ", char_dat[j].status.base_level, char_dat[j].status.job_level);
					}
				}
				// displaying of the map
				if (online_display_option & 24) { // 8 or 16
					// prepare map name
					memcpy(temp, mapindex_id2name(char_dat[j].status.last_point.map), MAP_NAME_LENGTH);
					temp[MAP_NAME_LENGTH] = '\0';
					if (strstr(temp, ".gat") != NULL) {
						temp[strstr(temp, ".gat") - temp] = 0; // suppress the '.gat'
					}
					// write map name
					if (online_display_option & 16) { // map-name AND coordinates
						fprintf(fp2, "        <td>%s (%d, %d)</td>\n", temp, char_dat[j].status.last_point.x, char_dat[j].status.last_point.y);
						fprintf(fp, "%-12s (%3d,%3d) ", temp, char_dat[j].status.last_point.x, char_dat[j].status.last_point.y);
					} else {
						fprintf(fp2, "        <td>%s</td>\n", temp);
						fprintf(fp, "%-12s ", temp);
					}
				}
				// displaying nimber of zenys
				if (online_display_option & 32) {
					// write number of zenys
					if (char_dat[j].status.zeny == 0) { // if no zeny
						fprintf(fp2, "        <td ALIGN=RIGHT>no zeny</td>\n");
						fprintf(fp, "        no zeny ");
					} else {
						fprintf(fp2, "        <td ALIGN=RIGHT>%d z</td>\n", char_dat[j].status.zeny);
						fprintf(fp, "%13d z ", char_dat[j].status.zeny);
					}
				}
				fprintf(fp, "\n");
				fprintf(fp2, "      </tr>\n");
			}
			// If we display at least 1 player
			if (players > 0) {
				fprintf(fp2, "    </table>\n");
				fprintf(fp, "\n");
			}

			// Displaying number of online players
			if (players == 0) {
				fprintf(fp2, "    <p>No user is online.</p>\n");
				fprintf(fp, "No user is online.\n");
			} else if (players == 1) {
				fprintf(fp2, "    <p>%d user is online.</p>\n", players);
				fprintf(fp, "%d user is online.\n", players);
			} else {
				fprintf(fp2, "    <p>%d users are online.</p>\n", players);
				fprintf(fp, "%d users are online.\n", players);
			}
			fprintf(fp2, "  </BODY>\n");
			fprintf(fp2, "</HTML>\n");
			fclose(fp2);
		}
		fclose(fp);
	}

	return;
}

//---------------------------------------------------------------------
// This function return the number of online players in all map-servers
//---------------------------------------------------------------------
int count_users(void) {
	int i, users;

	users = 0;
	for(i = 0; i < MAX_MAP_SERVERS; i++)
		if (server_fd[i] >= 0)
			users += server[i].users;

	return users;
}

//----------------------------------------
// Function to send characters to a player
//----------------------------------------
int mmo_char_send006b(int fd, struct char_session_data *sd) {
	int i, j, found_num;
	struct mmo_charstatus *p;
//#ifdef NEW_006b
	const int offset = 24;
//#else
//	const int offset = 4;
//#endif

	set_char_online(-1, 99,sd->account_id);

	found_num = 0;
	for(i = 0; i < char_num; i++) {
		if (char_dat[i].status.account_id == sd->account_id) {
			sd->found_char[found_num] = i;
			found_num++;
			if (found_num == 9)
				break;
		}
	}
	for(i = found_num; i < 9; i++)
		sd->found_char[i] = -1;

	WFIFOHEAD(fd, offset + found_num * 106);
	memset(WFIFOP(fd,0), 0, offset + found_num * 106);
	WFIFOW(fd,0) = 0x6b;
	WFIFOW(fd,2) = offset + found_num * 106;

	for(i = 0; i < found_num; i++) {
		p = &char_dat[sd->found_char[i]].status;
		j = offset + (i * 106); // increase speed of code

		WFIFOL(fd,j) = p->char_id;
		WFIFOL(fd,j+4) = p->base_exp>LONG_MAX?LONG_MAX:p->base_exp;
		WFIFOL(fd,j+8) = p->zeny;
		WFIFOL(fd,j+12) = p->job_exp>LONG_MAX?LONG_MAX:p->job_exp;
		WFIFOL(fd,j+16) = p->job_level;

		WFIFOL(fd,j+20) = 0;
		WFIFOL(fd,j+24) = 0;
		WFIFOL(fd,j+28) = p->option;

		WFIFOL(fd,j+32) = p->karma;
		WFIFOL(fd,j+36) = p->manner;

		WFIFOW(fd,j+40) = (p->status_point>SHRT_MAX) ? SHRT_MAX : p->status_point;
		WFIFOW(fd,j+42) = (p->hp > SHRT_MAX) ? SHRT_MAX : p->hp;
		WFIFOW(fd,j+44) = (p->max_hp > SHRT_MAX) ? SHRT_MAX : p->max_hp;
		WFIFOW(fd,j+46) = (p->sp > SHRT_MAX) ? SHRT_MAX : p->sp;
		WFIFOW(fd,j+48) = (p->max_sp > SHRT_MAX) ? SHRT_MAX : p->max_sp;
		WFIFOW(fd,j+50) = DEFAULT_WALK_SPEED; // p->speed;
		WFIFOW(fd,j+52) = p->class_;
		WFIFOW(fd,j+54) = p->hair;
		WFIFOW(fd,j+56) = p->option&0x20?0:p->weapon; //When the weapon is sent and your option is riding, the client crashes on login!?
		WFIFOW(fd,j+58) = p->base_level;
		WFIFOW(fd,j+60) = (p->skill_point>SHRT_MAX)? SHRT_MAX : p->skill_point;
		WFIFOW(fd,j+62) = p->head_bottom;
		WFIFOW(fd,j+64) = p->shield;
		WFIFOW(fd,j+66) = p->head_top;
		WFIFOW(fd,j+68) = p->head_mid;
		WFIFOW(fd,j+70) = p->hair_color;
		WFIFOW(fd,j+72) = p->clothes_color;

		memcpy(WFIFOP(fd,j+74), p->name, NAME_LENGTH);

		WFIFOB(fd,j+98) = (p->str > 255) ? 255 : p->str;
		WFIFOB(fd,j+99) = (p->agi > 255) ? 255 : p->agi;
		WFIFOB(fd,j+100) = (p->vit > 255) ? 255 : p->vit;
		WFIFOB(fd,j+101) = (p->int_ > 255) ? 255 : p->int_;
		WFIFOB(fd,j+102) = (p->dex > 255) ? 255 : p->dex;
		WFIFOB(fd,j+103) = (p->luk > 255) ? 255 : p->luk;
		WFIFOB(fd,j+104) = p->char_num;
	}

	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

// 離婚(char削除時に使用)
int char_divorce(struct mmo_charstatus *cs) {
	if (cs == NULL)
		return 0;

	if (cs->partner_id > 0){
		int i, j;
		for(i = 0; i < char_num; i++) {
			if (char_dat[i].status.char_id == cs->partner_id && char_dat[i].status.partner_id == cs->char_id) {
				cs->partner_id = 0;
				char_dat[i].status.partner_id = 0;
				for(j = 0; j < MAX_INVENTORY; j++)
					if (char_dat[i].status.inventory[j].nameid == WEDDING_RING_M || char_dat[i].status.inventory[j].nameid == WEDDING_RING_F)
						memset(&char_dat[i].status.inventory[j], 0, sizeof(char_dat[i].status.inventory[0]));
					if (cs->inventory[j].nameid == WEDDING_RING_M || cs->inventory[j].nameid == WEDDING_RING_F)
						memset(&cs->inventory[j], 0, sizeof(cs->inventory[0]));
				return 0;
			}
		}
	}
	return 0;
}

int char_married(int pl1,int pl2) {
	return (char_dat[pl1].status.char_id == char_dat[pl2].status.partner_id && char_dat[pl2].status.char_id == char_dat[pl1].status.partner_id);
}

int char_child(int parent_id, int child_id) {
	return (char_dat[parent_id].status.child == char_dat[child_id].status.char_id && 
		((char_dat[parent_id].status.char_id == char_dat[child_id].status.father) || 
		(char_dat[parent_id].status.char_id == char_dat[child_id].status.mother)));		
}

int char_family(int cid1, int cid2, int cid3) {
	int i, idx1 = -1, idx2 =-1;//, idx3 =-1;
	for(i = 0; i < char_num && (idx1 == -1 || idx2 == -1/* || idx3 == 1*/); i++)
  	{
		if (char_dat[i].status.char_id == cid1)
			idx1 = i;
		if (char_dat[i].status.char_id == cid2)
			idx2 = i;
//		if (char_dat[i].status.char_id == cid2)
//			idx3 = i;
	}
	if (idx1 == -1 || idx2 == -1/* || idx3 == -1*/)
  		return 0; //Some character not found??

	//Unless the dbs are corrupted, these 3 checks should suffice, even though 
	//we could do a lot more checks and force cross-reference integrity.
	if(char_dat[idx1].status.partner_id == cid2 &&
		char_dat[idx1].status.child == cid3)
		return cid3; //cid1/cid2 parents. cid3 child.

	if(char_dat[idx1].status.partner_id == cid3 &&
		char_dat[idx1].status.child == cid2)
		return cid2; //cid1/cid3 parents. cid2 child.

	if(char_dat[idx2].status.partner_id == cid3 &&
		char_dat[idx2].status.child == cid1)
		return cid1; //cid2/cid3 parents. cid1 child.
	return 0;
}

//Clears the given party id from all characters.
//Since sometimes the party format changes and parties must be wiped, this 
//method is required to prevent stress during the "party not found!" stages.
void char_clearparty(int party_id) 
{
	int i;
	for(i = 0; i < char_num; i++)
  	{
		if (char_dat[i].status.party_id == party_id)
			char_dat[i].status.party_id = 0;
	}
}

//------------------------------------------------------------
// E-mail check: return 0 (not correct) or 1 (valid). by [Yor]
//------------------------------------------------------------
int e_mail_check(char *email) {
	char ch;
	char* last_arobas;

	// athena limits
	if (strlen(email) < 3 || strlen(email) > 39)
		return 0;

	// part of RFC limits (official reference of e-mail description)
	if (strchr(email, '@') == NULL || email[strlen(email)-1] == '@')
		return 0;

	if (email[strlen(email)-1] == '.')
		return 0;

	last_arobas = strrchr(email, '@');

	if (strstr(last_arobas, "@.") != NULL ||
	    strstr(last_arobas, "..") != NULL)
		return 0;

	for(ch = 1; ch < 32; ch++) {
		if (strchr(last_arobas, ch) != NULL) {
			return 0;
			break;
		}
	}

	if (strchr(last_arobas, ' ') != NULL ||
	    strchr(last_arobas, ';') != NULL)
		return 0;

	// all correct
	return 1;
}

//----------------------------------------------------------------------
// Force disconnection of an online player (with account value) by [Yor]
//----------------------------------------------------------------------
int disconnect_player(int account_id) {
	int i;
	struct char_session_data *sd;

	// disconnect player if online on char-server
	for(i = 0; i < fd_max; i++) {
		if (session[i] && (sd = (struct char_session_data*)session[i]->session_data)) {
			if (sd->account_id == account_id) {
				session[i]->eof = 1;
				return 1;
			}
		}
	}

	return 0;
}

// キャラ削除に伴うデータ削除
static int char_delete(struct mmo_charstatus *cs) {
	int j;

	// ペット削除
	if (cs->pet_id)
		inter_pet_delete(cs->pet_id);
	if (cs->hom_id)
		inter_homun_delete(cs->hom_id);
	for (j = 0; j < MAX_INVENTORY; j++)
		if (cs->inventory[j].card[0] == (short)0xff00)
			inter_pet_delete(MakeDWord(cs->inventory[j].card[1],cs->inventory[j].card[2]));
	for (j = 0; j < MAX_CART; j++)
		if (cs->cart[j].card[0] == (short)0xff00)
			inter_pet_delete( MakeDWord(cs->cart[j].card[1],cs->cart[j].card[2]) );
	// ギルド脱退
	if (cs->guild_id)
		inter_guild_leave(cs->guild_id, cs->account_id, cs->char_id);
	// パーティー脱退
	if (cs->party_id)
		inter_party_leave(cs->party_id, cs->account_id, cs->char_id);
	// 離婚
	if (cs->partner_id){
		// 離婚情報をmapに通知
		unsigned char buf[10];
		WBUFW(buf,0) = 0x2b12;
		WBUFL(buf,2) = cs->char_id;
		WBUFL(buf,6) = cs->partner_id;
		mapif_sendall(buf,10);
		// 離婚
		char_divorce(cs);
	}
#ifdef ENABLE_SC_SAVING
	status_delete_scdata(cs->account_id, cs->char_id);
#endif
	return 0;
}

int send_accounts_tologin(int tid, unsigned int tick, int id, int data);

int parse_tologin(int fd) {
	int i;
	struct char_session_data *sd;
	RFIFOHEAD(fd);

	// only login-server can have an access to here.
	// so, if it isn't the login-server, we disconnect the session (fd != login_fd).
	if (fd != login_fd)
		session[fd]->eof = 1;
	if(session[fd]->eof) {
		if (fd == login_fd) {
			ShowWarning("Connection to login-server lost (connection #%d).\n", fd);
			login_fd = -1;
		}
		do_close(fd);
		return 0;
	}

	sd = (struct char_session_data*)session[fd]->session_data;

	while(RFIFOREST(fd) >= 2 && !session[fd]->eof) {
//		printf("parse_tologin: connection #%d, packet: 0x%x (with being read: %d bytes).\n", fd, RFIFOW(fd,0), RFIFOREST(fd));

		switch(RFIFOW(fd,0)) {
		case 0x2711:
			if (RFIFOREST(fd) < 3)
				return 0;
			if (RFIFOB(fd,2)) {
//				printf("connect login server error : %d\n", RFIFOB(fd,2));
				ShowError("Can not connect to the login-server.\n");
				ShowError("The server communication passwords (default s1/p1) are probably invalid.\n");
				ShowInfo("Also, please make sure your accounts file (default: accounts.txt) has those values present.\n");
				ShowInfo("The communication passwords can be changed in map_athena.conf and char_athena.conf\n");
				exit(1);
			} else {
				ShowStatus("Connected to login-server (connection #%d).\n", fd);
				
				//Send to login accounts currently connected.
				send_accounts_tologin(-1, gettick(), 0, 0);

				// if no map-server already connected, display a message...
				for(i = 0; i < MAX_MAP_SERVERS; i++)
					if (server_fd[i] >= 0 && server[i].map[0]) // if map-server online and at least 1 map
						break;
				if (i == MAX_MAP_SERVERS)
					ShowStatus("Awaiting maps from map-server.\n");
			}
			RFIFOSKIP(fd,3);
			break;

		case 0x2713:
			if (RFIFOREST(fd) < 51)
				return 0;
//			printf("parse_tologin 2713 : %d\n", RFIFOB(fd,6));
			for(i = 0; i < fd_max; i++) {
				if (session[i] && (sd = (struct char_session_data*)session[i]->session_data) && sd->account_id == RFIFOL(fd,2)) {
					if (RFIFOB(fd,6) != 0) {
						WFIFOHEAD(i, 3);
						WFIFOW(i,0) = 0x6c;
						WFIFOB(i,2) = 0x42;
						WFIFOSET(i,3);
					} else if (max_connect_user == 0 || count_users() < max_connect_user) {
//						if (max_connect_user == 0)
//							printf("max_connect_user (unlimited) -> accepted.\n");
//						else
//							printf("count_users(): %d < max_connect_user (%d) -> accepted.\n", count_users(), max_connect_user);
						memcpy(sd->email, RFIFOP(fd, 7), 40);
						if (e_mail_check(sd->email) == 0)
							strncpy(sd->email, "a@a.com", 40); // default e-mail
						sd->connect_until_time = (time_t)RFIFOL(fd,47);
						// send characters to player
						mmo_char_send006b(i, sd);
					} else if(isGM(sd->account_id) >= gm_allow_level) {
						sd->connect_until_time = (time_t)RFIFOL(fd,47);
						// send characters to player
						mmo_char_send006b(i, sd);
					} else {
						// refuse connection: too much online players
//						printf("count_users(): %d < max_connect_use (%d) -> fail...\n", count_users(), max_connect_user);
                                                WFIFOHEAD(fd, 3);
						WFIFOW(i,0) = 0x6c;
						WFIFOW(i,2) = 0;
						WFIFOSET(i,3);
					}
					break;
				}
			}
			RFIFOSKIP(fd,51);
			break;

		// Receiving of an e-mail/time limit from the login-server (answer of a request because a player comes back from map-server to char-server) by [Yor]
		case 0x2717:
			if (RFIFOREST(fd) < 50)
				return 0;
			for(i = 0; i < fd_max; i++) {
				if (session[i] && (sd = (struct char_session_data*)session[i]->session_data)) {
					if (sd->account_id == RFIFOL(fd,2)) {
						memcpy(sd->email, RFIFOP(fd,6), 40);
						if (e_mail_check(sd->email) == 0)
							strncpy(sd->email, "a@a.com", 40); // default e-mail
						sd->connect_until_time = (time_t)RFIFOL(fd,46);
						break;
					}
				}
			}
			RFIFOSKIP(fd,50);
			break;

		// login-server alive packet
		case 0x2718:
			if (RFIFOREST(fd) < 2)
				return 0;
			RFIFOSKIP(fd,2);
			break;

		// Receiving authentification from Freya-type login server (to avoid char->login->char)
		case 0x2719:
			if (RFIFOREST(fd) < 18)
				return 0;
			// to conserv a maximum of authentification, search if account is already authentified and replace it
			// that will reduce multiple connection too
			for(i = 0; i < AUTH_FIFO_SIZE; i++)
				if (auth_fifo[i].account_id == RFIFOL(fd,2))
					break;
			// if not found, use next value
			if (i == AUTH_FIFO_SIZE) {
				if (auth_fifo_pos >= AUTH_FIFO_SIZE)
					auth_fifo_pos = 0;
				i = auth_fifo_pos;
				auth_fifo_pos++;
			}
			auth_fifo[i].account_id = RFIFOL(fd,2);
			auth_fifo[i].char_id = 0;
			auth_fifo[i].login_id1 = RFIFOL(fd,6);
			auth_fifo[i].login_id2 = RFIFOL(fd,10);
			auth_fifo[i].delflag = 2; // 0: auth_fifo canceled/void, 2: auth_fifo received from login/map server in memory, 1: connection authentified
			auth_fifo[i].char_pos = 0;
			auth_fifo[i].connect_until_time = 0; // unlimited/unknown time by default (not display in map-server)
			auth_fifo[i].ip = RFIFOL(fd,14);
			RFIFOSKIP(fd,18);
			break;

		case 0x2721:	// gm reply
			if (RFIFOREST(fd) < 10)
				return 0;
		  {
			unsigned char buf[10];
			WBUFW(buf,0) = 0x2b0b;
			WBUFL(buf,2) = RFIFOL(fd,2); // account
			WBUFL(buf,6) = RFIFOL(fd,6); // GM level
			mapif_sendall(buf,10);
//			printf("parse_tologin: To become GM answer: char -> map.\n");
		  }
			RFIFOSKIP(fd,10);
			break;

		case 0x2723:	// changesex reply (modified by [Yor])
			if (RFIFOREST(fd) < 7)
				return 0;
		  {
			int acc, sex, i, j;
			unsigned char buf[7];
			acc = RFIFOL(fd,2);
			sex = RFIFOB(fd,6);
			RFIFOSKIP(fd, 7);
			if (acc > 0) {
				for(i = 0; i < AUTH_FIFO_SIZE; i++) {
					if (auth_fifo[i].account_id == acc)	
						auth_fifo[i].sex = sex;
				}
				for (i = 0; i < char_num; i++) {
					if (char_dat[i].status.account_id == acc) {
						int jobclass = char_dat[i].status.class_;
						char_dat[i].status.sex = sex;
						if (jobclass == JOB_BARD || jobclass == JOB_DANCER ||
						    jobclass == JOB_CLOWN || jobclass == JOB_GYPSY ||
						    jobclass == JOB_BABY_BARD || jobclass == JOB_BABY_DANCER) {
							// job modification
							if (jobclass == JOB_BARD || jobclass == JOB_DANCER) {
								char_dat[i].status.class_ = (sex) ? JOB_BARD : JOB_DANCER;
							} else if (jobclass == JOB_CLOWN || jobclass == JOB_GYPSY) {
								char_dat[i].status.class_ = (sex) ? JOB_CLOWN : JOB_GYPSY;
							} else if (jobclass == JOB_BABY_BARD || jobclass == JOB_BABY_DANCER) {
								char_dat[i].status.class_ = (sex) ? JOB_BABY_BARD : JOB_BABY_DANCER;
							}
							// remove specifical skills of classes 19, 4020 and 4042
							for(j = 315; j <= 322; j++) {
								if (char_dat[i].status.skill[j].id > 0 && !char_dat[i].status.skill[j].flag) {
									if (char_dat[i].status.skill_point > USHRT_MAX - char_dat[i].status.skill[j].lv)
										char_dat[i].status.skill_point = USHRT_MAX;
									else
										char_dat[i].status.skill_point += char_dat[i].status.skill[j].lv;
									char_dat[i].status.skill[j].id = 0;
									char_dat[i].status.skill[j].lv = 0;
								}
							}
							// remove specifical skills of classes 20, 4021 and 4043
							for(j = 323; j <= 330; j++) {
								if (char_dat[i].status.skill[j].id > 0 && !char_dat[i].status.skill[j].flag) {
									if (char_dat[i].status.skill_point > USHRT_MAX - char_dat[i].status.skill[j].lv)
										char_dat[i].status.skill_point = USHRT_MAX;
									else
										char_dat[i].status.skill_point += char_dat[i].status.skill[j].lv;

									char_dat[i].status.skill[j].id = 0;
									char_dat[i].status.skill[j].lv = 0;
								}
							}
						}
						// to avoid any problem with equipment and invalid sex, equipment is unequiped.
						for (j = 0; j < MAX_INVENTORY; j++) {
							if (char_dat[i].status.inventory[j].nameid && char_dat[i].status.inventory[j].equip)
								char_dat[i].status.inventory[j].equip = 0;
						}
						char_dat[i].status.weapon = 0;
						char_dat[i].status.shield = 0;
						char_dat[i].status.head_top = 0;
						char_dat[i].status.head_mid = 0;
						char_dat[i].status.head_bottom = 0;

						if (char_dat[i].status.guild_id)	//If there is a guild, update the guild_member data [Skotlex]
							inter_guild_sex_changed(char_dat[i].status.guild_id, acc, char_dat[i].status.char_id, sex);
					}
				}
				// disconnect player if online on char-server
				disconnect_player(acc);
			}
			WBUFW(buf,0) = 0x2b0d;
			WBUFL(buf,2) = acc;
			WBUFB(buf,6) = sex;
			mapif_sendall(buf, 7);
		  }
			break;

		case 0x2726:	// Request to send a broadcast message (no answer)
			if (RFIFOREST(fd) < 8 || RFIFOREST(fd) < (8 + RFIFOL(fd,4)))
				return 0;
			if (RFIFOL(fd,4) < 1)
				char_log("Receiving a message for broadcast, but message is void." RETCODE);
			else {
				// at least 1 map-server
				for(i = 0; i < MAX_MAP_SERVERS; i++)
					if (server_fd[i] >= 0)
						break;
				if (i == MAX_MAP_SERVERS)
					char_log("'ladmin': Receiving a message for broadcast, but no map-server is online." RETCODE);
				else {
					unsigned char buf[128];
					char message[4096]; // +1 to add a null terminated if not exist in the packet
					int lp;
					char *p;
					memset(message, '\0', sizeof(message));
					memcpy(message, RFIFOP(fd,8), RFIFOL(fd,4));
					message[sizeof(message)-1] = '\0';
					remove_control_chars((unsigned char *)message);
					// remove all first spaces
					p = message;
					while(p[0] == ' ')
						p++;
					// if message is only composed of spaces
					if (p[0] == '\0')
						char_log("Receiving a message for broadcast, but message is only a lot of spaces." RETCODE);
					// else send message to all map-servers
					else {
						if (RFIFOW(fd,2) == 0) {
							char_log("'ladmin': Receiving a message for broadcast (message (in yellow): %s)" RETCODE,
							         message);
							lp = 4;
						} else {
							char_log("'ladmin': Receiving a message for broadcast (message (in blue): %s)" RETCODE,
							         message);
							lp = 8;
						}
						// split message to max 80 char
						while(p[0] != '\0') { // if not finish
							if (p[0] == ' ') // jump if first char is a space
								p++;
							else {
								char split[80];
								char* last_space;
								sscanf(p, "%79[^\t]", split); // max 79 char, any char (\t is control char and control char was removed before)
								split[sizeof(split)-1] = '\0'; // last char always \0
								if ((last_space = strrchr(split, ' ')) != NULL) { // searching space from end of the string
									last_space[0] = '\0'; // replace it by NULL to have correct length of split
									p++; // to jump the new NULL
								}
								p += strlen(split);
								// send broadcast to all map-servers
								WBUFW(buf,0) = 0x3800;
								WBUFW(buf,2) = lp + strlen(split) + 1;
								WBUFL(buf,4) = 0x65756c62; // only write if in blue (lp = 8)
								memcpy(WBUFP(buf,lp), split, strlen(split) + 1);
								mapif_sendall(buf, WBUFW(buf,2));
							}
						}
					}
				}
			}
			RFIFOSKIP(fd,8 + RFIFOL(fd,4));
			break;

		// account_reg2変更通知
		case 0x2729:
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
		  {	//Receive account_reg2 registry, forward to map servers.
			unsigned char buf[ACCOUNT_REG2_NUM*(256+32+2)+16];
			memcpy(buf,RFIFOP(fd,0), RFIFOW(fd,2));
//			WBUFW(buf,0) = 0x2b11;
			WBUFW(buf,0) = 0x3804; //Map server can now receive all kinds of reg values with the same packet. [Skotlex]
			mapif_sendall(buf, WBUFW(buf,2));
			RFIFOSKIP(fd, RFIFOW(fd,2));
		  }
			break;

		// Account deletion notification (from login-server)
		case 0x2730:
			if (RFIFOREST(fd) < 6)
				return 0;
			// Deletion of all characters of the account
			for(i = 0; i < char_num; i++) {
				if (char_dat[i].status.account_id == RFIFOL(fd,2)) {
					char_delete(&char_dat[i].status);
					if (i < char_num - 1) {
						memcpy(&char_dat[i], &char_dat[char_num-1], sizeof(struct character_data));
						// if moved character owns to deleted account, check again it's character
						if (char_dat[i].status.account_id == RFIFOL(fd,2)) {
							i--;
						// Correct moved character reference in the character's owner by [Yor]
						} else {
							int j, k;
							struct char_session_data *sd2;
							for (j = 0; j < fd_max; j++) {
								if (session[j] && (sd2 = (struct char_session_data*)session[j]->session_data) &&
									sd2->account_id == char_dat[char_num-1].status.account_id) {
									for (k = 0; k < 9; k++) {
										if (sd2->found_char[k] == char_num-1) {
											sd2->found_char[k] = i;
											break;
										}
									}
									break;
								}
							}
						}
					}
					char_num--;
				}
			}
			// Deletion of the storage
			inter_storage_delete(RFIFOL(fd,2));
			// send to all map-servers to disconnect the player
			{
				unsigned char buf[6];
				WBUFW(buf,0) = 0x2b13;
				WBUFL(buf,2) = RFIFOL(fd,2);
				mapif_sendall(buf, 6);
			}
			// disconnect player if online on char-server
			disconnect_player(RFIFOL(fd,2));
			RFIFOSKIP(fd,6);
			break;

		// State change of account/ban notification (from login-server) by [Yor]
		case 0x2731:
			if (RFIFOREST(fd) < 11)
				return 0;
			// send to all map-servers to disconnect the player
			{
				unsigned char buf[11];
				WBUFW(buf,0) = 0x2b14;
				WBUFL(buf,2) = RFIFOL(fd,2);
				WBUFB(buf,6) = RFIFOB(fd,6); // 0: change of statut, 1: ban
				WBUFL(buf,7) = RFIFOL(fd,7); // status or final date of a banishment
				mapif_sendall(buf, 11);
			}
			// disconnect player if online on char-server
			disconnect_player(RFIFOL(fd,2));
			RFIFOSKIP(fd,11);
			break;

		// Receiving GM acounts info from login-server (by [Yor])
		case 0x2732:
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
		  {
			unsigned char buf[32000];
			if (gm_account != NULL)
				aFree(gm_account);
			gm_account = (struct gm_account*)aCalloc(sizeof(struct gm_account) * ((RFIFOW(fd,2) - 4) / 5), 1);
			GM_num = 0;
			for (i = 4; i < RFIFOW(fd,2); i = i + 5) {
				gm_account[GM_num].account_id = RFIFOL(fd,i);
				gm_account[GM_num].level = (int)RFIFOB(fd,i+4);
				//printf("GM account: %d -> level %d\n", gm_account[GM_num].account_id, gm_account[GM_num].level);
				GM_num++;
			}
			ShowStatus("From login-server: receiving information of %d GM accounts.\n", GM_num);
			char_log("From login-server: receiving information of %d GM accounts." RETCODE, GM_num);
			create_online_files(); // update online players files (perhaps some online players change of GM level)
			// send new gm acccounts level to map-servers
			memcpy(buf, RFIFOP(fd,0), RFIFOW(fd,2));
			WBUFW(buf,0) = 0x2b15;
			mapif_sendall(buf, RFIFOW(fd,2));
		  }
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		// Receive GM accounts [Freya login server packet by Yor]
		case 0x2733:
		// add test here to remember that the login-server is Freya-type
		// sprintf (login_server_type, "Freya");
			if (RFIFOREST(fd) < 7)
				return 0;
			{
				unsigned char buf[32000];
				int new_level = 0;
				for(i = 0; i < GM_num; i++)
					if (gm_account[i].account_id == RFIFOL(fd,2)) {
						if (gm_account[i].level != (int)RFIFOB(fd,6)) {
							gm_account[i].level = (int)RFIFOB(fd,6);
							new_level = 1;
						}
						break;
					}
				// if not found, add it
				if (i == GM_num) {
					// limited to 4000, because we send information to char-servers (more than 4000 GM accounts???)
					// int (id) + int (level) = 8 bytes * 4000 = 32k (limit of packets in windows)
					if (((int)RFIFOB(fd,6)) > 0 && GM_num < 4000) {
						if (GM_num == 0) {
							gm_account = (struct gm_account*)aMalloc(sizeof(struct gm_account));
						} else {
							gm_account = (struct gm_account*)aRealloc(gm_account, sizeof(struct gm_account) * (GM_num + 1));						
						}
						gm_account[GM_num].account_id = RFIFOL(fd,2);
						gm_account[GM_num].level = (int)RFIFOB(fd,6);
						new_level = 1;
						GM_num++;
						if (GM_num >= 4000) {
							ShowWarning("4000 GM accounts found. Next GM accounts are not readed.\n");
							char_log("***WARNING: 4000 GM accounts found. Next GM accounts are not readed." RETCODE);
						}
					}
				}
				if (new_level == 1) {
					int len;
					ShowStatus("From login-server: receiving GM account information (%d: level %d).\n", RFIFOL(fd,2), (int)RFIFOB(fd,6));
					char_log("From login-server: receiving a GM account information (%d: level %d)." RETCODE, RFIFOL(fd,2), (int)RFIFOB(fd,6));
					//create_online_files(); // not change online file for only 1 player (in next timer, that will be done
					// send gm acccounts level to map-servers
					len = 4;
					WBUFW(buf,0) = 0x2b15;
				
					for(i = 0; i < GM_num; i++) {
						WBUFL(buf, len) = gm_account[i].account_id;
						WBUFB(buf, len+4) = (unsigned char)gm_account[i].level;
						len += 5;
					}
					WBUFW(buf, 2) = len;
					mapif_sendall(buf, len);
				}
			}
			RFIFOSKIP(fd,7);
			break;

		//Login server request to kick a character out. [Skotlex]
		case 0x2734:
			if (RFIFOREST(fd) < 6)
				return 0;
			{
				struct online_char_data* character;
				int aid = RFIFOL(fd,2);
				if ((character = idb_get(online_char_db, aid)) != NULL)
				{	//Kick out this player.
					if (character->server > -1)
					{	//Kick it from the map server it is on.
						mapif_disconnectplayer(server_fd[character->server], character->account_id, character->char_id, 2);
						if (!character->waiting_disconnect)
							add_timer(gettick()+15000, chardb_waiting_disconnect, character->account_id, 0);
						character->waiting_disconnect = 1;
					} else { //Manual kick from char server.
						struct char_session_data *tsd;
						int i;
						for(i = 0; i < fd_max; i++) {
							if (session[i] && (tsd = (struct char_session_data*)session[i]->session_data) && tsd->account_id == aid)
							{
								WFIFOHEAD(fd, 3);
								WFIFOW(i,0) = 0x81;
								WFIFOB(i,2) = 2;
								WFIFOSET(i,3);
								break;
							}
						}
						if (i == fd_max) //Shouldn't happen, but just in case.
							set_char_offline(99, aid);
					}
				}
				RFIFOSKIP(fd,6);
			}
			break;
		case 0x2735:
		{
			unsigned char buf[2];
			in_addr_t new_ip = 0;
			RFIFOSKIP(fd,2);

			WBUFW(buf,0) = 0x2b1e;
			mapif_sendall(buf, 2);

			new_ip = resolve_hostbyname(login_ip_str, NULL, NULL);
			if (new_ip && new_ip != login_ip)
				login_ip = new_ip; //Update login up.

			new_ip = resolve_hostbyname(char_ip_str, NULL, NULL);
			if (new_ip && new_ip != char_ip)
			{	//Update ip.
				char_ip = new_ip;
				ShowInfo("Updating IP for [%s].\n",char_ip_str);
				WFIFOW(fd,0) = 0x2736;
				WFIFOL(fd,2) = char_ip;
				WFIFOSET(fd,6);
			}
			break;
		}
		default:
			ShowWarning("Unknown packet 0x%04x received from login-server, disconnecting.\n", RFIFOW(fd,0));
			session[fd]->eof = 1;
			return 0;
		}
	}
	RFIFOFLUSH(fd);

	return 0;
}

int request_accreg2(int account_id, int char_id) {
	if (login_fd > 0) {
		WFIFOW(login_fd, 0) = 0x272e;
		WFIFOL(login_fd, 2) = account_id;
		WFIFOL(login_fd, 6) = char_id;
		WFIFOSET(login_fd, 10);
		return 1;
	}
	return 0;
}

//Send packet forward to login-server for account saving
int save_accreg2(unsigned char* buf, int len) {
	if (login_fd > 0) {
		WFIFOHEAD(login_fd, len+4);
		memcpy(WFIFOP(login_fd,4), buf, len);
		WFIFOW(login_fd,0) = 0x2728;
		WFIFOW(login_fd,2) = len+4;
		WFIFOSET(login_fd,len+4);
		return 1;
	}
	return 0;
}

//Receive Registry information for a character.
int char_parse_Registry(int account_id, int char_id, unsigned char *buf, int buf_len) {
	int i,j,p,len;
	for (i = 0; i < char_num; i++) {
		if (char_dat[i].status.account_id == account_id && char_dat[i].status.char_id == char_id)
			break;
	}
	if(i >= char_num) //Character not found?
		return 1;
	for(j=0,p=0;j<GLOBAL_REG_NUM && p<buf_len;j++){
		sscanf(WBUFP(buf,p), "%31c%n",char_dat[i].global[j].str,&len);
		char_dat[i].global[j].str[len]='\0';
		p +=len+1; //+1 to skip the '\0' between strings.
		sscanf(WBUFP(buf,p), "%255c%n",char_dat[i].global[j].value,&len);
		char_dat[i].global[j].value[len]='\0';
		p +=len+1;
	}
	char_dat[i].global_num = j;
	return 0;
}

//Reply to map server with acc reg values.
int char_account_reg_reply(int fd,int account_id,int char_id) {
	int i,j,p;
	WFIFOHEAD(login_fd, GLOBAL_REG_NUM*288 + 13);
	WFIFOW(fd,0)=0x3804;
	WFIFOL(fd,4)=account_id;
	WFIFOL(fd,8)=char_id;
	WFIFOB(fd,12)=3; //Type 3: char acc reg.
	for (i = 0;i < char_num; i++) {
		if (char_dat[i].status.account_id == account_id && char_dat[i].status.char_id	== char_id)
			break;
	}
	if(i >= char_num){ //Character not found? Sent empty packet.
		WFIFOW(fd,2)=13;
	}else{
		for (p=13,j = 0; j < char_dat[i].global_num; j++) {
			if (char_dat[i].global[j].str[0]) {
				p+= sprintf(WFIFOP(fd,p), "%s", char_dat[i].global[j].str)+1; //We add 1 to consider the '\0' in place.
				p+= sprintf(WFIFOP(fd,p), "%s", char_dat[i].global[j].value)+1;
			}
		}
		WFIFOW(fd,2)=p;
	}
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

void char_read_fame_list(void)
{
	int i, j, k;
	struct fame_list fame_item;
	CREATE_BUFFER(id, int, char_num);

	for(i = 0; i < char_num; i++) {
		id[i] = i;
		for(j = 0; j < i; j++) {
			if (char_dat[i].status.fame > char_dat[id[j]].status.fame) {
				for(k = i; k > j; k--)
					id[k] = id[k-1];
				id[j] = i; // id[i]
				break;
			}
		}
	}

	// Empty ranking lists
	memset(smith_fame_list, 0, sizeof(smith_fame_list));
	memset(chemist_fame_list, 0, sizeof(chemist_fame_list));
	memset(taekwon_fame_list, 0, sizeof(taekwon_fame_list));
	// Build Blacksmith ranking list
	for (i = 0, j = 0; i < char_num && j < fame_list_size_smith; i++) {
		if (char_dat[id[i]].status.fame && (
			char_dat[id[i]].status.class_ == JOB_BLACKSMITH ||
			char_dat[id[i]].status.class_ == JOB_WHITESMITH ||
			char_dat[id[i]].status.class_ == JOB_BABY_BLACKSMITH))
		{
			fame_item.id = char_dat[id[i]].status.char_id;
			fame_item.fame = char_dat[id[i]].status.fame;
			strncpy(fame_item.name, char_dat[id[i]].status.name, NAME_LENGTH);

			memcpy(&smith_fame_list[j],&fame_item,sizeof(struct fame_list));
			j++;
		}
	}
	// Build Alchemist ranking list
	for (i = 0, j = 0; i < char_num && j < fame_list_size_chemist; i++) {
		if (char_dat[id[i]].status.fame && (
			char_dat[id[i]].status.class_ == JOB_ALCHEMIST ||
			char_dat[id[i]].status.class_ == JOB_CREATOR ||
			char_dat[id[i]].status.class_ == JOB_BABY_ALCHEMIST))
		{
			fame_item.id = char_dat[id[i]].status.char_id;
			fame_item.fame = char_dat[id[i]].status.fame;
			strncpy(fame_item.name, char_dat[id[i]].status.name, NAME_LENGTH);

			memcpy(&chemist_fame_list[j],&fame_item,sizeof(struct fame_list));

			j++;
		}
	}
	// Build Taekwon ranking list
	for (i = 0, j = 0; i < char_num && j < fame_list_size_taekwon; i++) {
		if (char_dat[id[i]].status.fame &&
			char_dat[id[i]].status.class_ == JOB_TAEKWON)
		{
			fame_item.id = char_dat[id[i]].status.char_id;
			fame_item.fame = char_dat[id[i]].status.fame;
			strncpy(fame_item.name, char_dat[id[i]].status.name, NAME_LENGTH);

			memcpy(&taekwon_fame_list[j],&fame_item,sizeof(struct fame_list));

			j++;
		}
	}
	DELETE_BUFFER(id);
}
// Send map-servers the fame ranking lists
int char_send_fame_list(int fd) {
	int i, len = 8;
	unsigned char buf[32000];
	
	WBUFW(buf,0) = 0x2b1b;

	for(i = 0; i < fame_list_size_smith && smith_fame_list[i].id; i++) {
		memcpy(WBUFP(buf, len), &smith_fame_list[i], sizeof(struct fame_list));
		len += sizeof(struct fame_list);
	}
	// add blacksmith's block length
	WBUFW(buf, 6) = len;

	for(i = 0; i < fame_list_size_chemist && chemist_fame_list[i].id; i++) {
		memcpy(WBUFP(buf, len), &chemist_fame_list[i], sizeof(struct fame_list));
		len += sizeof(struct fame_list);
	}
	// add alchemist's block length
	WBUFW(buf, 4) = len;

	for(i = 0; i < fame_list_size_taekwon && taekwon_fame_list[i].id; i++) {
		memcpy(WBUFP(buf, len), &taekwon_fame_list[i], sizeof(struct fame_list));
		len += sizeof(struct fame_list);
	}
	// add total packet length
	WBUFW(buf, 2) = len;

	if(fd!=-1)
		mapif_send(fd, buf, len);
	else
		mapif_sendall(buf, len);

	return 0;
}

int search_mapserver(unsigned short map, long ip, short port);

int parse_frommap(int fd) {
	int i, j;
	int id;
	RFIFOHEAD(fd);

	for(id = 0; id < MAX_MAP_SERVERS; id++)
		if (server_fd[id] == fd)
			break;
	if(id==MAX_MAP_SERVERS)
		session[fd]->eof=1;
	if(session[fd]->eof){
		if (id < MAX_MAP_SERVERS) {
			unsigned char buf[16384];
			ShowStatus("Map-server %d has disconnected.\n", id);
			//Notify other map servers that this one is gone. [Skotlex]
			WBUFW(buf,0) = 0x2b20;
			WBUFL(buf,4) = server[id].ip;
			WBUFW(buf,8) = server[id].port;
			j = 0;
			for(i = 0; i < MAX_MAP_PER_SERVER; i++)
				if (server[id].map[i])
					WBUFW(buf,10+(j++)*4) = server[id].map[i];
			if (j > 0) {
				WBUFW(buf,2) = j * 4 + 10;
				mapif_sendallwos(fd, buf, WBUFW(buf,2));
			}
			server_fd[id] = -1;
			online_char_db->foreach(online_char_db,char_db_setoffline,i); //Tag relevant chars as 'in disconnected' server.
		}
		do_close(fd);
		create_online_files();
		return 0;
	}

	while(RFIFOREST(fd) >= 2 && !session[fd]->eof) {
		//ShowDebug("Received packet 0x%4x (%d bytes) from map-server (connection %d)\n", RFIFOW(fd, 0), RFIFOREST(fd), fd);

		switch(RFIFOW(fd,0)) {

		// map-server alive packet
		case 0x2718:
			if (RFIFOREST(fd) < 2)
				return 0;
			RFIFOSKIP(fd,2);
			break;

		// request from map-server to reload GM accounts. Transmission to login-server (by Yor)
		case 0x2af7:
			if (login_fd > 0) { // don't send request if no login-server
				WFIFOHEAD(login_fd, 2);
				WFIFOW(login_fd,0) = 0x2709;
				WFIFOSET(login_fd, 2);
			}
			RFIFOSKIP(fd,2);
			break;

		// Receiving map names list from the map-server
		case 0x2afa:
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			memset(server[id].map, 0, sizeof(server[id].map));
			j = 0;
			for(i = 4; i < RFIFOW(fd,2); i += 4) {
				server[id].map[j] =  RFIFOW(fd,i);
				j++;
			}
			{
				unsigned char *p = (unsigned char *)&server[id].ip;
				ShowStatus("Map-Server %d connected: %d maps, from IP %d.%d.%d.%d port %d.\n",
				       id, j, p[0], p[1], p[2], p[3], server[id].port);
				ShowStatus("Map-server %d loading complete.\n", id);
				char_log("Map-Server %d connected: %d maps, from IP %d.%d.%d.%d port %d. Map-server %d loading complete." RETCODE,
				         id, j, p[0], p[1], p[2], p[3], server[id].port, id);

				if (max_account_id != DEFAULT_MAX_ACCOUNT_ID || max_char_id != DEFAULT_MAX_CHAR_ID)
					mapif_send_maxid(max_account_id, max_char_id); //Send the current max ids to the server to keep in sync [Skotlex]
			}
			WFIFOHEAD(fd, 3 + NAME_LENGTH);
			WFIFOW(fd,0) = 0x2afb;
			WFIFOB(fd,2) = 0;
			memcpy(WFIFOP(fd,3), wisp_server_name, NAME_LENGTH); // name for wisp to player
			WFIFOSET(fd,3+NAME_LENGTH);
			//WFIFOSET(fd,27);
			char_send_fame_list(fd); //Send fame list.
			{
				unsigned char buf[16384];
				int x;
				if (j == 0) {
					ShowWarning("Map-Server %d have NO map.\n", id);
					char_log("WARNING: Map-Server %d have NO map." RETCODE, id);
				// Transmitting maps information to the other map-servers
				} else {
					WBUFW(buf,0) = 0x2b04;
					WBUFW(buf,2) = j * 4 + 10;
					WBUFL(buf,4) = server[id].ip;
					WBUFW(buf,8) = server[id].port;
					memcpy(WBUFP(buf,10), RFIFOP(fd,4), j * 4);
					mapif_sendallwos(fd, buf, WBUFW(buf,2));
				}
				// Transmitting the maps of the other map-servers to the new map-server
				for(x = 0; x < MAX_MAP_SERVERS; x++) {
					if (server_fd[x] >= 0 && x != id) {
						WFIFOW(fd,0) = 0x2b04;
						WFIFOL(fd,4) = server[x].ip;
						WFIFOW(fd,8) = server[x].port;
						j = 0;
						for(i = 0; i < MAX_MAP_PER_SERVER; i++)
							if (server[x].map[i])
								WFIFOW(fd,10+(j++)*4) = server[x].map[i];
						if (j > 0) {
							WFIFOW(fd,2) = j * 4 + 10;
							WFIFOSET(fd,WFIFOW(fd,2));
						}
					}
				}
			}
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		//Packet command is now used for sc_data request. [Skotlex]
		case 0x2afc:
			if (RFIFOREST(fd) < 10)
				return 0;
		{
#ifdef ENABLE_SC_SAVING
			int aid, cid;
			struct scdata *data;
			aid = RFIFOL(fd,2);
			cid = RFIFOL(fd,6);
#endif
			RFIFOSKIP(fd, 10);
#ifdef ENABLE_SC_SAVING
			data = status_search_scdata(aid, cid);
			if (data->count > 0)
			{	//Deliver status change data.
				WFIFOW(fd,0) = 0x2b1d;
				WFIFOW(fd,2) = 14 + data->count*sizeof(struct status_change_data);
				WFIFOL(fd,4) = aid;
				WFIFOL(fd,8) = cid;
				WFIFOW(fd,12) = data->count;
				for (i = 0; i < data->count; i++)
					memcpy(WFIFOP(fd,14+i*sizeof(struct status_change_data)), &data->data[i], sizeof(struct status_change_data));
				WFIFOSET(fd, WFIFOW(fd,2));
				status_delete_scdata(aid, cid); //Data sent, so it needs be discarded now.
			}
#endif
			break;
		}
		
		//set MAP user count
		case 0x2afe:
			if (RFIFOREST(fd) < 4)
				return 0;
			if (RFIFOW(fd,2) != server[id].users) {
				server[id].users = RFIFOW(fd,2);
				ShowInfo("User Count: %d (Server: %d)\n", server[id].users, id);
			}
			RFIFOSKIP(fd, 4);
			break;
		//set MAP users
		case 0x2aff:
			if (RFIFOREST(fd) < 6 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			//TODO: When data mismatches memory, update guild/party online/offline states.
			server[id].users = RFIFOW(fd,4);
			// add online players in the list by [Yor], adapted to use dbs by [Skotlex]
			j = 0;
			online_char_db->foreach(online_char_db,char_db_setoffline,id); //Set all chars from this server as 'unknown'
			for(i = 0; i < server[id].users; i++) {
				int aid, cid;
				struct online_char_data* character;
				aid = RFIFOL(fd,6+i*8);
				cid = RFIFOL(fd,6+i*8+4);
				character = idb_ensure(online_char_db, aid, create_online_char_data);
				if (online_check && character->server > -1 && character->server != id)
				{
					ShowNotice("Set map user: Character (%d:%d) marked on map server %d, but map server %d claims to have (%d:%d) online!\n",
						character->account_id, character->char_id, character->server, id, aid, cid);
					mapif_disconnectplayer(server_fd[character->server], character->account_id, character->char_id, 2);
				}
				character->char_id = cid;
				character->server = id;
			}
			if (update_online < time(NULL)) { // Time is done
				update_online = time(NULL) + 8;
				create_online_files(); // only every 8 sec. (normally, 1 server send users every 5 sec.) Don't update every time, because that takes time, but only every 2 connection.
				                       // it set to 8 sec because is more than 5 (sec) and if we have more than 1 map-server, informations can be received in shifted.
			}
			//If any chars remain in -2, they will be cleaned in the cleanup timer.
			RFIFOSKIP(fd,6+i*8);
			break;

		// キャラデータ保存
		// Recieve character data from map-server
		case 0x2b01:
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			for(i = 0; i < char_num; i++) {
				if (char_dat[i].status.account_id == RFIFOL(fd,4) &&
				    char_dat[i].status.char_id == RFIFOL(fd,8))
					break;
			}
			if (i != char_num)
				memcpy(&char_dat[i].status, RFIFOP(fd,13), sizeof(struct mmo_charstatus));
			if (RFIFOB(fd,12))
			{	//Flag, set character offline. [Skotlex]
				set_char_offline(RFIFOL(fd,8),RFIFOL(fd,4));
				WFIFOW(fd, 0) = 0x2b21; //Save ack only needed on final save.
				WFIFOL(fd, 2) = RFIFOL(fd,4);
				WFIFOL(fd, 6) = RFIFOL(fd,8);
				WFIFOSET(fd, 10);
			}
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		// キャラセレ要求
		case 0x2b02:
			if (RFIFOREST(fd) < 18)
				return 0;
			if (auth_fifo_pos >= AUTH_FIFO_SIZE)
				auth_fifo_pos = 0;
			auth_fifo[auth_fifo_pos].account_id = RFIFOL(fd,2);
			auth_fifo[auth_fifo_pos].char_id = 0;
			auth_fifo[auth_fifo_pos].login_id1 = RFIFOL(fd,6);
			auth_fifo[auth_fifo_pos].login_id2 = RFIFOL(fd,10);
			auth_fifo[auth_fifo_pos].delflag = 2;
			auth_fifo[auth_fifo_pos].char_pos = 0;
			auth_fifo[auth_fifo_pos].connect_until_time = 0; // unlimited/unknown time by default (not display in map-server)
			auth_fifo[auth_fifo_pos].ip = RFIFOL(fd,14);
			auth_fifo_pos++;
			WFIFOW(fd,0) = 0x2b03;
			WFIFOL(fd,2) = RFIFOL(fd,2);
			WFIFOB(fd,6) = 0;
			WFIFOSET(fd,7);
			RFIFOSKIP(fd,18);
			break;

		// request "change map server"
		case 0x2b05:
			if (RFIFOREST(fd) < 35)
				return 0;
			{
				unsigned short name;
				int map_id, map_fd = -1;
				struct online_char_data* data;
				struct mmo_charstatus* char_data;

				name = RFIFOW(fd,18);
				map_id = search_mapserver(name, RFIFOL(fd,24), RFIFOW(fd,28)); //Locate mapserver by ip and port.
				if (map_id >= 0)
					map_fd = server_fd[map_id];
				for(i = 0; i < char_num; i++) {
				if (char_dat[i].status.account_id == RFIFOL(fd,2) &&
				    char_dat[i].status.char_id == RFIFOL(fd,14))
					break;
				}
				char_data = i< char_num? &char_dat[i].status:NULL;
				//Tell the new map server about this player using Kevin's new auth packet. [Skotlex]
				if (map_fd>=0 && session[map_fd] && char_data) 
				{	//Send the map server the auth of this player.
					//Update the "last map" as this is where the player must be spawned on the new map server.
					char_data->last_point.map = RFIFOW(fd,18);
					char_data->last_point.x = RFIFOW(fd,20);
					char_data->last_point.y = RFIFOW(fd,22);
					char_data->sex = RFIFOB(fd,30);  // Buuyo^

					WFIFOW(map_fd,0) = 0x2afd;
					WFIFOW(map_fd,2) = 20 + sizeof(struct mmo_charstatus);
					WFIFOL(map_fd,4) = RFIFOL(fd, 2); //Account ID
					WFIFOL(map_fd,8) = RFIFOL(fd, 6); //Login1
					WFIFOL(map_fd,16) = RFIFOL(fd,10); //Login2
					WFIFOL(map_fd,12) = (unsigned long)0; //TODO: connect_until_time, how do I figure it out right now?
					memcpy(WFIFOP(map_fd,20), char_data, sizeof(struct mmo_charstatus));
					WFIFOSET(map_fd, WFIFOW(map_fd,2));
					data = idb_ensure(online_char_db, RFIFOL(fd, 2), create_online_char_data);
					data->char_id = char_data->char_id;
					data->server = map_id; //Update server where char is.

					//Reply with an ack.
					WFIFOW(fd, 0) = 0x2b06;
					memcpy(WFIFOP(fd,2), RFIFOP(fd,2), 28);
					WFIFOSET(fd, 30);
				} else { //Reply with nak
					WFIFOW(fd, 0) = 0x2b06;
					memcpy(WFIFOP(fd,2), RFIFOP(fd,2), 28);
					WFIFOL(fd, 6) = 0; //Set login1 to 0.
					WFIFOSET(fd, 30);
				}
				RFIFOSKIP(fd, 35);
			}
			break;

		// キャラ名検索
		case 0x2b08:
			if (RFIFOREST(fd) < 6)
				return 0;
			for(i = 0; i < char_num; i++) {
				if (char_dat[i].status.char_id == RFIFOL(fd,2))
					break;
			}
			WFIFOW(fd,0) = 0x2b09;
			WFIFOL(fd,2) = RFIFOL(fd,2);
			if (i != char_num)
				memcpy(WFIFOP(fd,6), char_dat[i].status.name, NAME_LENGTH);
			else
				memcpy(WFIFOP(fd,6), unknown_char_name, NAME_LENGTH);
			WFIFOSET(fd,6+NAME_LENGTH);
			RFIFOSKIP(fd,6);
			break;

		// it is a request to become GM
		case 0x2b0a:
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
//			printf("parse_frommap: change gm -> login, account: %d, pass: '%s'.\n", RFIFOL(fd,4), RFIFOP(fd,8));
			if (login_fd > 0) { // don't send request if no login-server
				WFIFOW(login_fd,0) = 0x2720;
				memcpy(WFIFOP(login_fd,2), RFIFOP(fd,2), RFIFOW(fd,2)-2);
				WFIFOSET(login_fd, RFIFOW(fd,2));
			} else {
				WFIFOW(fd,0) = 0x2b0b;
				WFIFOL(fd,2) = RFIFOL(fd,4);
				WFIFOL(fd,6) = 0;
				WFIFOSET(fd, 10);
			}
			RFIFOSKIP(fd, RFIFOW(fd,2));
			break;

		// Map server send information to change an email of an account -> login-server
		case 0x2b0c:
			if (RFIFOREST(fd) < 86)
				return 0;
			if (login_fd > 0) { // don't send request if no login-server
				memcpy(WFIFOP(login_fd,0), RFIFOP(fd,0), 86); // 0x2722 <account_id>.L <actual_e-mail>.40B <new_e-mail>.40B
				WFIFOW(login_fd,0) = 0x2722;
				WFIFOSET(login_fd, 86);
			}
			RFIFOSKIP(fd, 86);
			break;

		// Map server ask char-server about a character name to do some operations (all operations are transmitted to login-server)
		case 0x2b0e:
			if (RFIFOREST(fd) < 44)
				return 0;
		  {
			char character_name[NAME_LENGTH];
			int acc = RFIFOL(fd,2); // account_id of who ask (-1 if nobody)
			memcpy(character_name, RFIFOP(fd,6), NAME_LENGTH-1);
			character_name[NAME_LENGTH -1] = '\0';
			// prepare answer
			WFIFOW(fd,0) = 0x2b0f; // answer
			WFIFOL(fd,2) = acc; // who want do operation
			WFIFOW(fd,30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex
			// search character
			i = search_character_index(character_name);
			if (i >= 0) {
				memcpy(WFIFOP(fd,6), search_character_name(i), NAME_LENGTH); // put correct name if found
				WFIFOW(fd,6+NAME_LENGTH) = 0; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
				//WFIFOW(fd,32) = 0; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
				switch(RFIFOW(fd, 30)) {
				case 1: // block
					if (acc == -1 || isGM(acc) >= isGM(char_dat[i].status.account_id)) {
						if (login_fd > 0) { // don't send request if no login-server
							WFIFOW(login_fd,0) = 0x2724;
							WFIFOL(login_fd,2) = char_dat[i].status.account_id; // account value
							WFIFOL(login_fd,6) = 5; // status of the account
							WFIFOSET(login_fd, 10);
//							printf("char : status -> login: account %d, status: %d \n", char_dat[i].account_id, 5);
						} else
							WFIFOW(fd,32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
					} else
						WFIFOW(fd,32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
					break;
				case 2: // ban
					if (acc == -1 || isGM(acc) >= isGM(char_dat[i].status.account_id)) {
						if (login_fd > 0) { // don't send request if no login-server
							WFIFOW(login_fd, 0) = 0x2725;
							WFIFOL(login_fd, 2) = char_dat[i].status.account_id; // account value
							WFIFOW(login_fd, 6) = RFIFOW(fd,32); // year
							WFIFOW(login_fd, 8) = RFIFOW(fd,34); // month
							WFIFOW(login_fd,10) = RFIFOW(fd,36); // day
							WFIFOW(login_fd,12) = RFIFOW(fd,38); // hour
							WFIFOW(login_fd,14) = RFIFOW(fd,40); // minute
							WFIFOW(login_fd,16) = RFIFOW(fd,42); // second
							WFIFOSET(login_fd,18);
//							printf("char : status -> login: account %d, ban: %dy %dm %dd %dh %dmn %ds\n",
//							       char_dat[i].account_id, (short)RFIFOW(fd,32), (short)RFIFOW(fd,34), (short)RFIFOW(fd,36), (short)RFIFOW(fd,38), (short)RFIFOW(fd,40), (short)RFIFOW(fd,42));
						} else
							WFIFOW(fd,32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
					} else
						WFIFOW(fd,32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
					break;
				case 3: // unblock
					if (acc == -1 || isGM(acc) >= isGM(char_dat[i].status.account_id)) {
						if (login_fd > 0) { // don't send request if no login-server
							WFIFOW(login_fd,0) = 0x2724;
							WFIFOL(login_fd,2) = char_dat[i].status.account_id; // account value
							WFIFOL(login_fd,6) = 0; // status of the account
							WFIFOSET(login_fd, 10);
//							printf("char : status -> login: account %d, status: %d \n", char_dat[i].account_id, 0);
						} else
							WFIFOW(fd,32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
					} else
						WFIFOW(fd,32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
					break;
				case 4: // unban
					if (acc == -1 || isGM(acc) >= isGM(char_dat[i].status.account_id)) {
						if (login_fd > 0) { // don't send request if no login-server
							WFIFOW(login_fd, 0) = 0x272a;
							WFIFOL(login_fd, 2) = char_dat[i].status.account_id; // account value
							WFIFOSET(login_fd, 6);
//							printf("char : status -> login: account %d, unban request\n", char_dat[i].account_id);
						} else
							WFIFOW(fd,32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
					} else
						WFIFOW(fd,32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
					break;
				case 5: // changesex
					if (acc == -1 || isGM(acc) >= isGM(char_dat[i].status.account_id)) {
						if (login_fd > 0) { // don't send request if no login-server
							WFIFOW(login_fd, 0) = 0x2727;
							WFIFOL(login_fd, 2) = char_dat[i].status.account_id; // account value
							WFIFOSET(login_fd, 6);
//							printf("char : status -> login: account %d, change sex request\n", char_dat[i].account_id);
						} else
							WFIFOW(fd,32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
					} else
						WFIFOW(fd,32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
					break;
				}
			} else {
				// character name not found
				memcpy(WFIFOP(fd,6), character_name, NAME_LENGTH);
				WFIFOW(fd,8+NAME_LENGTH) = 1; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
				//WFIFOW(fd,32) = 1; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
			}
			// send answer if a player ask, not if the server ask
			if (acc != -1) {
				//WFIFOSET(fd, 34);
				WFIFOSET(fd, 10+NAME_LENGTH);
			}
			RFIFOSKIP(fd, 44);
			break;
		  }

//		case 0x2b0f: Not used anymore, available for future use

		// Update and send fame ranking list [DracoRPG]
		case 0x2b10:
			if (RFIFOREST(fd) < 12)
				return 0;
			{
				int cid = RFIFOL(fd, 2);
				int fame = RFIFOL(fd, 6);
				char type = RFIFOB(fd, 10);
				char pos = RFIFOB(fd, 11);
				int size;
				struct fame_list *list = NULL;
				RFIFOSKIP(fd,12);
				
				switch(type) {
					case 1:
						size = fame_list_size_smith;
						list = smith_fame_list;
						break;
					case 2:
						size = fame_list_size_chemist;
						list = chemist_fame_list;
						break;
					case 3:
						size = fame_list_size_taekwon;
						list = taekwon_fame_list;
						break;
					default:
						size = 0;
						break;
				}
				if(!size)
					break;
				if(pos)
				{
				 	pos--; //Convert from pos to index.
					if(
						(pos == 0 || fame < list[pos-1].fame) &&
						(pos == size-1 || fame > list[pos+1].fame)
					) { //No change in order.
						list[(int)pos].fame = fame;
						char_send_fame_list(fd);
						break;
					}
					// If the player's already in the list, remove the entry and shift the following ones 1 step up
					memmove(list+pos, list+pos+1, (size-pos-1) * sizeof(struct fame_list));
					//Clear out last entry.
					list[size-1].id = 0;
					list[size-1].fame = 0;
				}
				// Find the position where the player has to be inserted
				for(i = 0; i < size && fame < list[i].fame; i++);
				// When found someone with less or as much fame, insert just above
				if(i >= size) break;//Out of ranking.
				memmove(list+i+1, list+i, (size-i-1) * sizeof(struct fame_list));
				list[i].id = cid;
				list[i].fame = fame;
				// Look for the player's name
				for(j = 0; j < char_num && char_dat[j].status.char_id != id; j++);
				if(j < char_num)
					strncpy(list[i].name, char_dat[j].status.name, NAME_LENGTH);
				else //Not found??
					strncpy(list[i].name, "Unknown", NAME_LENGTH);
				char_send_fame_list(-1);
			}
			break;

		// Recieve rates [Wizputer]
		case 0x2b16:
			if (RFIFOREST(fd) < 6 || RFIFOREST(fd) < RFIFOW(fd,8))
				return 0;
			// Txt doesn't need this packet, so just skip it
			RFIFOSKIP(fd,RFIFOW(fd,8));
			break;

		// Character disconnected set online 0 [Wizputer]
		case 0x2b17:
			if (RFIFOREST(fd) < 6)
				return 0;
			set_char_offline(RFIFOL(fd,2),RFIFOL(fd,6));
			RFIFOSKIP(fd,10);
			break;

		// Reset all chars to offline [Wizputer]
		case 0x2b18:
			set_all_offline(id);
			RFIFOSKIP(fd,2);
			break;

		// Character set online [Wizputer]
		case 0x2b19:
			if (RFIFOREST(fd) < 6)
				return 0;
			set_char_online(id, RFIFOL(fd,2),RFIFOL(fd,6));
			RFIFOSKIP(fd,10);
			break;

		// Build and send fame ranking lists [DracoRPG]
		case 0x2b1a:
			if (RFIFOREST(fd) < 2)
				return 0;
			char_read_fame_list();
			char_send_fame_list(-1);
			RFIFOSKIP(fd,2);
			break;

		//Request to save status change data. [Skotlex]
		case 0x2b1c:
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
		{
#ifdef ENABLE_SC_SAVING
			int count, aid, cid;
			struct scdata *data;
			aid = RFIFOL(fd, 4);
			cid = RFIFOL(fd, 8);
			count = RFIFOW(fd, 12);
			data = status_search_scdata(aid, cid);
			if (data->count != count)
			{
				data->count = count;
				data->data = aRealloc(data->data, count*sizeof(struct status_change_data));
			}
			for (i = 0; i < count; i++)
				memcpy (&data->data[i], RFIFOP(fd, 14+i*sizeof(struct status_change_data)), sizeof(struct status_change_data));
#endif
			RFIFOSKIP(fd, RFIFOW(fd, 2));
			break;
		}
		case 0x2736:
			if (RFIFOREST(fd) < 6) return 0;
			ShowInfo("Updated IP address of Server #%d to %d.%d.%d.%d.\n",id,
				(int)RFIFOB(fd,2),(int)RFIFOB(fd,3),
				(int)RFIFOB(fd,4),(int)RFIFOB(fd,5));
			server[id].ip = RFIFOL(fd, 2);
			RFIFOSKIP(fd,6);
			break;
		default:
			// inter server処理に渡す
			{
				int r = inter_parse_frommap(fd);
				if (r == 1) // 処理できた
					break;
				if (r == 2) // パケット長が足りない
					return 0;
			}
			// inter server処理でもない場合は切断
			ShowError("Unknown packet 0x%04x from map server, disconnecting.\n", RFIFOW(fd,0));
			session[fd]->eof = 1;
			return 0;
		}
	}
	return 0;
}

int search_mapserver(unsigned short map, long ip, short port) {
	int i, j;
	
	for(i = 0; i < MAX_MAP_SERVERS; i++)
		if (server_fd[i] >= 0)
			for (j = 0; server[i].map[j]; j++)
				if (server[i].map[j] == map) {
					if (ip > 0 && server[i].ip != ip)
						continue;
					if (port > 0 && server[i].port != port)
						continue;
					return i;
				}

	return -1;
}

// char_mapifの初期化処理（現在はinter_mapif初期化のみ）
static int char_mapif_init(int fd) {
	return inter_mapif_init(fd);
}

//--------------------------------------------
// Test to know if an IP come from LAN or WAN.
// Rewrote: Adnvanced subnet check [LuzZza]
//--------------------------------------------
int lan_subnetcheck(long *p) {

	int i;
	unsigned char *sbn, *msk, *src = (unsigned char *)p;
	
	for(i=0; i<subnet_count; i++) {
	
		if(subnet[i].subnet == (*p & subnet[i].mask)) {
			
			sbn = (unsigned char *)&subnet[i].subnet;
			msk = (unsigned char *)&subnet[i].mask;
			
			ShowInfo("Subnet check [%u.%u.%u.%u]: Matches "CL_CYAN"%u.%u.%u.%u/%u.%u.%u.%u"CL_RESET"\n",
				src[0], src[1], src[2], src[3], sbn[0], sbn[1], sbn[2], sbn[3], msk[0], msk[1], msk[2], msk[3]);
			
			return subnet[i].map_ip;
		}
	}
	
	ShowInfo("Subnet check [%u.%u.%u.%u]: "CL_CYAN"WAN"CL_RESET"\n", src[0], src[1], src[2], src[3]);
	return 0;
}

int parse_char(int fd) {
	int i, ch;
	unsigned short cmd;
	char email[40];
	int map_fd;
	struct char_session_data *sd;
	unsigned char *p = (unsigned char *) &session[fd]->client_addr.sin_addr;
	long subnet_map_ip;
	
	RFIFOHEAD(fd);

	sd = (struct char_session_data*)session[fd]->session_data;

	if (login_fd < 0)
		session[fd]->eof = 1;
	if(session[fd]->eof) { // disconnect any player (already connected to char-server or coming back from map-server) if login-server is diconnected.
		if (fd == login_fd)
			login_fd = -1;
		if (sd != NULL)
		{
			struct online_char_data* data = idb_get(online_char_db, sd->account_id);
			if (!data || data->server== -1) //If it is not in any server, send it offline. [Skotlex]
				set_char_offline(99,sd->account_id);
		}
		do_close(fd);
		return 0;
	}

	while(RFIFOREST(fd) >= 2 && !session[fd]->eof) {
		cmd = RFIFOW(fd,0);
		// crc32のスキップ用
		if(	sd==NULL			&&	// 未ログインor管理パケット
			RFIFOREST(fd)>=4	&&	// 最低バイト数制限 ＆ 0x7530,0x7532管理パケ除去
			RFIFOREST(fd)<=21	&&	// 最大バイト数制限 ＆ サーバーログイン除去
			cmd!=0x20b	&&	// md5通知パケット除去
			(RFIFOREST(fd)<6 || RFIFOW(fd,4)==0x65)	){	// 次に何かパケットが来てるなら、接続でないとだめ
			RFIFOSKIP(fd,4);
			cmd = RFIFOW(fd,0);
			ShowDebug("parse_char : %d crc32 skipped\n",fd);
			if(RFIFOREST(fd)==0)
				return 0;
		}

//For use in packets that depend on an sd being present [Skotlex]
#define FIFOSD_CHECK(rest) { if(RFIFOREST(fd) < rest) return 0; if (sd==NULL) { RFIFOSKIP(fd,rest); return 0; } }

		switch(cmd){
		case 0x20b:	//20040622暗号化ragexe対応
			if (RFIFOREST(fd) < 19)
				return 0;
			RFIFOSKIP(fd,19);
			break;

		case 0x65:	// 接続要求
			if (RFIFOREST(fd) < 17)
				return 0;
		  {
			int GM_value;
			if ((GM_value = isGM(RFIFOL(fd,2))))
				ShowInfo("Account Logged On; Account ID: %d (GM level %d).\n", RFIFOL(fd,2), GM_value);
			else
				ShowInfo("Account Logged On; Account ID: %d.\n", RFIFOL(fd,2));
			if (sd == NULL) {
				sd = (struct char_session_data*)aCalloc(sizeof(struct char_session_data), 1);
				session[fd]->session_data = sd;

//				memset(sd, 0, sizeof(struct char_session_data)); aCalloc does this [Skotlex]
				strncpy(sd->email, "no mail", 40); // put here a mail without '@' to refuse deletion if we don't receive the e-mail
				sd->connect_until_time = 0; // unknow or illimited (not displaying on map-server)
			} else {
				//Received again auth packet for already authentified account?? Discard it.
				//TODO: Perhaps log this as a hack attempt?
				RFIFOSKIP(fd,17);
				break;
			}
			sd->account_id = RFIFOL(fd,2);
			sd->login_id1 = RFIFOL(fd,6);
			sd->login_id2 = RFIFOL(fd,10);
			sd->sex = RFIFOB(fd,16);
			// send back account_id
			WFIFOHEAD(fd, 4);
			WFIFOL(fd,0) = RFIFOL(fd,2);
			WFIFOSET(fd,4);
			// search authentification
			for(i = 0; i < AUTH_FIFO_SIZE; i++) {
				if (auth_fifo[i].account_id == sd->account_id &&
				    auth_fifo[i].login_id1 == sd->login_id1 &&
#if CMP_AUTHFIFO_LOGIN2 != 0
				    auth_fifo[i].login_id2 == sd->login_id2 && // relate to the versions higher than 18
#endif
				    (!check_ip_flag || auth_fifo[i].ip == session[fd]->client_addr.sin_addr.s_addr) &&
				    auth_fifo[i].delflag == 2) {
					auth_fifo[i].delflag = 1;
						
					if (online_check)
					{	// check if character is not online already. [Skotlex]
						struct online_char_data* character;
						character = idb_get(online_char_db, sd->account_id);

						if (character)
						{
							if(character->server > -1)
							{	//Kick it from the map server it is on.
								mapif_disconnectplayer(server_fd[character->server], character->account_id, character->char_id, 2);
								if (!character->waiting_disconnect)
									add_timer(gettick()+20000, chardb_waiting_disconnect, character->account_id, 0);
								character->waiting_disconnect = 1;
						/* Not a good idea because this would trigger when you do a char-change from the map server! [Skotlex]
							} else { //Manual kick from char server.
								struct char_session_data *tsd;
								int i;
								for(i = 0; i < fd_max; i++) {
									if (session[i] && i!=fd && (tsd = (struct char_session_data*)session[i]->session_data) && tsd->account_id == sd->account_id)
									{
										WFIFOW(i,0) = 0x81;
										WFIFOB(i,2) = 2;
										WFIFOSET(i,3);
										break;
									}
								}
								if (i == fd_max) //Shouldn't happen, but just in case.
									set_char_offline(99, sd->account_id);
						*/
								WFIFOW(fd,0) = 0x81;
								WFIFOB(fd,2) = 8;
								WFIFOSET(fd,3);
								break;
							}
						}
					}

					if (max_connect_user == 0 || count_users() < max_connect_user) {
						if (login_fd > 0) { // don't send request if no login-server
							// request to login-server to obtain e-mail/time limit
							WFIFOW(login_fd,0) = 0x2716;
							WFIFOL(login_fd,2) = sd->account_id;
							WFIFOSET(login_fd,6);
						}
						// send characters to player
						mmo_char_send006b(fd, sd);
					} else {
						// refuse connection (over populated)
						WFIFOW(fd,0) = 0x6c;
						WFIFOW(fd,2) = 0;
						WFIFOSET(fd,3);
					}
					break;
				}
			}
			// authentification not found
			if (i == AUTH_FIFO_SIZE) {
				if (login_fd > 0) { // don't send request if no login-server
					WFIFOW(login_fd,0) = 0x2712; // ask login-server to authentify an account
					WFIFOL(login_fd,2) = sd->account_id;
					WFIFOL(login_fd,6) = sd->login_id1;
					WFIFOL(login_fd,10) = sd->login_id2; // relate to the versions higher than 18
					WFIFOB(login_fd,14) = sd->sex;
					WFIFOL(login_fd,15) = session[fd]->client_addr.sin_addr.s_addr;
					WFIFOSET(login_fd,19);
				} else { // if no login-server, we must refuse connection
					WFIFOW(fd,0) = 0x6c;
					WFIFOW(fd,2) = 0;
					WFIFOSET(fd,3);
				}
			}
		  }
			RFIFOSKIP(fd,17);
			break;

		case 0x66:	// キャラ選択
			FIFOSD_CHECK(3);
		{
			int char_num = RFIFOB(fd,2);
			struct mmo_charstatus *cd;
			RFIFOSKIP(fd,3);

			// if we activated email creation and email is default email
			if (email_creation != 0 && strcmp(sd->email, "a@a.com") == 0 && login_fd > 0) { // to modify an e-mail, login-server must be online
				WFIFOHEAD(fd, 3);
				WFIFOW(fd, 0) = 0x70;
				WFIFOB(fd, 2) = 0; // 00 = Incorrect Email address
				WFIFOSET(fd, 3);
				break;
			}
			// otherwise, load the character
			for (ch = 0; ch < 9; ch++)
				if (sd->found_char[ch] >= 0 && char_dat[sd->found_char[ch]].status.char_num == char_num)
					break;
			if (ch == 9)
			{	//Not found?? May be forged packet.
				break;
			}
			cd = &char_dat[sd->found_char[ch]].status;
			char_log("Character Selected, Account ID: %d, Character Slot: %d, Character Name: %s." RETCODE,
	         sd->account_id, char_num, cd->name);

			cd->sex = sd->sex;
			
			// searching map server
			i = search_mapserver(cd->last_point.map,-1,-1);
			// if map is not found, we check major cities
			if (i < 0) {
				unsigned short j;
				//First check that there's actually a map server online.
				for(j = 0; j < MAX_MAP_SERVERS; j++)
					if (server_fd[j] >= 0 && server[j].map[0])
						break;
				if (j == MAX_MAP_SERVERS) {
					ShowInfo("Connection Closed. No map servers available.\n");
					WFIFOHEAD(fd, 3);
					WFIFOW(fd,0) = 0x81;
					WFIFOB(fd,2) = 1; // 01 = Server closed
					WFIFOSET(fd,3);
					break;
				}
				if ((i = search_mapserver((j=mapindex_name2id(MAP_PRONTERA)),-1,-1)) >= 0) {
					cd->last_point.x = 273;
					cd->last_point.y = 354;
				} else if ((i = search_mapserver((j=mapindex_name2id(MAP_GEFFEN)),-1,-1)) >= 0) {
					cd->last_point.x = 120;
					cd->last_point.y = 100;
				} else if ((i = search_mapserver((j=mapindex_name2id(MAP_MORROC)),-1,-1)) >= 0) {
					cd->last_point.x = 160;
					cd->last_point.y = 94;
				} else if ((i = search_mapserver((j=mapindex_name2id(MAP_ALBERTA)),-1,-1)) >= 0) {
					cd->last_point.x = 116;
					cd->last_point.y = 57;
				} else if ((i = search_mapserver((j=mapindex_name2id(MAP_PAYON)),-1,-1)) >= 0) {
					cd->last_point.x = 87;
					cd->last_point.y = 117;
				} else if ((i = search_mapserver((j=mapindex_name2id(MAP_IZLUDE)),-1,-1)) >= 0) {
					cd->last_point.x = 94;
					cd->last_point.y = 103;
				} else {
					ShowInfo("Connection Closed. No map server available that has a major city, and unable to find map-server for '%s'.\n", mapindex_id2name(cd->last_point.map));
					WFIFOHEAD(fd, 3);
					WFIFOW(fd,0) = 0x81;
					WFIFOB(fd,2) = 1; // 01 = Server closed
					WFIFOSET(fd,3);
					break;
				}
				ShowWarning("Unable to find map-server for '%s', sending to major city '%s'.\n", mapindex_id2name(cd->last_point.map), mapindex_id2name(j));
				cd->last_point.map = j;
			}
			WFIFOHEAD(fd, 28);
			WFIFOW(fd,0) = 0x71;
			WFIFOL(fd,2) = cd->char_id;
			memcpy(WFIFOP(fd,6), mapindex_id2name(cd->last_point.map), MAP_NAME_LENGTH);
			ShowInfo("Character selection '%s' (account: %d, slot: %d).\n", cd->name, sd->account_id, ch);
			
			// Advanced subnet check [LuzZza]
			if((subnet_map_ip = lan_subnetcheck((long *)p)))
				WFIFOL(fd,22) = subnet_map_ip;
			else
				WFIFOL(fd,22) = server[i].ip;

			WFIFOW(fd,26) = server[i].port;
			WFIFOSET(fd,28);
			if (auth_fifo_pos >= AUTH_FIFO_SIZE)
				auth_fifo_pos = 0;
			auth_fifo[auth_fifo_pos].account_id = sd->account_id;
			auth_fifo[auth_fifo_pos].char_id = cd->char_id;
			auth_fifo[auth_fifo_pos].login_id1 = sd->login_id1;
			auth_fifo[auth_fifo_pos].login_id2 = sd->login_id2;
			auth_fifo[auth_fifo_pos].delflag = 0;
			auth_fifo[auth_fifo_pos].char_pos = sd->found_char[ch];
			auth_fifo[auth_fifo_pos].sex = sd->sex;
			auth_fifo[auth_fifo_pos].connect_until_time = sd->connect_until_time;
			auth_fifo[auth_fifo_pos].ip = session[fd]->client_addr.sin_addr.s_addr;

			//Send NEW auth packet [Kevin]
			if ((map_fd = server_fd[i]) < 1 || session[map_fd] == NULL)
			{	//0 Should not be a valid server_fd [Skotlex]
				ShowError("parse_char: Attempting to write to invalid session %d! Map Server #%d disconnected.\n", map_fd, i);
				server_fd[i] = -1;
				memset(&server[i], 0, sizeof(struct mmo_map_server));
				//Send server closed.
				WFIFOW(fd,0) = 0x81;
				WFIFOB(fd,2) = 1; // 01 = Server closed
				WFIFOSET(fd,3);
				break;
			}	
			WFIFOW(map_fd,0) = 0x2afd;
			WFIFOW(map_fd,2) = 20 + sizeof(struct mmo_charstatus);
			WFIFOL(map_fd,4) = auth_fifo[auth_fifo_pos].account_id;
			WFIFOL(map_fd,8) = auth_fifo[auth_fifo_pos].login_id1;
			WFIFOL(map_fd,16) = auth_fifo[auth_fifo_pos].login_id2;
			WFIFOL(map_fd,12) = (unsigned long)auth_fifo[auth_fifo_pos].connect_until_time;
			memcpy(WFIFOP(map_fd,20), cd, sizeof(struct mmo_charstatus));
			WFIFOSET(map_fd, WFIFOW(map_fd,2));

			set_char_online(i, cd->char_id, cd->account_id);
			//Sets char online in the party and breaks even share if needed.
			inter_party_logged(cd->party_id, cd->account_id, cd->char_id);

			auth_fifo_pos++;
		}
		break;

		case 0x67:	// 作成
			FIFOSD_CHECK(37);
				
			if(char_new == 0) //turn character creation on/off [Kevin]
				i = -2;
			else
				i = make_new_char(fd, RFIFOP(fd,2));
				
			if(i == -1){ //added some better faile reporting to client on the txt version [Kevin]
                         //already exists
                            WFIFOHEAD(fd, 3);
                            WFIFOW(fd, 0) = 0x6e;
                            WFIFOB(fd, 2) = 0x00;
                            WFIFOSET(fd, 3);
                            RFIFOSKIP(fd, 37);
                            break;
                        }else if(i == -2){
                            //denied
                            WFIFOHEAD(fd, 3);
                            WFIFOW(fd, 0) = 0x6e;
                            WFIFOB(fd, 2) = 0x02;
                            WFIFOSET(fd, 3); 
                            RFIFOSKIP(fd, 37);                           
                            break;
                        }else if(i == -3){
                            //underaged XD
                            WFIFOHEAD(fd, 3);
                            WFIFOW(fd, 0) = 0x6e;
                            WFIFOB(fd, 2) = 0x01;
                            WFIFOSET(fd, 3);
                            RFIFOSKIP(fd, 37);
                            break;
                        }

                        WFIFOHEAD(fd, 108);
			WFIFOW(fd,0) = 0x6d;
			memset(WFIFOP(fd,2), 0, 106);

			WFIFOL(fd,2) = char_dat[i].status.char_id;
			WFIFOL(fd,2+4) = char_dat[i].status.base_exp>LONG_MAX?LONG_MAX:char_dat[i].status.base_exp;
			WFIFOL(fd,2+8) = char_dat[i].status.zeny;
			WFIFOL(fd,2+12) = char_dat[i].status.job_exp>LONG_MAX?LONG_MAX:char_dat[i].status.job_exp;
			WFIFOL(fd,2+16) = char_dat[i].status.job_level;

			WFIFOL(fd,2+28) = char_dat[i].status.karma;
			WFIFOL(fd,2+32) = char_dat[i].status.manner;

			WFIFOW(fd,2+40) = 0x30;
			WFIFOW(fd,2+42) = (char_dat[i].status.hp > SHRT_MAX) ? SHRT_MAX : char_dat[i].status.hp;
			WFIFOW(fd,2+44) = (char_dat[i].status.max_hp > SHRT_MAX) ? SHRT_MAX : char_dat[i].status.max_hp;
			WFIFOW(fd,2+46) = (char_dat[i].status.sp > SHRT_MAX) ? SHRT_MAX : char_dat[i].status.sp;
			WFIFOW(fd,2+48) = (char_dat[i].status.max_sp > SHRT_MAX) ? SHRT_MAX : char_dat[i].status.max_sp;
			WFIFOW(fd,2+50) = DEFAULT_WALK_SPEED; // char_dat[i].status.speed;
			WFIFOW(fd,2+52) = char_dat[i].status.class_;
			WFIFOW(fd,2+54) = char_dat[i].status.hair;

			WFIFOW(fd,2+58) = char_dat[i].status.base_level;
			WFIFOW(fd,2+60) = (char_dat[i].status.skill_point > SHRT_MAX) ? SHRT_MAX : char_dat[i].status.skill_point;

			WFIFOW(fd,2+64) = char_dat[i].status.shield;
			WFIFOW(fd,2+66) = char_dat[i].status.head_top;
			WFIFOW(fd,2+68) = char_dat[i].status.head_mid;
			WFIFOW(fd,2+70) = char_dat[i].status.hair_color;

			memcpy(WFIFOP(fd,2+74), char_dat[i].status.name, NAME_LENGTH);

			WFIFOB(fd,2+98) = (char_dat[i].status.str > UCHAR_MAX) ? UCHAR_MAX : char_dat[i].status.str;
			WFIFOB(fd,2+99) = (char_dat[i].status.agi > UCHAR_MAX) ? UCHAR_MAX : char_dat[i].status.agi;
			WFIFOB(fd,2+100) = (char_dat[i].status.vit > UCHAR_MAX) ? UCHAR_MAX : char_dat[i].status.vit;
			WFIFOB(fd,2+101) = (char_dat[i].status.int_ > UCHAR_MAX) ? UCHAR_MAX : char_dat[i].status.int_;
			WFIFOB(fd,2+102) = (char_dat[i].status.dex > UCHAR_MAX) ? UCHAR_MAX : char_dat[i].status.dex;
			WFIFOB(fd,2+103) = (char_dat[i].status.luk > UCHAR_MAX) ? UCHAR_MAX : char_dat[i].status.luk;
			WFIFOB(fd,2+104) = char_dat[i].status.char_num;

			WFIFOSET(fd,108);
			RFIFOSKIP(fd,37);
			for(ch = 0; ch < 9; ch++) {
				if (sd->found_char[ch] == -1) {
					sd->found_char[ch] = i;
					break;
				}
			}

		case 0x68:	// delete char //Yor's Fix
			FIFOSD_CHECK(46);

			memcpy(email, RFIFOP(fd,6), 40);
			if (e_mail_check(email) == 0)
				strncpy(email, "a@a.com", 40); // default e-mail

			// if we activated email creation and email is default email
			if (email_creation != 0 && strcmp(sd->email, "a@a.com") == 0 && login_fd > 0) { // to modify an e-mail, login-server must be online
				// if sended email is incorrect e-mail
				if (strcmp(email, "a@a.com") == 0) {
					WFIFOW(fd, 0) = 0x70;
					WFIFOB(fd, 2) = 0; // 00 = Incorrect Email address
					WFIFOSET(fd, 3);
					RFIFOSKIP(fd,46);
				// we act like we have selected a character
				} else {
					// we change the packet to set it like selection.
					for (i = 0; i < 9; i++)
						if (char_dat[sd->found_char[i]].status.char_id == RFIFOL(fd,2)) {
							// we save new e-mail
							memcpy(sd->email, email, 40);
							// we send new e-mail to login-server ('online' login-server is checked before)
							WFIFOW(login_fd,0) = 0x2715;
							WFIFOL(login_fd,2) = sd->account_id;
							memcpy(WFIFOP(login_fd, 6), email, 40);
							WFIFOSET(login_fd,46);
							// skip part of the packet! (46, but leave the size of select packet: 3)
							RFIFOSKIP(fd,43);
							// change value to put new packet (char selection)
							RFIFOW(fd, 0) = 0x66;
							RFIFOB(fd, 2) = char_dat[sd->found_char[i]].status.char_num;
							// not send packet, it's modify of actual packet
							break;
						}
					if (i == 9) {
						WFIFOW(fd, 0) = 0x70;
						WFIFOB(fd, 2) = 0; // 00 = Incorrect Email address
						WFIFOSET(fd, 3);
						RFIFOSKIP(fd,46);
					}
				}

			// otherwise, we delete the character
			} else {
				if (strcmpi(email, sd->email) != 0) { // if it's an invalid email
					WFIFOW(fd, 0) = 0x70;
					WFIFOB(fd, 2) = 0; // 00 = Incorrect Email address
					WFIFOSET(fd, 3);
				// if mail is correct
				} else {
					for (i = 0; i < 9; i++) {
						struct mmo_charstatus *cs = NULL;
						if ((cs = &char_dat[sd->found_char[i]].status)->char_id == RFIFOL(fd,2)) {
							char_delete(cs); // deletion process

							if (sd->found_char[i] != char_num - 1) {
								memcpy(&char_dat[sd->found_char[i]], &char_dat[char_num-1], sizeof(struct mmo_charstatus));
								// Correct moved character reference in the character's owner
								{
									int j, k;
									struct char_session_data *sd2;
									for (j = 0; j < fd_max; j++) {
										if (session[j] && (sd2 = (struct char_session_data*)session[j]->session_data) &&
											sd2->account_id == char_dat[char_num-1].status.account_id) {
											for (k = 0; k < 9; k++) {
												if (sd2->found_char[k] == char_num-1) {
													sd2->found_char[k] = sd->found_char[i];
													break;
												}
											}
											break;
										}
									}
								}
							}

							char_num--;
							for(ch = i; ch < 9-1; ch++)
								sd->found_char[ch] = sd->found_char[ch+1];
							sd->found_char[8] = -1;
							WFIFOW(fd,0) = 0x6f;
							WFIFOSET(fd,2);
							break;
						}
					}

					if (i == 9) {
						WFIFOW(fd,0) = 0x70;
						WFIFOB(fd,2) = 0;
						WFIFOSET(fd,3);
					}
				}
				RFIFOSKIP(fd,46);
			}
			break;

		case 0x2af8:	// マップサーバーログイン
			if (RFIFOREST(fd) < 60)
				return 0;
			WFIFOW(fd,0) = 0x2af9;
			for(i = 0; i < MAX_MAP_SERVERS; i++) {
				if (server_fd[i] < 0)
					break;
			}
			if (i == MAX_MAP_SERVERS || strcmp((char*)RFIFOP(fd,2), userid) || strcmp((char*)RFIFOP(fd,26), passwd)){
				WFIFOB(fd,2) = 3;
				WFIFOSET(fd,3);
				RFIFOSKIP(fd,60);
			} else {
				int len;
				WFIFOB(fd,2) = 0;
				session[fd]->func_parse = parse_frommap;
				server_fd[i] = fd;
				server[i].ip = RFIFOL(fd,54);
				server[i].port = RFIFOW(fd,58);
				server[i].users = 0;
				memset(server[i].map, 0, sizeof(server[i].map));
				WFIFOSET(fd,3);
				RFIFOSKIP(fd,60);
				realloc_fifo(fd, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);
				char_mapif_init(fd);
				// send gm acccounts level to map-servers
				len = 4;
				WFIFOW(fd,0) = 0x2b15;
				for(i = 0; i < GM_num; i++) {
					WFIFOL(fd,len) = gm_account[i].account_id;
					WFIFOB(fd,len+4) = (unsigned char)gm_account[i].level;
					len += 5;
				}
				WFIFOW(fd,2) = len;
				WFIFOSET(fd,len);
				return 0;
			}
			break;

		case 0x187:	// Alive信号？
			if (RFIFOREST(fd) < 6)
				return 0;
			RFIFOSKIP(fd, 6);
			break;

		case 0x7530:	// Athena情報所得
			WFIFOW(fd,0) = 0x7531;
			WFIFOB(fd,2) = ATHENA_MAJOR_VERSION;
			WFIFOB(fd,3) = ATHENA_MINOR_VERSION;
			WFIFOB(fd,4) = ATHENA_REVISION;
			WFIFOB(fd,5) = ATHENA_RELEASE_FLAG;
			WFIFOB(fd,6) = ATHENA_OFFICIAL_FLAG;
			WFIFOB(fd,7) = ATHENA_SERVER_INTER | ATHENA_SERVER_CHAR;
			WFIFOW(fd,8) = ATHENA_MOD_VERSION;
			WFIFOSET(fd,10);
			RFIFOSKIP(fd,2);
			return 0;

		case 0x7532:	// 接続の切断(defaultと処理は一緒だが明示的にするため)
		default:
			session[fd]->eof = 1;
			return 0;
		}
	}
	RFIFOFLUSH(fd);
	return 0;
}

// Console Command Parser [Wizputer]
int parse_console(char *buf) {
    char *type,*command;

	type = (char *)aCalloc(64,1);
	command = (char *)aCalloc(64,1);

//	memset(type,0,64);
//	memset(command,0,64);

    ShowStatus("Console: %s\n",buf);

    if ( sscanf(buf, "%[^:]:%[^\n]", type , command ) < 2 )
        sscanf(buf,"%[^\n]",type);

    ShowDebug("Type of command: %s || Command: %s \n",type,command);

    if(buf) aFree(buf);
    if(type) aFree(type);
    if(command) aFree(command);

    return 0;
}

// 全てのMAPサーバーにデータ送信（送信したmap鯖の数を返す）
int mapif_sendall(unsigned char *buf, unsigned int len) {
	int i, c;

	c = 0;
	for(i = 0; i < MAX_MAP_SERVERS; i++) {
		int fd;
		if ((fd = server_fd[i]) >= 0) {
			if (session[fd] == NULL)
			{	//Could this be the crash's source? [Skotlex]
				ShowError("mapif_sendall: Attempting to write to invalid session %d! Map Server #%d disconnected.\n", fd, i);
				server_fd[i] = -1;
				memset(&server[i], 0, sizeof(struct mmo_map_server));
				continue;
			}
			WFIFOHEAD(fd, len);
			if (WFIFOSPACE(fd) < len) //Increase buffer size.
				realloc_writefifo(fd, len);
			memcpy(WFIFOP(fd,0), buf, len);
			WFIFOSET(fd,len);
			c++;
		}
	}
	return c;
}

// 自分以外の全てのMAPサーバーにデータ送信（送信したmap鯖の数を返す）
int mapif_sendallwos(int sfd, unsigned char *buf, unsigned int len) {
	int i, c;

	c = 0;
	for(i = 0; i < MAX_MAP_SERVERS; i++) {
		int fd;
		if ((fd = server_fd[i]) >= 0 && fd != sfd) {
			WFIFOHEAD(fd, len);
			if (WFIFOSPACE(fd) < len) //Increase buffer size.
				realloc_writefifo(fd, len);
			memcpy(WFIFOP(fd,0), buf, len);
			WFIFOSET(fd, len);
			c++;
		}
	}
	return c;
}
// MAPサーバーにデータ送信（map鯖生存確認有り）
int mapif_send(int fd, unsigned char *buf, unsigned int len) {
	int i;

	if (fd >= 0) {
		for(i = 0; i < MAX_MAP_SERVERS; i++) {
			if (fd == server_fd[i]) {
				WFIFOHEAD(fd, len);
				if (WFIFOSPACE(fd) < len) //Increase buffer size.
					realloc_writefifo(fd, len);
				memcpy(WFIFOP(fd,0), buf, len);
				WFIFOSET(fd,len);
				return 1;
			}
		}
	}
	return 0;
}

int send_users_tologin(int tid, unsigned int tick, int id, int data) {
	int users = count_users();
	unsigned char buf[16];

	if (login_fd > 0 && session[login_fd]) {
		// send number of user to login server
		WFIFOHEAD(login_fd, 6);
		WFIFOW(login_fd,0) = 0x2714;
		WFIFOL(login_fd,2) = users;
		WFIFOSET(login_fd,6);
	}
	// send number of players to all map-servers
	WBUFW(buf,0) = 0x2b00;
	WBUFL(buf,2) = users;
	mapif_sendall(buf, 6);

	return 0;
}

static int send_accounts_tologin_sub(DBKey key, void* data, va_list ap) {
	struct online_char_data* character = (struct online_char_data*)data;
	int *i = va_arg(ap, int*);
	int count = va_arg(ap, int);
	if ((*i) >= count)
		return 0; //This is an error that shouldn't happen....
	if(character->server > -1) {
		WFIFOHEAD(login_fd, 8+count*4);
		WFIFOL(login_fd, 8+(*i)*4) =character->account_id;
		(*i)++;
		return 1;
	}
	return 0;
}

int send_accounts_tologin(int tid, unsigned int tick, int id, int data) {
	int users = count_users(), i=0;

	if (login_fd > 0 && session[login_fd]) {
		// send account list to login server
		WFIFOHEAD(login_fd, 8+users*4);
		WFIFOW(login_fd,0) = 0x272d;
		WFIFOL(login_fd,4) = users;
		online_char_db->foreach(online_char_db, send_accounts_tologin_sub, &i);
		WFIFOW(login_fd,2) = 8+ i*4;
		WFIFOSET(login_fd,WFIFOW(login_fd,2));
	}
	return 0;
}

int check_connect_login_server(int tid, unsigned int tick, int id, int data) {
	if (login_fd > 0 && session[login_fd] != NULL)
		return 0;

	ShowInfo("Attempt to connect to login-server...\n");
	login_fd = make_connection(login_ip, login_port);
	if (login_fd == -1)
	{	//Try again later... [Skotlex]
		login_fd = 0;
		return 0;
	}
	session[login_fd]->func_parse = parse_tologin;
	realloc_fifo(login_fd, FIFOSIZE_SERVERLINK, FIFOSIZE_SERVERLINK);
	WFIFOHEAD(login_fd, 86);
	WFIFOW(login_fd,0) = 0x2710;
	memcpy(WFIFOP(login_fd,2), userid, 24);
	memcpy(WFIFOP(login_fd,26), passwd, 24);
	WFIFOL(login_fd,50) = 0;
	WFIFOL(login_fd,54) = char_ip;
	WFIFOL(login_fd,58) = char_port;
	memcpy(WFIFOP(login_fd,60), server_name, 20);
	WFIFOW(login_fd,80) = 0;
	WFIFOW(login_fd,82) = char_maintenance;

	WFIFOW(login_fd,84) = char_new_display; //only display (New) if they want to [Kevin]
	
	WFIFOSET(login_fd,86);
	return 1;
}

//------------------------------------------------
//Invoked 15 seconds after mapif_disconnectplayer in case the map server doesn't
//replies/disconnect the player we tried to kick. [Skotlex]
//------------------------------------------------
static int chardb_waiting_disconnect(int tid, unsigned int tick, int id, int data)
{
	struct online_char_data* character;
	if ((character = idb_get(online_char_db, id)) != NULL && character->waiting_disconnect)
	{	//Mark it offline due to timeout.
		set_char_offline(character->char_id, character->account_id);
	}
	return 0;
}

//----------------------------------------------------------
// Return numerical value of a switch configuration by [Yor]
// on/off, english, fran軋is, deutsch, espa�ol
//----------------------------------------------------------
int config_switch(const char *str) {
	if (strcmpi(str, "on") == 0 || strcmpi(str, "yes") == 0 || strcmpi(str, "oui") == 0 || strcmpi(str, "ja") == 0 || strcmpi(str, "si") == 0)
		return 1;
	if (strcmpi(str, "off") == 0 || strcmpi(str, "no") == 0 || strcmpi(str, "non") == 0 || strcmpi(str, "nein") == 0)
		return 0;

	return atoi(str);
}

//----------------------------------
// Reading Lan Support configuration
// Rewrote: Anvanced subnet check [LuzZza]
//----------------------------------
int char_lan_config_read(const char *lancfgName) {

	FILE *fp;
	int line_num = 0;
	char line[1024], w1[64], w2[64], w3[64], w4[64];
	
	if((fp = fopen(lancfgName, "r")) == NULL) {
		ShowWarning("LAN Support configuration file is not found: %s\n", lancfgName);
		return 1;
	}

	ShowInfo("Reading the configuration file %s...\n", lancfgName);

	while(fgets(line, sizeof(line)-1, fp)) {

		line_num++;		
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\n' || line[1] == '\n')
			continue;

		line[sizeof(line)-1] = '\0';
		if(sscanf(line,"%[^:]: %[^:]:%[^:]:%[^\r\n]", w1, w2, w3, w4) != 4) {
	
			ShowWarning("Error syntax of configuration file %s in line %d.\n", lancfgName, line_num);	
			continue;
		}

		remove_control_chars((unsigned char *)w1);
		remove_control_chars((unsigned char *)w2);
		remove_control_chars((unsigned char *)w3);
		remove_control_chars((unsigned char *)w4);

		if(strcmpi(w1, "subnet") == 0) {
	
			subnet[subnet_count].mask = inet_addr(w2);
			subnet[subnet_count].char_ip = inet_addr(w3);
			subnet[subnet_count].map_ip = inet_addr(w4);
			subnet[subnet_count].subnet = subnet[subnet_count].char_ip&subnet[subnet_count].mask;
			if (subnet[subnet_count].subnet != (subnet[subnet_count].map_ip&subnet[subnet_count].mask)) {
				ShowError("%s: Configuration Error: The char server (%s) and map server (%s) belong to different subnetworks!\n", lancfgName, w3, w4);
				continue;
			}
				
			subnet_count++;
		}

		ShowStatus("Read information about %d subnetworks.\n", subnet_count);
	}

	fclose(fp);
	return 0;
}
#endif //TXT_SQL_CONVERT

int char_config_read(const char *cfgName) {
	char line[1024], w1[1024], w2[1024];
	FILE *fp = fopen(cfgName, "r");

	if (fp == NULL) {
		ShowFatalError("Configuration file not found: %s.\n", cfgName);
		exit(1);
	}

	ShowInfo("Reading configuration file %s...\n", cfgName);
	while(fgets(line, sizeof(line)-1, fp)) {
		if (line[0] == '/' && line[1] == '/')
			continue;

		line[sizeof(line)-1] = '\0';
		if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) != 2)
			continue;

		remove_control_chars((unsigned char *)w1);
		remove_control_chars((unsigned char *)w2);
		if(strcmpi(w1,"timestamp_format") == 0) {
			strncpy(timestamp_format, w2, 20);
		} else if(strcmpi(w1,"console_silent")==0){
			msg_silent = 0; //To always allow the next line to show up.
			ShowInfo("Console Silent Setting: %d\n", atoi(w2));
			msg_silent = atoi(w2);
#ifndef TXT_SQL_CONVERT
		} else if (strcmpi(w1, "userid") == 0) {
			strncpy(userid, w2, 24);
		} else if (strcmpi(w1, "passwd") == 0) {
			strncpy(passwd, w2, 24);
		} else if (strcmpi(w1, "server_name") == 0) {
			strncpy(server_name, w2, 20);
			server_name[sizeof(server_name) - 1] = '\0';
			ShowStatus("%s server has been initialized\n", w2);
		} else if (strcmpi(w1, "wisp_server_name") == 0) {
			if (strlen(w2) >= 4) {
				memcpy(wisp_server_name, w2, sizeof(wisp_server_name));
				wisp_server_name[sizeof(wisp_server_name) - 1] = '\0';
			}
		} else if (strcmpi(w1, "login_ip") == 0) {
			char ip_str[16];
			login_ip = resolve_hostbyname(w2, NULL, ip_str);
			if (login_ip) {
				strncpy(login_ip_str, w2, sizeof(login_ip_str));
				ShowStatus("Login server IP address : %s -> %s\n", w2, ip_str);
			}
		} else if (strcmpi(w1, "login_port") == 0) {
			login_port = atoi(w2);
		} else if (strcmpi(w1, "char_ip") == 0) {
			char ip_str[16];
			char_ip = resolve_hostbyname(w2, NULL, ip_str);
			if (char_ip){
				strncpy(char_ip_str, w2, sizeof(char_ip_str));
				ShowStatus("Character server IP address : %s -> %s\n", w2, ip_str);
			}
		} else if (strcmpi(w1, "bind_ip") == 0) {
			char ip_str[16];
			bind_ip = resolve_hostbyname(w2, NULL, ip_str);
			if (bind_ip) {
				strncpy(bind_ip_str, w2, sizeof(bind_ip_str));
				ShowStatus("Character server binding IP address : %s -> %s\n", w2, ip_str);
			}
		} else if (strcmpi(w1, "char_port") == 0) {
			char_port = atoi(w2);
		} else if (strcmpi(w1, "char_maintenance") == 0) {
			char_maintenance = atoi(w2);
		} else if (strcmpi(w1, "char_new") == 0) {
			char_new = atoi(w2);
		} else if (strcmpi(w1, "char_new_display") == 0) {
			char_new_display = atoi(w2);
		} else if (strcmpi(w1, "email_creation") == 0) {
			email_creation = config_switch(w2);
		} else if (strcmpi(w1, "scdata_txt") == 0) { //By Skotlex
			strcpy(scdata_txt, w2);
#endif
		} else if (strcmpi(w1, "char_txt") == 0) {
			strcpy(char_txt, w2);
		} else if (strcmpi(w1, "backup_txt") == 0) { //By zanetheinsane
			strcpy(backup_txt, w2);
		} else if (strcmpi(w1, "friends_txt") == 0) { //By davidsiaw
			strcpy(friends_txt, w2);
#ifndef TXT_SQL_CONVERT
		} else if (strcmpi(w1, "backup_txt_flag") == 0) { // The backup_txt file was created because char deletion bug existed. Now it's finish and that take a lot of time to create a second file when there are a lot of characters. By [Yor]
			backup_txt_flag = config_switch(w2);
		} else if (strcmpi(w1, "max_connect_user") == 0) {
			max_connect_user = atoi(w2);
			if (max_connect_user < 0)
				max_connect_user = 0; // unlimited online players
		} else if(strcmpi(w1, "gm_allow_level") == 0) {
			gm_allow_level = atoi(w2);
			if(gm_allow_level < 0)
				gm_allow_level = 99;
		} else if (strcmpi(w1, "check_ip_flag") == 0) {
			check_ip_flag = config_switch(w2);
		} else if (strcmpi(w1, "online_check") == 0) {
			online_check = config_switch(w2);
		} else if (strcmpi(w1, "autosave_time") == 0) {
			autosave_interval = atoi(w2)*1000;
			if (autosave_interval <= 0)
				autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
		} else if (strcmpi(w1, "save_log") == 0) {
			save_log = config_switch(w2);
		} else if (strcmpi(w1, "start_point") == 0) {
			char map[32];
			int x, y;
			if (sscanf(w2, "%[^,],%d,%d", map, &x, &y) < 3)
				continue;
			if (strstr(map, ".gat") != NULL) { // Verify at least if '.gat' is in the map name
				start_point.map = mapindex_name2id(map);
				if (!start_point.map) {
					ShowError("Specified start_point %s not found in map-index cache.\n", map);
					start_point.map = 0;
				}
				start_point.x = x;
				start_point.y = y;
			}
		} else if(strcmpi(w1,"log_char")==0) {		//log char or not [devil]
			log_char = atoi(w2);
		} else if (strcmpi(w1, "start_zeny") == 0) {
			start_zeny = atoi(w2);
			if (start_zeny < 0)
				start_zeny = 0;
		} else if (strcmpi(w1, "start_weapon") == 0) {
			start_weapon = atoi(w2);
			if (start_weapon < 0)
				start_weapon = 0;
		} else if (strcmpi(w1, "start_armor") == 0) {
			start_armor = atoi(w2);
			if (start_armor < 0)
				start_armor = 0;
		} else if (strcmpi(w1, "unknown_char_name") == 0) {
			strcpy(unknown_char_name, w2);
			unknown_char_name[NAME_LENGTH-1] = '\0';
		} else if (strcmpi(w1, "char_log_filename") == 0) {
			strcpy(char_log_filename, w2);
		} else if (strcmpi(w1, "name_ignoring_case") == 0) {
			name_ignoring_case = config_switch(w2);
		} else if (strcmpi(w1, "char_name_option") == 0) {
			char_name_option = atoi(w2);
		} else if (strcmpi(w1, "char_name_letters") == 0) {
			strcpy(char_name_letters, w2);
// online files options
		} else if (strcmpi(w1, "online_txt_filename") == 0) {
			strcpy(online_txt_filename, w2);
		} else if (strcmpi(w1, "online_html_filename") == 0) {
			strcpy(online_html_filename, w2);
		} else if (strcmpi(w1, "online_sorting_option") == 0) {
			online_sorting_option = atoi(w2);
		} else if (strcmpi(w1, "online_display_option") == 0) {
			online_display_option = atoi(w2);
		} else if (strcmpi(w1, "online_gm_display_min_level") == 0) { // minimum GM level to display 'GM' when we want to display it
			online_gm_display_min_level = atoi(w2);
			if (online_gm_display_min_level < 5) // send online file every 5 seconds to player is enough
				online_gm_display_min_level = 5;
		} else if (strcmpi(w1, "online_refresh_html") == 0) {
			online_refresh_html = atoi(w2);
			if (online_refresh_html < 1)
				online_refresh_html = 1;
		} else if(strcmpi(w1,"db_path")==0) {
			strcpy(db_path,w2);
		} else if (strcmpi(w1, "console") == 0) {
			if(strcmpi(w2,"on") == 0 || strcmpi(w2,"yes") == 0 )
				console = 1;
		} else if (strcmpi(w1, "fame_list_alchemist") == 0) {
			fame_list_size_chemist = atoi(w2);
			if (fame_list_size_chemist > MAX_FAME_LIST) {
				ShowWarning("Max fame list size is %d (fame_list_alchemist)\n", MAX_FAME_LIST);
				fame_list_size_chemist = MAX_FAME_LIST;
			}
		} else if (strcmpi(w1, "fame_list_blacksmith") == 0) {
			fame_list_size_smith = atoi(w2);
			if (fame_list_size_smith > MAX_FAME_LIST) {
				ShowWarning("Max fame list size is %d (fame_list_blacksmith)\n", MAX_FAME_LIST);
				fame_list_size_smith = MAX_FAME_LIST;
			}
		} else if (strcmpi(w1, "fame_list_taekwon") == 0) {
			fame_list_size_taekwon = atoi(w2);
			if (fame_list_size_taekwon > MAX_FAME_LIST) {
				ShowWarning("Max fame list size is %d (fame_list_taekwon)\n", MAX_FAME_LIST);
				fame_list_size_taekwon = MAX_FAME_LIST;
			}
		} else if (strcmpi(w1, "guild_exp_rate") == 0) {
			guild_exp_rate = atoi(w2);
#endif //TXT_SQL_CONVERT
		} else if (strcmpi(w1, "import") == 0) {
			char_config_read(w2);
		}
	}
	fclose(fp);

	ShowInfo("done reading %s.\n", cfgName);
	return 0;
}

#ifndef TXT_SQL_CONVERT
int chardb_final(int key, void* data, va_list va)
{
	aFree(data);
	return 0;
}
void do_final(void) {
	ShowStatus("Terminating server.\n");
	// write online players files with no player
	online_char_db->clear(online_char_db, NULL); //clean the db...
	create_online_files();
	online_char_db->destroy(online_char_db, NULL); //dispose the db...

	mmo_char_sync();
	inter_save();
	set_all_offline(-1);
	flush_fifos();
	
	if(gm_account) aFree(gm_account);
	if(char_dat) aFree(char_dat);

	delete_session(login_fd);
	delete_session(char_fd);

#ifdef ENABLE_SC_SAVING
	status_final();
#endif
	inter_final();
	mapindex_final();

	char_log("----End of char-server (normal end with closing of all files)." RETCODE);
}

void set_server_type(void)
{
	SERVER_TYPE = ATHENA_SERVER_CHAR;
}

static int online_data_cleanup_sub(DBKey key, void *data, va_list ap)
{
	struct online_char_data *character= (struct online_char_data*)data;
	if (character->server == -2) //Unknown server.. set them offline
		set_char_offline(character->char_id, character->account_id);
	if (character->server < 0)
		//Free data from players that have not been online for a while.
		db_remove(online_char_db, key);
	return 0;
}

static int online_data_cleanup(int tid, unsigned int tick, int id, int data)
{
	online_char_db->foreach(online_char_db, online_data_cleanup_sub);
	return 0;
}

int do_init(int argc, char **argv) {
	int i;

	mapindex_init(); //Needed here for the start-point reading.
	start_point.map = mapindex_name2id("new_zone01.gat");
	char_config_read((argc < 2) ? CHAR_CONF_NAME : argv[1]);
	char_lan_config_read((argc > 3) ? argv[3] : LOGIN_LAN_CONF_NAME);

	if (strcmp(userid, "s1")==0 && strcmp(passwd, "p1")==0) {
		ShowError("Using the default user/password s1/p1 is NOT RECOMMENDED.\n");
		ShowNotice("Please edit your save/account.txt file to create a proper inter-server user/password (gender 'S')\n");
		ShowNotice("And then change the user/password to use in conf/char_athena.conf (or conf/import/char_conf.txt)\n");
	}

	// a newline in the log...
	char_log("");
	// moved behind char_config_read in case we changed the filename [celest]
	char_log("The char-server starting..." RETCODE);

	if ((naddr_ != 0) && (!login_ip || !char_ip)) {
		// The char server should know what IP address it is running on
		 //   - MouseJstr
		int localaddr = ntohl(addr_[0]);
		unsigned char *ptr = (unsigned char *) &localaddr;
		char buf[16];
		sprintf(buf, "%d.%d.%d.%d", ptr[0], ptr[1], ptr[2], ptr[3]);;
		if (naddr_ != 1)
			ShowStatus("Multiple interfaces detected..  using %s as our IP address\n", buf);
		else
			ShowStatus("Defaulting to %s as our IP address\n", buf);
		if (!login_ip) {
			strcpy(login_ip_str, buf);
			login_ip = inet_addr(login_ip_str);
		}
		if (!char_ip) {
			strcpy(char_ip_str, buf);
			char_ip = inet_addr(char_ip_str);
		}

		if (ptr[0] == 192 && ptr[1] == 168)
			ShowWarning("Firewall detected.. edit subnet_athena.conf and char_athena.conf\n");
	}

	for(i = 0; i < MAX_MAP_SERVERS; i++) {
		memset(&server[i], 0, sizeof(struct mmo_map_server));
		server_fd[i] = -1;
	}

	online_char_db = db_alloc(__FILE__,__LINE__,DB_INT,DB_OPT_RELEASE_DATA,sizeof(int));

	mmo_char_init();
#ifdef ENABLE_SC_SAVING
	status_init();
#endif
	update_online = time(NULL);
	create_online_files(); // update online players files at start of the server

	inter_init_txt((argc > 2) ? argv[2] : inter_cfgName);	// inter server 初期化

	set_defaultparse(parse_char);

	char_fd = make_listen_bind(bind_ip?bind_ip:INADDR_ANY,char_port);

	add_timer_func_list(check_connect_login_server, "check_connect_login_server");
	add_timer_func_list(send_users_tologin, "send_users_tologin");
	add_timer_func_list(send_accounts_tologin, "send_accounts_tologin");
	add_timer_func_list(mmo_char_sync_timer, "mmo_char_sync_timer");
	add_timer_func_list(chardb_waiting_disconnect, "chardb_waiting_disconnect");
	add_timer_func_list(online_data_cleanup, "online_data_cleanup");
	
	add_timer_interval(gettick() + 600*1000, online_data_cleanup, 0, 0, 600 * 1000);
	add_timer_interval(gettick() + 1000, check_connect_login_server, 0, 0, 10 * 1000);
	add_timer_interval(gettick() + 1000, send_users_tologin, 0, 0, 5 * 1000);
	add_timer_interval(gettick() + 3600*1000, send_accounts_tologin, 0, 0, 3600*1000); //Sync online accounts every hour
	add_timer_interval(gettick() + autosave_interval, mmo_char_sync_timer, 0, 0, autosave_interval);

	char_read_fame_list(); //Read fame lists.
	
	if(console) {
	    set_defaultconsoleparse(parse_console);
	   	start_console();
	}

	char_log("The char-server is ready (Server is listening on the port %d)." RETCODE, char_port);

	ShowStatus("The char-server is "CL_GREEN"ready"CL_RESET" (Server is listening on the port %d).\n\n", char_port);

	return 0;
}
#endif //TXT_SQL_CONVERT
