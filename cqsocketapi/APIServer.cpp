#include "stdafx.h"
#include <string>
#include "cqp.h"
#include "base64.h"

#include "appmain.h"
#include "APIClient.h"
#include "APIServer.h"


extern APIClient *client;
extern int appAuthCode;


/********
 * Message Processer
 ********/
void prcsClientHello(const char *payload) {
	int port;
	sscanf_s(payload, "%d", &port);
	CQ_addLog(appAuthCode, CQLOG_INFO, "qwq", payload);
	client->add(port);

	char* buffer = "ServerHello";
	client->send(buffer, strlen(buffer));
}

void prcsPrivateMessage(const char *payload) {
	int64_t id;
	char* text = new char[FRAME_PAYLOAD_SIZE];
	sscanf_s(payload, "%I64d %[^\n]", &id, text, sizeof(char) * FRAME_PAYLOAD_SIZE);
	
	char* decodedText = new char[FRAME_PAYLOAD_SIZE];
	Base64decode(decodedText, text);
	CQ_addLog(appAuthCode, CQLOG_WARNING, text, payload);
	CQ_sendPrivateMsg(appAuthCode, id, decodedText);
}

void prcsGroupMessage(const char *payload) {
	int64_t id;
	char* text = new char[FRAME_PAYLOAD_SIZE];
	sscanf_s(payload, "%I64d %[^\n]", &id, text, sizeof(char) * FRAME_PAYLOAD_SIZE);

	char* decodedText = new char[FRAME_PAYLOAD_SIZE];
	Base64decode(decodedText, text);

	CQ_sendGroupMsg(appAuthCode, id, decodedText);
}

void prcsGroupBan(const char *payload) {
	int64_t group, qq, duration;
	sscanf_s(payload, "%I64d %I64d %I64d", &group, &qq, &duration);

	CQ_setGroupBan(appAuthCode, group, qq, duration);
}

void prcsDiscussMessage(const char *payload) {
	int64_t id;
	char* text = new char[FRAME_PAYLOAD_SIZE];
	sscanf_s(payload, "%I64d %[^\n]", &id, text, sizeof(char) * FRAME_PAYLOAD_SIZE);
	
	char* decodedText = new char[FRAME_PAYLOAD_SIZE];
	Base64decode(decodedText, text);

	CQ_sendDiscussMsg(appAuthCode, id, decodedText);
}

void prscLike(const char *payload) {
	int64_t id;
	sscanf_s(payload, "%I64d", &id, sizeof(char) * FRAME_PAYLOAD_SIZE);
	CQ_sendLike(appAuthCode,id);
}

void prcsUnknownFramePrefix(const char *buffer) {
	char category[] = "UnknownFramePrefix";
	CQ_addLog(appAuthCode, CQLOG_WARNING, category, buffer);
}

/********
* 获取群成员
********/
void prcsGetGropuInfo(const char *payload) {
	int64_t groupId;
	int64_t qqId;
	sscanf_s(payload, "%I64d %I64d", &groupId, &qqId, sizeof(char)*FRAME_PAYLOAD_SIZE);
	const char * r = CQ_getGroupMemberInfoV2(appAuthCode, groupId, qqId, true);
	char *qwq;
	sprintf_s(qwq,sizeof(qwq), "group %c", r);
	client->send(r, sizeof(r));
}

/********
* 设置群成员名片
********/
void prcssetGroupCard(const char *payload) {
	int64_t groupId;
	int64_t qqId;
	char* text = new char[FRAME_PAYLOAD_SIZE];
	sscanf_s(payload, "%I64d %I64d %[^\n] ", groupId, qqId, text, sizeof(char)*FRAME_PAYLOAD_SIZE);
	CQ_setGroupCard(appAuthCode, groupId, qqId, text);
}


/********
* 设置群成员头衔
********/
void prcssetGroupSpecialTitle(const char *payload) {
	int64_t groupId;
	int64_t qqId;
	char* text = new char[FRAME_PAYLOAD_SIZE];
	sscanf_s(payload, "%I64d %I64d %[^\n] ", groupId, qqId, text, sizeof(char)*FRAME_PAYLOAD_SIZE);
	CQ_setGroupSpecialTitle(appAuthCode, groupId, qqId, text, -1);
}




/********
 * API Server
 ********/
APIServer::APIServer(void)
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(1, 1), &wsa);
	
	localInfo.sin_family = AF_INET;
	localInfo.sin_addr.s_addr = inet_addr("127.0.0.1");
	localInfo.sin_port = htons(SERVER_PORT);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	bind(sock, (sockaddr *)&localInfo, sizeof(localInfo));
}


APIServer::~APIServer(void)
{
	closesocket(sock);
}


void APIServer::run()
{
	char* buffer = new char[FRAME_SIZE];
	char* prefix = new char[FRAME_PREFIX_SIZE];
	char* payload = new char[FRAME_PAYLOAD_SIZE];

	while (1) {
		memset(buffer, 0, sizeof(char) * FRAME_SIZE);
		memset(prefix, 0, sizeof(char) * FRAME_PREFIX_SIZE);
		memset(payload, 0, sizeof(char) * FRAME_PAYLOAD_SIZE);
		if (recv(sock, buffer, sizeof(char) * FRAME_SIZE, 0) != SOCKET_ERROR) {
			sscanf_s(buffer, "%s %[^\n]", prefix, sizeof(char) * FRAME_PREFIX_SIZE, payload, sizeof(char) * FRAME_PAYLOAD_SIZE);
			CQ_addLog(appAuthCode, CQLOG_WARNING, prefix, buffer);
			if (strcmp(prefix, "ClientHello") == 0) {
				prcsClientHello(payload);
				continue;
			}
			if (strcmp(prefix, "PrivateMessage") == 0) {
				prcsPrivateMessage(payload);
				continue;
			}
			if (strcmp(prefix, "GroupMessage") == 0) {
				prcsGroupMessage(payload);
				continue;
			}
			if (strcmp(prefix, "GroupBan") == 0) {
				prcsGroupBan(payload);
				continue;
			}
			if (strcmp(prefix, "DiscussMessage") == 0) {
				prcsDiscussMessage(payload);
				continue;
			}
			if (strcmp(prefix, "Like") == 0) {
				prscLike(payload);
				continue;
			}
			if (strcmp(prefix, "getGroupInfo") == 0) {
				prcsGetGropuInfo(prefix);
				continue;
			}
			if (strcmp(prefix, "setGroupCard") == 0) {
				prcssetGroupCard(payload);
			}
			if (strcmp(prefix, "setGroupSpecialTitle") == 0) {
				prcssetGroupSpecialTitle(payload);
			}
			// Unknown prefix
			prcsUnknownFramePrefix(buffer);
		}
	}
}