
/*
 * tiny_broker.c
 *
 *  Created on: 07.05.2018
 *      Author: tomek
 */

#include <string.h>
#include "tiny_broker.h"



#define BROKER_TIMEOUT		60
#define TOPIC_POS 			6
#define M_HEAP_SIZE			512
#define X_MALLOC				m_malloc

#define X_HTONS(a) ((a>>8) | (a<<8))

//extern local_host_t local_lost;



void broker_init_directly (broker_t * broker,
		broker_net_conn connect,
		broker_net_send send,
		broker_net_rec receive,
		broker_net_discon disconnect){
	memset(broker, 0, sizeof(broker_t));
	broker->net->connect = connect;
	broker->net->send = send;
	broker->net->receive = receive;
	broker->net->disconnect = disconnect;
}


void broker_init_by_given_net(broker_t * broker, broker_net_t * broker_net){
	memset(broker, 0, sizeof(broker_t));
	broker->net = broker_net;
}


bool is_client_connected(broker_t * broker, char* client_id){
	for (uint8_t i =0; i < MAX_CONN_CLIENTS; i++){
		if (broker->clients[i].active){
			return true;
		}
	}
	return false;
}

static inline void read_connection_flags(conn_flags_t ** conn_flags, uint8_t * frame){
	uint8_t * flag_byte = &frame[9];
	*conn_flags =  (conn_flags_t*) flag_byte;
}



static inline bool can_broker_accept_next_client(broker_t * broker){
	for (uint8_t i = 0; i < MAX_CONN_CLIENTS; i++){
		if (!(broker->clients[i].active)){
			return true;
		}
	}
	return false;
}

static inline uint8_t broker_find_client_pos(broker_t * broker, char* client_id){
	for (uint8_t i = 0; i < MAX_CONN_CLIENTS; i++){
		if (strcmp(broker->clients[i].id, client_id) ==0 ) {
			return i;
		}
	}

	return NOT_FOUND;

}

bool broker_remove_client(broker_t * broker, char* client_id){
	uint8_t pos = broker_find_client_pos(broker, client_id);
	if (pos != NOT_FOUND){
		memset(&broker->clients[pos], 0, sizeof (conn_client_t));
		return true;
	}
	return false;
}


static uint8_t broker_first_free_pos_for_client(broker_t * broker){
	for (uint8_t i = 0; i < MAX_CONN_CLIENTS; i++){
		if (!(broker->clients[i].active)){
			return i;
		}
	}
	return NOT_FOUND;
}


static void add_client (broker_t * broker, conn_client_t * new_client){
	uint8_t pos = broker_first_free_pos_for_client(broker);
		memcpy(&broker->clients[pos], new_client, sizeof (conn_client_t));
}





void broker_packets_dispatcher (broker_t * broker, uint8_t * frame, sockaddr_t * sockaddr){
	uint8_t pckt_type = (frame[0]>>4);
	switch (pckt_type) {
	case PCKT_TYPE_CONNECT:{
		conn_pck_t conn_pck;
		broker_decode_connect(frame, &conn_pck);
		conn_result_t conn_result;
		broker_handle_new_connection(broker, &conn_pck, sockaddr, &conn_result);
		conn_ack_t conn_ack;
		encode_conn_ack(&conn_ack, &conn_result);
		broker->net->send(NULL, sockaddr, (uint8_t*)&conn_ack, sizeof(conn_ack_t) );
		break;
	}
	case PCKT_TYPE_PUBLISH:{
		pub_pck_t pub_pck;
		broker_decode_publish(frame, &pub_pck);
		publish_msg_to_subscribers(broker, &pub_pck);
		publish_ack_t publish_ack;
		encode_publish_ack(&publish_ack, pub_pck.var_head.packet_id);
	}

	}


}










/*-------------------------------CONNECT-----------------------------------------*/


