#ifndef TXT_ONLY
// Mail System for eAthena SQL
// Created by Valaris
// moved all strings to msg_athena.conf [Lupus]

#include "map.h"

#include "base.h"
#include "strlib.h"
#include "socket.h"
#include "timer.h"
#include "nullpo.h"
#include "showmsg.h"


#include "clif.h"
#include "chrif.h"
#include "intif.h"
#include "atcommand.h"
#include "pc.h"
#include "mail.h"

char mail_db[32] = "mail";

int MAIL_CHECK_TIME = 120000;
int mail_timer;

int mail_check(struct map_session_data &sd, int type)
{
	int i = 0, new_ = 0, priority = 0;
	char message[50];

	sprintf(tmp_sql,"SELECT `message_id`,`to_account_id`,`from_char_name`,`read_flag`,`priority`,`check_flag` "
		"FROM `%s` WHERE `to_account_id` = \"%ld\" ORDER by `message_id`", 
		mail_db, sd.status.account_id);

	if (mysql_SendQuery(mysql_handle, tmp_sql)) 
	{
		ShowMessage("Database server error (executing query for %s): %s\n", mail_db, mysql_error(mysql_handle));
		return 0;
   	}

   	sql_res = mysql_store_result(mysql_handle);

	if(sql_res) 
	{
		if (mysql_num_rows(sql_res) == 0) 
		{
			//clif_displaymessage(sd.fd,"You have no messages.");
			clif_displaymessage(sd.fd,msg_txt(516));

			mysql_free_result(sql_res);
			return 0;
		}

		while ((sql_row = mysql_fetch_row(sql_res))) 
		{
			i++;
			if(!atoi(sql_row[5])) 
			{
				sprintf(tmp_sql,"UPDATE `%s` SET `check_flag`='1' WHERE `message_id`= \"%d\"", 
					mail_db, atoi(sql_row[0]));
				if(mysql_SendQuery(mysql_handle, tmp_sql) ) 
				{
						ShowMessage("DB server Error (update Read `%s`)- %s\n", 
							mail_db, mysql_error(mysql_handle) );
				}
			}

			if(!atoi(sql_row[3])) 
			{
				new_++;
				if(atoi(sql_row[4]))
					priority++;
				if(type==2 || type==3) 
				{
					if(atoi(sql_row[4])) 
					{
						//sprintf(message, "%d - From : %s (New - Priority)", i, sql_row[2]);
						sprintf(message, msg_txt(511), i, sql_row[2]);

						clif_displaymessage(sd.fd, jstrescape(message));
					} 
					else
					{
						//sprintf(message, "%d - From : %s (New)", i, sql_row[2]);
						sprintf(message, msg_txt(512), i, sql_row[2]);
						clif_displaymessage(sd.fd, jstrescape(message));
					}
				}
			} 
			else if(type==2)
			{
				//sprintf(message, "%d - From : %s", i, sql_row[2]);
				sprintf(message, msg_txt(513), i, sql_row[2]);
				clif_displaymessage(sd.fd, jstrescape(message));
			}
		}

		mysql_free_result(sql_res);
	}
	else
	{
		ShowMessage("MySQL error (storing query result for %s): %s\n", mail_db, mysql_error(mysql_handle));
		return 0;
    }

	if(i>0 && new_>0 && type==1)
	{
		//sprintf(message, "You have %d new messages.", new_);
		sprintf(message, msg_txt(514), new_);

		clif_displaymessage(sd.fd, jstrescape(message));
	}
	if(i>0 && new_>0 && priority>0 && type==1)
	{
		//sprintf(message, "You have %d unread priority messages.", priority);
		sprintf(message, msg_txt(515), priority);
		clif_displaymessage(sd.fd, jstrescape(message));
	}
	if(!new_)
	{
		//clif_displaymessage(sd.fd, "You have no new messages.");
		clif_displaymessage(sd.fd, msg_txt(516));
	}
	return 0;
}

