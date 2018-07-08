#include <sys/_stdint.h>

/*
 * tiny_broker.h
 *  Created on: 10.05.2018
 *      Author: tomek
 */

#ifndef INC_TINY_BROKER_H_
#define INC_TINY_BROKER_H_

#include <stdbool.h>
#include <string.h>
#include "mqtt_socket.h"
#include "tiny_netinet.h"


/*configurations*/
#define DEFAULT_BROKER_TIMEOUT  	(100)
#define ADDR_SIZE					(4)
#define PROTO_LEVEL_MQTT311 		(4)
#define MAX_PLD_SIZE				(128)
#define MAX_SUBS_TOPIC  			(8)
#define MAX_TOPIC_NAME_SIZE 		(32)
#define MAX_WILL_MSG_SIZE			(32)
#define MAX_USR_NAME_SIZE			(32)
#define MAX_PSWD_NAME_SIZE			(32)
#define MAX_ID_SIZE					(32)
#define NOT_FOUND					(255)
#define MAX_CONN_CLIENTS			(8)


/*protocol defined packet types*/
#define PCKT_TYPE_CONNECT			(1)
#define PCKT_TYPE_CONNACK			(2)
#define PCKT_TYPE_PUBLISH			(3)
#define PCKT_TYPE_PUBACK			(4)
#define PCKT_TYPE_PUBREC			(5)
#define PCKT_TYPE_PUBREL			(6)
#define PCKT_TYPE_PUBCOMP			(7)
#define PCKT_TYPE_SUBSCRIBE  		(8)
#define PCKT_TYPE_SUBACK			(9)
#define PCKT_TYPE_UNSUBSCRIBE		(10)
#define PCKT_TYPE_UNSUBACK			(11)
#define PCKT_TYPE_PINGREQ			(12)
#define PCKT_TYPE_PINGRESP 			(13)
#define PCKT_TYPE_DISCONNECT		(14)


/*header flags*/
#define SESSION_PRESENT				(1<<0)
#define CLEAN_S_FLAG				(1<<1)
#define WILL_FLAG					(1<<2)
#define WILL_QOS_FLAG				(3<<3)
#define WILL_RETAIN_FLAG			(1<<5)
#define USR_NAME_FLAG				(1<<6)
#define PSWD_FLAG					(1<<7)


/*ack flags*/
#define CONN_ACK_PLD_LEN			(2)
#define CONTR_TYPE_CONNACK 			(2)
#define PUB_ACK_LEN					(2)
#define SUB_ACK_LEN					(3)
#define SUB_ACK_FAIL				(80)

/*connection ack coded*/
#define CONN_ACK_OK					(0)
#define CONN_ACK_BAD_PROTO			(1)
#define CONN_ACK_BAD_ID				(2)
#define CONN_ACK_NOT_AVBL			(3)
#define CONN_ACK_AUTH_MALFORM		(4)
#define CONN_ACK_BAD_AUTH			(5)
#define CONN_ACK_OK_SESS_PRESENT	(0)

/*misc*/
#define X_HTONS(a) 					((a>>8) | (a<<8))
#define STRINGS_EQUAL				(0)


/* Shadowing of "sockaddr_in" definitions by broker's own type. It could be easy replaced here
 * without messing in broker's type*/
typedef struct sockaddr_in sockaddr_t;


/*Broker callbacks for mantaining network transmission. It is no needed to use TCP/IP, only compliance
 * with "sockaddr_t" type is needed*/
typedef int (*broker_net_conn)(void *cntx, sockaddr_t * sockaddr);
typedef int (*broker_net_send)(void *cntx, sockaddr_t * sockaddr, const uint8_t* buf, uint16_t buf_len);
typedef int (*broker_net_rec)(void *cntx, sockaddr_t * sockaddr, uint8_t* buf, uint16_t buf_len);
typedef int (*broker_net_discon)(void *context, sockaddr_t * sockaddr);



/*---------connect-------------------*/

typedef struct{
	uint8_t reserved 	   :1;
	uint8_t cleans_session :1;
	uint8_t last_will      :1;
	uint8_t will_qos       :2;
	uint8_t will_retain    :1;
	uint8_t pswd           :1;
	uint8_t user_name      :1;
}conn_flags_t;


typedef struct{
	uint8_t session_pres   :1;
	uint8_t reserved 	   :7;
}connect_ack_Flags;


typedef struct{
	uint8_t control_type;
	uint8_t remainin_len;
	connect_ack_Flags ack_flags;
	uint8_t conn_code;
}conn_ack_t;


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
	uint16_t * len;
	char * name;
	uint16_t  * packet_id;
}pub_var_head_t;


typedef struct{
	pub_fix_head_t fix_head;
	pub_var_head_t var_head;
	uint8_t * pld;
}pub_pck_t;


typedef struct{
	uint8_t control_type;
	uint8_t remainin_len;
	uint16_t packet_id;
}publish_ack_t;