void broker_decode_connect(uint8_t * frame, conn_pck_t *conn_pck){
	uint8_t pos = 0;
	conn_pck->fix_head.ctrl_byte =  (conn_ctrl_byte_t *) &frame[pos];
	pos ++;
	rem_length_t rem_length = decode_pck_len(&frame[pos]);
	conn_pck->fix_head.rem_len = rem_length.value;
	pos += rem_length.bytes_nb;
	conn_pck->var_head.len = (uint16_t*) &frame[pos];
	*conn_pck->var_head.len  = X_HTONS(*conn_pck->var_head.len);
	pos += 2;

	conn_pck->var_head.proto_name = (char*) &frame[pos];
	pos += *conn_pck->var_head.len;
	conn_pck->var_head.proto_level = (uint8_t*) &frame[pos];//8
	pos += 1;
	conn_pck->var_head.conn_flags = (conn_flags_t*) &frame[pos];//9
	pos += 1;
	conn_pck->var_head.keep_alive = (uint16_t*)  &frame[pos];//10+11
	*conn_pck->var_head.keep_alive = X_HTONS(*conn_pck->var_head.keep_alive);
	pos += 2;
	conn_pck->pld.client_id_len  = (uint16_t*) &frame[pos];//1213
	*conn_pck->pld.client_id_len = X_HTONS(*conn_pck->pld.client_id_len);
	pos += 2;
	conn_pck->pld.client_id = (char*) &frame[pos];
	pos += *conn_pck->pld.client_id_len;

	if (conn_pck->var_head.conn_flags->last_will){
		conn_pck->pld.will_topic_len = (uint16_t*)  &frame[pos];
		*conn_pck->pld.will_topic_len = X_HTONS(* conn_pck->pld.will_topic_len);
		pos += 2;
		conn_pck->pld.will_topic = (char*)  &frame[pos];
		pos += *conn_pck->pld.will_topic_len;

		conn_pck->pld.will_msg_len = (uint16_t*)  &frame[pos];
		*conn_pck->pld.will_msg_len = X_HTONS(* conn_pck->pld.will_msg_len);
		pos += 2;
		conn_pck->pld.will_msg = (char*)  &frame[pos];
		pos += *conn_pck->pld.will_msg_len;
	}
	if (conn_pck->var_head.conn_flags->user_name){
		conn_pck->pld.usr_name_len = (uint16_t*)  &frame[pos];
		*conn_pck->pld.usr_name_len = X_HTONS(* conn_pck->pld.usr_name_len);
		pos += 2;
		conn_pck->pld.usr_name= (char*) &frame[pos];
		pos += *conn_pck->pld.usr_name_len;
	}
	if (conn_pck->var_head.conn_flags->pswd){
		conn_pck->pld.pswd_len = (uint16_t*)  &frame[pos];
		*conn_pck->pld.pswd_len = X_HTONS(* conn_pck->pld.pswd_len);
		pos += 2;
		conn_pck->pld.pswd= (char*) &frame[pos];
		pos += *conn_pck->pld.pswd_len;
	}
}


__attribute__( ( weak ) ) bool is_client_authorised (char* usr_name, char* pswd){
	return true;
}


uint8_t * encode_conn_ack(conn_ack_t * header_ack, conn_result_t * conn_res){
	memset(header_ack, 0, sizeof (conn_ack_t));
	header_ack->control_type = (CONTR_TYPE_CONNACK << 4);
	header_ack->remainin_len = CONN_ACK_PLD_LEN;
	header_ack->ack_flags.session_pres = conn_res->session_present;
	header_ack->conn_code = conn_res->code;
	return (uint8_t *)header_ack;
}



