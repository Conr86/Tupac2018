#include "UDPServer.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

int udp_server_init(udp_server_ref server, uint16_t port)
{
	server->socket = socket(AF_INET, SOCK_DGRAM, 0);
	if (server->socket < 0) {
		perror("socket");
		return errno;
	}
	const int flag = 1;
	if (setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0) {
		perror("setsockopt");
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(server->socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		return errno;
	}
	return 0;
}

ssize_t udp_server_read(udp_server_ref server, uint8_t *buf, size_t bufSize)
{
	return recv(server->socket, buf, bufSize, 0);
}

ssize_t udp_server_readFrom(udp_server_ref server, uint8_t *buf, size_t bufSize, udp_address_t *source)
{
	memset(&source->address, 0, sizeof(struct sockaddr_in));
	source->addressLength = sizeof(struct sockaddr_in);
	return recvfrom(server->socket, buf, bufSize, 0, (struct sockaddr *)&source->address, &source->addressLength);
}

int udp_server_connect(udp_server_ref server, const udp_address_t *destination)
{
	int ret = connect(server->socket, (struct sockaddr *)&destination->address, destination->addressLength);
	if (ret < 0) perror("connect");
	return ret;
}

ssize_t udp_server_write(udp_server_ref server, const uint8_t *data, size_t length)
{
	return write(server->socket, data, length);
}

ssize_t udp_server_writeTo(udp_server_ref server, const uint8_t *data, size_t length, udp_address_t *dest)
{
	ssize_t ret = sendto(server->socket, data, length, 0, (struct sockaddr *)&dest->address, dest->addressLength);
	if (ret < 0) {
		perror("sendto");
	}
	return ret;
}

int udp_server_enable_broadcast(udp_server_ref server, int enable)
{
	if (setsockopt(server->socket, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable)) < 0) {
		perror("setsockopt");
		return errno;
	}
	return 0;
}