int mail_read(struct map_session_data &sd, unsigned long message_id)
{
	char message[80];

	sprintf(tmp_sql,"SELECT `message_id`,`to_account_id`,`from_char_name`,`message`,`read_flag`,`priority`,`check_flag` from `%s` WHERE `to_account_id` = \"%ld\" ORDER by `message_id` LIMIT %ld, 1",mail_db,sd.status.account_id,message_id-1);

	if (mysql_SendQuery(mysql_handle, tmp_sql))
	{
		ShowMessage("Database server error (executing query for %s): %s\n", mail_db, mysql_error(mysql_handle));
		return 0;
   	}

   	sql_res = mysql_store_result(mysql_handle);
	if(sql_res)
	{
		if (mysql_num_rows(sql_res) == 0)
		{
			mysql_free_result(sql_res);
			//clif_displaymessage(sd.fd, "Message not found.");
			clif_displaymessage(sd.fd, msg_txt(517));
		       return 0;
		}

		if ((sql_row = mysql_fetch_row(sql_res)))
		{
			if(!atoi(sql_row[6]))
			{
				sprintf(tmp_sql,"UPDATE `%s` SET `check_flag`='1' WHERE `message_id`= \"%d\"", mail_db, atoi(sql_row[0]));
				if(mysql_SendQuery(mysql_handle, tmp_sql) )
				{
						ShowMessage("DB server Error (update Read `%s`)- %s\n", mail_db, mysql_error(mysql_handle) );
				}
			}

			//sprintf(message, "Reading message from %s", sql_row[2]);
			sprintf(message, msg_txt(518), sql_row[2]);
			clif_displaymessage(sd.fd, jstrescape(message));

			sprintf(message, "%s", sql_row[3]);
			clif_displaymessage(sd.fd, jstrescape(message));

			sprintf(tmp_sql,"UPDATE `%s` SET `read_flag`='1' WHERE `message_id`= \"%d\"", mail_db, atoi(sql_row[0]));
			if(mysql_SendQuery(mysql_handle, tmp_sql) ) {
				ShowMessage("DB server Error (update Read `%s`)- %s\n", mail_db, mysql_error(mysql_handle) );
			}
		}
		mysql_free_result(sql_res);
	} 
	else
	{
		ShowMessage("MySQL error (storing query result for %s): %s\n", mail_db, mysql_error(mysql_handle));
		return 0;
    
	}
	return 0;
}

int mail_delete(struct map_session_data &sd, unsigned long message_id)
{
	sprintf(tmp_sql,"SELECT `message_id`,`to_account_id`,`read_flag`,`priority`,`check_flag` from `%s` WHERE `to_account_id` = \"%ld\" ORDER by `message_id` LIMIT %ld, 1",mail_db,sd.status.account_id,message_id-1);

	if (mysql_SendQuery(mysql_handle, tmp_sql)) 
	{
		ShowMessage("Database server error (executing query for %s): %s\n", mail_db, mysql_error(mysql_handle));
		return 0;
   	}

   	sql_res = mysql_store_result(mysql_handle);
	if(sql_res) 
	{
		if (mysql_num_rows(sql_res) == 0)
		{
			mysql_free_result(sql_res);
			//clif_displaymessage(sd.fd, "Message not found.");
			clif_displaymessage(sd.fd, msg_txt(517));
			return 0;
		}

		if ((sql_row = mysql_fetch_row(sql_res))) 
		{
			if(!atoi(sql_row[2]) && atoi(sql_row[3]))
			{
				mysql_free_result(sql_res);
				//clif_displaymessage(sd.fd,"Cannot delete unread priority mail.");
				clif_displaymessage(sd.fd,msg_txt(519));

				return 0;
			}
			if(!atoi(sql_row[4]))
			{
				mysql_free_result(sql_res);
				//clif_displaymessage(sd.fd,"You have recieved new mail, use @listmail before deleting.");
				clif_displaymessage(sd.fd,msg_txt(520));
				return 0;
			}
			sprintf(tmp_sql,"DELETE FROM `%s` WHERE `message_id` = \"%d\"", mail_db, atoi(sql_row[0]));
			if(mysql_SendQuery(mysql_handle, tmp_sql) )
			{
				mysql_free_result(sql_res);
				ShowMessage("DB server Error (update Read `%s`)- %s\n", mail_db, mysql_error(mysql_handle) );
				return 0;
			}
			else
				clif_displaymessage(sd.fd,msg_txt(521));
		}

		mysql_free_result(sql_res);

	} 
	else 
	{
		ShowMessage("MySQL error (delete query result for %s): %s\n", mail_db, mysql_error(mysql_handle));
		return 0;
	}
	return 0;
}