static void broker_fill_new_client(conn_client_t *new_client, const conn_pck_t * conn_pck,  sockaddr_t * sockaddr){
	memset(new_client, 0, sizeof(conn_client_t));
	memcpy(&new_client->sockaddr, &sockaddr, sizeof (sockaddr_t));

	strncpy(new_client->id,  conn_pck->pld.client_id, *conn_pck->pld.client_id_len);

	new_client->keepalive = *conn_pck->var_head.keep_alive;

	if (conn_pck->var_head.conn_flags->will_retain){
		new_client->will_retain = 1;
	}

	if (conn_pck->var_head.conn_flags->last_will){
		new_client->will_retain = 1;
		strncpy(new_client->will_topic,  conn_pck->pld.will_topic, *conn_pck->pld.will_topic_len );
		strncpy(new_client->will_msg,  conn_pck->pld.will_msg, *conn_pck->pld.will_msg_len);
		new_client->will_qos = conn_pck->var_head.conn_flags->will_qos;
	}

	if (conn_pck->var_head.conn_flags->user_name){
		strncpy(new_client->username,  conn_pck->pld.usr_name, *conn_pck->pld.usr_name_len);
	}

	if (conn_pck->var_head.conn_flags->pswd){
		strncpy(new_client->password,  conn_pck->pld.pswd, *conn_pck->pld.pswd_len);
	}
}



// https://www.bevywise.com/developing-mqtt-clients/
// https://morphuslabs.com/hacking-the-iot-with-mqtt-8edaf0d07b9b ack codes


void broker_handle_new_connection (broker_t *broker, conn_pck_t *conn_pck, sockaddr_t * sockaddr,  conn_result_t * conn_res){

	if  (*conn_pck->var_head.proto_level != PROTO_LEVEL_MQTT311){
		conn_res->session_present = false;
		conn_res->code = CONN_ACK_BAD_PROTO;
		return;
	}

	if (conn_pck->var_head.conn_flags->cleans_session){
		if (broker_remove_client(broker, conn_pck->pld.client_id)){
		}
	}

	if (is_client_connected(broker, conn_pck->pld.client_id)){
		conn_res->session_present = true;
		conn_res->code = CONN_ACK_OK;
		return;
	}

	if (can_broker_accept_next_client(broker))
	{
		conn_client_t new_client;
		broker_fill_new_client(&new_client, conn_pck, sockaddr);

		if (is_client_authorised(new_client.username, new_client.password)){
			add_client(broker, &new_client);
			conn_res->session_present = false;
			conn_res->code = CONN_ACK_OK;
			return;
		}else{
			conn_res->session_present = false;
			conn_res->code = CONN_ACK_BAD_AUTH;
			return;
		}
	} else {
		conn_res->session_present = false;
		conn_res->code = CONN_ACK_NOT_AVBL;
		return;
	}
}


/*-------------------------------PUBLIHS-----------------------------------------*/

 void broker_decode_publish(uint8_t* frame, pub_pck_t * pub_pck){
	uint8_t pos = 0;

	pub_pck->fix_head.ctrl_byte = (pub_ctrl_byte_t *) frame;
	pos ++;
	rem_length_t rem_length = decode_pck_len(&frame[pos]);
	pub_pck->fix_head.rem_len = rem_length.value;
	pos += rem_length.bytes_nb;



	pub_pck->var_head.topic_name_len  = (uint16_t*) &frame[pos];
	*pub_pck->var_head.topic_name_len = X_HTONS(*pub_pck->var_head.topic_name_len);
	pos += 2;

	pub_pck->var_head.topic_name = (char*)  &frame[pos];
	pos += *pub_pck->var_head.topic_name_len;

	if (pub_pck->fix_head.ctrl_byte->QoS > 0){
		pub_pck->var_head.packet_id  = (uint16_t*) &frame[pos];
		*pub_pck->var_head.packet_id = X_HTONS(*pub_pck->var_head.packet_id);
		pos += 2;
	}
	pub_pck->pld = &frame[pos];
}