/*---------subscribe-------------------*/

typedef struct{
	uint8_t reserved :4;
	uint8_t type :4;
}subs_ctrl_byte_t;


typedef struct {
	subs_ctrl_byte_t * subs_ctrl_byte;
	uint32_t rem_len;
}sub_fix_head_t;


typedef struct{
	uint16_t  * packet_id;
}sub_var_head_t;


typedef struct{
	uint16_t *len;
	char *name;
	uint8_t *qos;
}sub_topic_ptr_t;


typedef struct{
	sub_fix_head_t fix_head;
	sub_var_head_t var_head;
	sub_topic_ptr_t pld_topics[MAX_SUBS_TOPIC];
}sub_pck_t;


typedef struct{
	uint16_t len;
	char name[MAX_TOPIC_NAME_SIZE];
	uint8_t qos;
}sub_topic_t;



typedef struct{
	uint8_t control_type;
	uint8_t remainin_len;
	uint16_t packet_id;
	uint8_t payload[MAX_SUBS_TOPIC];
}sub_ack_t;


/*---------ping-------------------*/

typedef struct{
	uint8_t reserved :4;
	uint8_t type :4;
}ping_ctrl_byte_t;


typedef struct{
	ping_ctrl_byte_t* ctrl_byte;
	uint8_t *rem_len;
}ping_req_fix_head_t;

typedef struct{
	ping_ctrl_byte_t ctrl_byte;
	uint8_t rem_len;
}ping_rsp_fix_head_t;


typedef struct{
	ping_req_fix_head_t fix_head;
}ping_req_pck_t;

typedef struct{
	ping_rsp_fix_head_t fix_head;
}ping_rsp_pck_t;


/*---------broker-------------------*/

typedef struct {
	sockaddr_t sockaddr;
	char  id[MAX_ID_SIZE];
	uint16_t keepalive;
	char  username[MAX_USR_NAME_SIZE];
	char  password[MAX_PSWD_NAME_SIZE];
	bool last_will;
	char  will_topic[MAX_TOPIC_NAME_SIZE];
	char  will_msg[MAX_WILL_MSG_SIZE];
	uint8_t will_qos;
	uint8_t will_retain;
	sub_topic_t subs_topic[MAX_SUBS_TOPIC];
	bool connected;
}  tb_client_t;


typedef struct{
	broker_net_conn connect;
	broker_net_send send;
	broker_net_rec receive;
	broker_net_discon disconnect;
}broker_net_t;


typedef struct{
	tb_client_t clients[MAX_CONN_CLIENTS];
	broker_net_t * net;
}broker_t;



/*----------------misc---------------*/

typedef struct{
	uint32_t value;
	uint8_t bytes_nb;
}rem_length_t;




/*----------------function declaration---------------*/

/*there is two way to initialize broker - by given net struct or all callback directly*/
void broker_init_by_given_net(broker_t * broker, broker_net_t * broker_net);
void broker_init_directly (broker_t * broker,
		broker_net_conn connect,
		broker_net_send send,
		broker_net_rec receive,
		broker_net_discon disconnect); //

rem_length_t decode_pck_len (uint8_t * frame);

void broker_decode_connect(uint8_t * frame, conn_pck_t *conn_pck);
bool was_clean_session_requested(conn_pck_t *conn_pck);
uint8_t broker_validate_conn(broker_t * broker, conn_pck_t *conn_pck);
uint8_t broker_can_accept_conn(broker_t * broker, conn_pck_t *conn_pck);
uint8_t * encode_conn_ack(conn_ack_t * header_ack, bool session_present, uint8_t code);

bool is_client_exist(broker_t * broker, char* client_id);
void broker_create_new_client(tb_client_t *new_client, const conn_pck_t * conn_pck,  sockaddr_t * sockaddr);
void add_client (broker_t * broker, tb_client_t * new_client);
tb_client_t * broker_get_client_by_socket(broker_t * broker, sockaddr_t * sockaddr);
bool broker_remove_client(broker_t * broker, char* client_id);

void broker_decode_publish(uint8_t* frame, pub_pck_t * pub_pck);
void publish_msg_to_subscribers(broker_t * broker, pub_pck_t * pub_pck);
void encode_publish_ack(publish_ack_t * publish_ack, uint16_t pckt_id);

uint8_t broker_decode_subscribe(uint8_t* frame, sub_pck_t * sub_pck);
bool add_subscriptions_from_packet(tb_client_t * client, sub_pck_t * sub_pck, uint8_t topic_nb, uint8_t * result_list);
void encode_subscribe_ack(sub_ack_t * sub_ack, uint16_t pckt_id, uint8_t topic_nb, uint8_t * result_list);


void broker_decode_ping_req(uint8_t* frame, ping_req_pck_t * ping_pck);
void broker_encode_ping_rsp(ping_rsp_pck_t* ping_pck);

#endif /* INC_TINY_BROKER_H_ */
