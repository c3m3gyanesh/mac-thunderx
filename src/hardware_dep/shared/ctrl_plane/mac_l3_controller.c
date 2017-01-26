#include "controller.h"
#include "messages.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

controller c;

void fill_ipv4_fib_lpm_table(uint8_t ip[4], uint8_t port, uint8_t mac[6])
{
	char buffer[2048]; /* TODO: ugly */
	struct p4_header* h;
	struct p4_add_table_entry* te;
	struct p4_action* a;
	struct p4_action_parameter* ap,* ap2;
	struct p4_field_match_exact* exact; // TODO: replace to lpm

	h = create_p4_header(buffer, 0, 2048);
	te = create_p4_add_table_entry(buffer,0,2048);
	strcpy(te->table_name, "ipv4_fib_lpm");

	exact = add_p4_field_match_exact(te, 2048);
	strcpy(exact->header.name, "ipv4.dstAddr");
	memcpy(exact->bitmap, ip, 4);
	exact->length = 4*8+0;

	a = add_p4_action(h, 2048);
	strcpy(a->description.name, "fib_hit_nexthop");

	ap = add_p4_action_parameter(h, a, 2048);
	strcpy(ap->name, "dmac");
	memcpy(ap->bitmap, mac, 6);
	ap->length = 6*8+0;

	ap2 = add_p4_action_parameter(h, a, 2048);
	strcpy(ap2->name, "port");
	ap2->bitmap[0] = port;
	ap2->bitmap[1] = 0;
	ap2->length = 2*8+0;

	netconv_p4_header(h);
	netconv_p4_add_table_entry(te);
	netconv_p4_field_match_exact(exact);
	netconv_p4_action(a);
	netconv_p4_action_parameter(ap);
	netconv_p4_action_parameter(ap2);

	send_p4_msg(c, buffer, 2048);
}

void fill_sendout_table(uint8_t port, uint8_t smac[6])
{
    char buffer[2048]; /* TODO: ugly */
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
	struct p4_action_parameter* ap;
	struct p4_field_match_exact* exact;

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "sendout");

    exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, "standard_metadata.egress_port");
    exact->bitmap[0] = port;
	exact->bitmap[1] = 0;
    exact->length = 2*8+0;

	 printf("exact bitmap %d:%d, exact length %d \n",exact->bitmap[0],exact->bitmap[1],exact->length);

	a = add_p4_action(h, 2048);
	strcpy(a->description.name, "rewrite_src_mac");

	ap = add_p4_action_parameter(h, a, 2048);
	strcpy(ap->name, "smac");
	memcpy(ap->bitmap, smac, 6);
	ap->length = 6*8+0;

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap);
    send_p4_msg(c, buffer, 2048);
}

void test_learn_ip(void* b) {
	uint8_t ip[4];
	uint16_t pr;
	int i1;
	int i2;
	int i3;
	uint16_t offset=0;
	offset = sizeof(struct p4_digest);
	struct p4_digest_field* df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
	memcpy(ip, df->value, 4);
	offset += sizeof(struct p4_digest_field);
	df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
	memcpy(&pr, df->value, 2);
	offset += sizeof(struct p4_digest_field);
	df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
	memcpy(&i1, df->value, 4);
	offset += sizeof(struct p4_digest_field);
	df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
	memcpy(&i2, df->value, 4);
	offset += sizeof(struct p4_digest_field);
	df = netconv_p4_digest_field(unpack_p4_digest_field(b, offset));
	memcpy(&i3, df->value, 4);

	printf("test_learn_ip\n");

	char buffer[2048]; /* TODO: ugly */
	struct p4_header* h;
	struct p4_add_table_entry* te;
	struct p4_action* a;
	struct p4_action_parameter *ap1, *ap2, *ap3;
	struct p4_field_match_lpm* lpm;
	h = create_p4_header(buffer, 0, 2048);
	te = create_p4_add_table_entry(buffer,0,2048);
	strcpy(te->table_name, "table1");
	lpm = add_p4_field_match_lpm(te, 2048);
	strcpy(lpm->header.name, "field1");
	memcpy(lpm->bitmap, ip, 4);
	lpm->prefix_length = pr;
	a = add_p4_action(h, 2048);
	strcpy(a->description.name, "korte");
	ap1 = add_p4_action_parameter(h, a, 2048);
	memcpy(ap1->bitmap, &i1, 4);
	ap2 = add_p4_action_parameter(h, a, 2048);
	memcpy(ap2->bitmap, &i2, 4);
	ap3 = add_p4_action_parameter(h, a, 2048);
	memcpy(ap3->bitmap, &i3, 4);
	netconv_p4_header(h);
	netconv_p4_add_table_entry(te);
	netconv_p4_field_match_lpm(lpm);
	netconv_p4_action(a);
	netconv_p4_action_parameter(ap1);
	netconv_p4_action_parameter(ap2);
	netconv_p4_action_parameter(ap3);
	send_p4_msg(c, buffer, 2048);
}

void dhf(void* b) {
	printf("Unknown digest received\n");
}

int main()
{
	uint8_t ip[4] = {192,168,10,20};
//	uint8_t mac[6] = {0xd1, 0x69, 0x0f, 0xa8, 0x39, 0x91};
        uint8_t mac[6] = {0x3c, 0xfd, 0x27, 0xba, 0xe2, 0x90};
	uint8_t port = 0;

	uint8_t ip2[4] = {192,168,10,10};
//	uint8_t mac2[6] = {0xd2, 0x69, 0x0f, 0xa8, 0x39, 0x92};
        uint8_t mac2[6] = {0x3d, 0xfd, 0x27, 0xba, 0xe2, 0x90};
	uint8_t port2 = 1;

    uint8_t smac[6] = {0xd0, 0x69, 0x0f, 0xa8, 0x39, 0x90};

	printf("Create and configure l3 test controller...\n");
	c = create_controller(11111, 3, dhf);

	fill_ipv4_fib_lpm_table(ip, port, mac);
	fill_ipv4_fib_lpm_table(ip2, port2, mac2);

	fill_sendout_table(port, smac);
	fill_sendout_table(port2, smac);

	printf("Launching controller's main loop...\n");
	execute_controller(c);

	printf("Destroy controller\n");
	destroy_controller(c);

	return 0;
}

