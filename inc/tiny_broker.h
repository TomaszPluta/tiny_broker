#include <sys/_stdint.h>

/*
 * tiny_broker.h
 *
 *  Created on: 10.05.2018
 *      Author: tomek
 */

#ifndef INC_TINY_BROKER_H_
#define INC_TINY_BROKER_H_

#include "mqtt_socket.h"
#include "stdbool.h"
#include "string.h"

#define DEFAULT_BROKER_TIMEOUT		(100)
#define MAX_PLD_SIZE				(128)
#define MAX_SUB_PLDT				(8)





#define PROTO_LEVEL_MQTT311			(4)

#define MAX_SUBS_TOPIC 				(8)
#define MAX_TOPIC_NAME_SIZE 		(32)
#define MAX_WILL_MSG_SIZE			(32)
#define MAX_USR_NAME_SIZE			(32)
#define MAX_PSWD_NAME_SIZE			(32)
#define NOT_FOUND					(255)
#define MAX_CONN_CLIENTS			(8)
#define ADDR_SIZE					(4)

#define PLD_START					(12)
#define CLNT_ID_POS					(14)
#define CLNT_ID_SIZE_MSB_POS		(12)
#define CLNT_ID_SIZE_LSB_POS		(13)

#define CONN_ACK_PLD_LEN			(2)

#define CLEAN_S_FLAG			(1<<1)
#define WILL_FLAG				(1<<2)
#define WILL_QOS_FLAG			(3<<3)
#define WILL_RETAIN_FLAG		(1<<5)
#define USR_NAME_FLAG			(1<<6)
#define PSWD_FLAG				(1<<7)


#define SESSION_PRESENT			(1<<0)

#define STRINGS_EQUAL			(0)




#define CONN_ACK_OK					(0)
#define CONN_ACK_BAD_PROTO			(1)
#define CONN_ACK_BAD_ID				(2)
#define CONN_ACK_NOT_AVBL			(3)
#define CONN_ACK_AUTH_MALFORM		(4)
#define CONN_ACK_BAD_AUTH			(5)
#define CONN_ACK_OK_SESS_PRESENT	(0)

#define CONTR_TYPE_CONNACK 			(2)


typedef struct{
uint8_t data[256];
uint8_t len;
uint8_t pos;
}local_host_t;

typedef struct{
	uint8_t len_LSB;

	char * data;
}string_in_frame_t;



typedef struct{
	uint32_t value;
	uint8_t bytes_nb;
}rem_length_t;



typedef struct{
	uint8_t reserved 	   :1;
	uint8_t cleans_session :1;
	uint8_t last_will      :1;
	uint8_t will_qos       :2;
	uint8_t will_retain    :1;
	uint8_t pswd          :1;
	uint8_t user_name      :1;
}conn_flags_t;



typedef struct{
	uint8_t session_pres   :1;
	uint8_t reserved 	   :7;
}Connect_ack_Flags;


typedef struct{
	uint8_t control_type;
	uint8_t remainin_len;
	Connect_ack_Flags ack_flags;
	uint8_t conn_code;
}header_conn_ack_t;



typedef struct{
	uint8_t reserved :4;
	uint8_t type :4;
}conn_ctrl_byte_t;


typedef struct{
	conn_ctrl_byte_t* ctrl_byte;
	uint32_t rem_len;
}conn_fix_head_t;


typedef struct{
	uint16_t * len;
	char * proto_name;
	uint8_t * proto_level;
	conn_flags_t * conn_flags;
	uint16_t * keep_alive;
}conn_var_head_t;


typedef struct{
	uint16_t * client_id_len;
	char * client_id;
	uint16_t  * will_topic_len;
	char * will_topic;
	uint16_t  * will_msg_len;
	char *  will_msg;
	uint16_t * usr_name_len;
	char* usr_name;
	uint16_t * pswd_len;
	char*  pswd;
}conn_pld_t;


typedef struct{
	conn_fix_head_t fix_head;
	conn_var_head_t var_head;
	conn_pld_t pld;
}conn_pck_t;



typedef struct{
	bool session_present;
	uint8_t code;
}conn_ack_stat_t;



/*---------publish-------------------*/

typedef struct{
	uint8_t retain :1;
	uint8_t QoS :2;
	uint8_t dup :1;
	uint8_t type :4;
}pub_ctrl_byte_t;


typedef struct{
	pub_ctrl_byte_t* ctrl_byte;
	uint32_t rem_len;
}pub_fix_head_t;



typedef struct{
	uint16_t * topic_name_len;
	unsigned char * topic_name;
	uint16_t  * packet_id;
}pub_var_head_t;


typedef struct{
	pub_fix_head_t fix_head;
	pub_var_head_t var_head;
	uint8_t * pld;
}pub_pck_t;


typedef struct{
	bool session_present;
	uint8_t code;
}pub_ack_stat_t;



typedef struct{
	uint8_t reserved :4;
	uint8_t type :4;
}subs_ctrl_byte_t;

/*---------subscribe-------------------*/
typedef struct {
	subs_ctrl_byte_t * subs_ctrl_byte;
	uint32_t rem_len;
}sub_fix_head_t;


typedef struct{
	uint16_t  * packet_id;
}sub_var_head_t;


typedef struct{
	uint16_t *topic_name_len;
	unsigned char *topic_name;
	uint8_t *qos;
}sub_topic_t;




typedef struct{
	sub_fix_head_t fix_head;
	sub_var_head_t var_head;
	sub_topic_t pld_topics[MAX_SUB_PLDT];
}sub_pck_t;




typedef struct {
	uint8_t net_address[ADDR_SIZE];
	char  id[MAX_ID_SIZE];
	uint16_t keepalive;
	char  username[MAX_USR_NAME_SIZE];
	char  password;[MAX_PSWD_NAME_SIZE];
	bool last_will;
	char  will_topic[MAX_TOPIC_NAME_SIZE];
	char  will_msg[MAX_WILL_MSG_SIZE];
	uint8_t will_qos;
	uint8_t will_retain;
	sub_topic_t subs_topic[MAX_SUBS_TOPIC];
	bool active;
}  conn_client_t;

typedef struct{
	conn_client_t clients[MAX_CONN_CLIENTS];
	MqttNet * net;
}broker_t;


void broker_init (broker_t * broker, MqttNet* net);
rem_length_t decode_pck_len (uint8_t * frame);
void broker_handle_new_connect (broker_t *broker, conn_pck_t *conn_pck, conn_ack_stat_t * stat, uint8_t* net_add);
void * m_malloc(size_t size);
void broker_fill_new_client(conn_client_t *new_client, const conn_pck_t * conn_pck, uint8_t* net_address);
void broker_handle_new_connect (broker_t *broker, conn_pck_t *conn_pck, conn_ack_stat_t * stat, uint8_t* net_add);
void broker_send_conn_ack(broker_t * broker,  conn_ack_stat_t * stat);
uint8_t broker_decode_connect(uint8_t * frame, conn_pck_t *conn_pck);
void broker_decode_publish(uint8_t* frame, pub_pck_t * pub_pck);
void broker_decode_subscribe(uint8_t* frame, sub_pck_t * sub_pck);
#endif /* INC_TINY_BROKER_H_ */