int mail_send(struct map_session_data &sd, char *name, char *message, int flag)
{
	if(pc_isGM(sd) < 80 && sd.mail_counter > 0)
	{
		//clif_displaymessage(sd.fd,"You must wait 10 minutes before sending another message");
		clif_displaymessage(sd.fd,msg_txt(522));
		return 0;
	}

	if(strcmp(name,"*")==0)
	{
		if(pc_isGM(sd) < 80)
		{
			//clif_displaymessage(sd.fd, "Access Denied.");
			clif_displaymessage(sd.fd, msg_txt(523));
			return 0;
		}
		else
			sprintf(tmp_sql,"SELECT DISTINCT `account_id` FROM `%s` WHERE `account_id` <> '%ld' ORDER BY `account_id`", char_db, sd.status.account_id);
	}
	else
		sprintf(tmp_sql,"SELECT `account_id`,`name` FROM `%s` WHERE `name` = \"%s\"", char_db, jstrescape(name));

	if (mysql_SendQuery(mysql_handle, tmp_sql))
	{
		ShowMessage("Database server error (executing query for %s): %s\n", char_db, mysql_error(mysql_handle));
		return 0;
   	}

   	sql_res = mysql_store_result(mysql_handle);
	if(sql_res)
	{
		if (mysql_num_rows(sql_res) == 0) 
		{
			mysql_free_result(sql_res);
			//clif_displaymessage(sd.fd,"Character does not exist.");
			clif_displaymessage(sd.fd,msg_txt(524));
			return 0;
		}

		while ((sql_row = mysql_fetch_row(sql_res))) 
		{
			if(strcmp(name,"*")==0) 
			{
				sprintf(tmp_sql, "INSERT DELAYED INTO `%s` (`to_account_id`,`from_account_id`,`from_char_name`,`message`,`priority`)"
					" VALUES ('%d', '%ld', '%s', '%s', '%d')",mail_db, atoi(sql_row[0]), sd.status.account_id, sd.status.name, jstrescape(message), flag);
			}
			else 
			{
				sprintf(tmp_sql, "INSERT DELAYED INTO `%s` (`to_account_id`,`to_char_name`,`from_account_id`,`from_char_name`,`message`,`priority`)"
					" VALUES ('%d', '%s', '%ld', '%s', '%s', '%d')",mail_db, atoi(sql_row[0]), sql_row[1], sd.status.account_id, sd.status.name, jstrescape(message), flag);
				if(pc_isGM(sd) < 80)
					sd.mail_counter=5;
			}

			if(mysql_SendQuery(mysql_handle, tmp_sql) ) 
			{
				mysql_free_result(sql_res);
				ShowMessage("DB server Error (insert `mail_db`)- %s\n", mysql_error(mysql_handle) );
				return 0;
			}
		}
	}

	//clif_displaymessage(sd.fd,"Mail has been sent.");
	clif_displaymessage(sd.fd,msg_txt(525));

	return 0;
}

int mail_check_timer(int tid,unsigned long tick,int id,int data)
{
	struct map_session_data *sd = NULL;
	size_t i;
	
	if(mail_timer != tid)
		return 0;
	sprintf(tmp_sql,"SELECT DISTINCT `to_account_id` FROM `%s` WHERE `read_flag` = '0' AND `check_flag` = '0'", mail_db);
	if (mysql_SendQuery(mysql_handle, tmp_sql))
	{
		ShowMessage("Database server error (executing query for %s): %s\n", char_db, mysql_error(mysql_handle));
		mail_timer=add_timer(gettick()+MAIL_CHECK_TIME,mail_check_timer,0,0);
		return 0;
	}
	
	sql_res = mysql_store_result(mysql_handle);
	if (sql_res)
	{
		if (mysql_num_rows(sql_res) == 0) 
		{
			mysql_free_result(sql_res);
			mail_timer=add_timer(gettick()+MAIL_CHECK_TIME,mail_check_timer,0,0);
			return 0;
		}
		while ((sql_row = mysql_fetch_row(sql_res)))
		{
			for (i = 0; i < fd_max; i++)
			{
				if (session[i] && (sd = (map_session_data *)session[i]->session_data) && sd->state.auth)
				{
					if(pc_isGM(*sd) < 80 && sd->mail_counter > 0)
						sd->mail_counter--;
					if(sd->status.account_id == (unsigned long)atoi(sql_row[0]))
						//clif_displaymessage(sd->fd, "You have new mail.");
						clif_displaymessage(sd->fd, msg_txt(526));
				}
			}
		}
	}
	
	sprintf(tmp_sql,"UPDATE `%s` SET `check_flag`='1' WHERE `check_flag`= '0' ", mail_db);
	if( mysql_SendQuery(mysql_handle, tmp_sql) )
	{
		ShowMessage("DB server Error (update Read `%s`)- %s\n", mail_db, mysql_error(mysql_handle) );
	}
	mail_timer=add_timer(gettick()+MAIL_CHECK_TIME,mail_check_timer,0,0);
	return 0;
}

int do_init_mail(void)
{
	add_timer_func_list(mail_check_timer,"mail_check_timer");
	mail_timer=add_timer(gettick()+MAIL_CHECK_TIME,mail_check_timer,0,0);
	return 0;
}

#endif//TXT_ONLY