void publish_msg_to_subscribers(broker_t * broker, pub_pck_t * pub_pck){
	for (uint8_t i =0; i < MAX_CONN_CLIENTS; i++){
		if ((broker->clients[i].active)){
			for (uint8_t j =0; j < MAX_SUBS_TOPIC; j++){
				uint16_t len = *pub_pck->var_head.topic_name_len;
				char* topic = pub_pck->var_head.topic_name;
				if (strncmp(broker->clients[i].subs_topic[j].topic_name, topic, len)){
					broker->net->send(NULL, &broker->clients[i].sockaddr, (uint8_t*)&pub_pck, sizeof(pub_pck_t) );
					break;
				}
			}
		}
	}
}



rem_length_t decode_pck_len (uint8_t * frame){
	uint8_t multiplier = 1;
	uint8_t  encodedByte;
	rem_length_t rem_length;
	memset(&rem_length, 0, sizeof(rem_length_t));
	const uint8_t max_nb_bytes = 4;
	do{
		encodedByte = frame[rem_length.bytes_nb];
		rem_length.value += (encodedByte & 127) * multiplier;
		multiplier *= 128;
		rem_length.bytes_nb++;
		if (rem_length.bytes_nb == max_nb_bytes){
			break;
		}
	}while ((encodedByte & 128) != 0);
	return rem_length;
}



void encode_publish_ack(publish_ack_t * publish_ack, uint16_t pckt_id){
	publish_ack->control_type = (PCKT_TYPE_PUBLISH << 4);
	publish_ack->remainin_len = 2;
	publish_ack->packet_id = pckt_id;
}

/*-------------------------------SUBSCRIBE-----------------------------------------*/

void broker_decode_subscribe(uint8_t* frame, sub_pck_t * sub_pck){
	uint8_t pos = 0;

	sub_pck->fix_head.subs_ctrl_byte = (subs_ctrl_byte_t *) frame;
	pos++;
	rem_length_t rem_length = decode_pck_len(&frame[pos]);
	sub_pck->fix_head.rem_len = rem_length.value;
	pos += rem_length.bytes_nb;


	sub_pck->var_head.packet_id  = (uint16_t*) &frame[pos];
	*sub_pck->var_head.packet_id = X_HTONS(*sub_pck->var_head.packet_id);
	pos += 2;

	const uint8_t fix_head_size = 2;
	uint8_t topic_nb =0;
	while (pos < (sub_pck->fix_head.rem_len + fix_head_size)){
		sub_pck->pld_topics[topic_nb].topic_name_len = (uint16_t *)  &frame[pos];
		*sub_pck->pld_topics[topic_nb].topic_name_len  = X_HTONS(*sub_pck->pld_topics[topic_nb].topic_name_len );
		pos += 2;
		sub_pck->pld_topics[topic_nb].topic_name =  (char*)  &frame[pos];
		pos += (*sub_pck->pld_topics[topic_nb].topic_name_len);
		sub_pck->pld_topics[topic_nb].qos = (uint8_t*) &frame[pos];
		pos += 1;
		topic_nb++;
	}
}


void add_subscribtion(conn_client_t *client, sub_pck_t * sub_pck){
	for (uint8_t i=0; i < MAX_SUBS_TOPIC; i++){
		for (uint8_t j =0; j < MAX_SUBS_TOPIC; j++){
			/*to be sure that one topic is not subset of another filter*/
			if (client->subs_topic[i].used){
				if (client->subs_topic[i].topic_name_len == *sub_pck->pld_topics[j].topic_name_len){
					if (memcmp(client->subs_topic[i].topic_name, sub_pck->pld_topics[j].topic_name, *sub_pck->pld_topics[j].topic_name_len)){
						client->subs_topic[i].qos = *sub_pck->pld_topics[j].qos;
					}
				}
			}else{
				memcpy(&client->subs_topic[i], &sub_pck->pld_topics[j], *sub_pck->pld_topics[j].topic_name_len);
				client->subs_topic[i].used = false;
				break;
			}
		}
	}
}






//broker->net->write(context, buf, buf_len, timeout_ms);
//broker->net->write(void *context, const byte* buf, int buf_len, int timeout_ms);

