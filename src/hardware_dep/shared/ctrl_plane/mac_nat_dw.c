#include "controller.h"
#include "messages.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_MACS 1000000

controller c;

uint8_t macs[MAX_MACS][6];
uint8_t portmap[MAX_MACS];
uint8_t ips[MAX_MACS][4];
uint8_t ipd[MAX_MACS][4];
uint16_t dtcp_txt[MAX_MACS];
uint8_t dtcp_new_txt[MAX_MACS][2];

int mac_count = -1;

int read_macs_and_ports_from_file(char *filename) {
    FILE *f;
    char line[200];
    int values[6];
    int values_ip[4];
    uint16_t values_dtcp;
    int values_dtcp_nw[2];
    int port;
    //int values_stcp;
    int i;

    printf("Read the trace file \n");

    f = fopen(filename, "r");
    if (f == NULL) return -1;

    while (fgets(line, sizeof(line), f)) {
        line[strlen(line)-1] = '\0';
        if (13 == sscanf(line, "%x:%x:%x:%x:%x:%x %d.%d.%d.%d %d %d %d",
                    &values[0], &values[1], &values[2],
                    &values[3], &values[4], &values[5],
                    &values_ip[0], &values_ip[1], &values_ip[2], &values_ip[3],
                    &values_dtcp, &values_dtcp_nw[1],
                    &port
                    ) )
        {
            if (mac_count==MAX_MACS-1)
            {
                printf("Too many entries...\n");
                break;
            }

            ++mac_count;
            for( i = 0; i < 6; ++i )
                macs[mac_count][i] = (uint8_t) values[i];
            for( i = 0; i < 4; ++i )
                ips[mac_count][i] = (uint8_t) values_ip[i];
            //dtcp_txt[mac_count][1] = (uint8_t) values_dtcp[1];
           // dtcp_txt[mac_count][0] = 0;
            dtcp_txt[mac_count] = values_dtcp;
            portmap[mac_count] = (uint8_t) port;

        } else {
            printf("Wrong format error in line %d : %s\n", mac_count+2, line);
            fclose(f);
            return -1;
        }
    }
    printf("Total table entries to be inserted is %d\n", mac_count);
    fclose(f);
    return 0;
}

//void set_default_action_ipv4_lpm()
//{
//	char buffer[2048];
//	struct p4_header* h;
//	struct p4_set_default_action* sda;
//	struct p4_action* a;
//
//	h = create_p4_header(buffer, 0, sizeof(buffer));
//	sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
//        strcpy(sda->table_name, "ipv4_lpm");
//
//	a = &(sda->action);
//	strcpy(a->description.name, "drop");
//
//	netconv_p4_header(h);
//	netconv_p4_set_default_action(sda);
//	netconv_p4_action(a);
//	send_p4_msg(c, buffer, sizeof(buffer));
//
//	printf("########## \n");
//	printf("\n");
//	printf("Table name: %s\n", sda->table_name);
//	printf("default action: %s\n", a->description.name);
//}

void set_default_action_nat_up()
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_set_default_action* sda;
    struct p4_action* a;

    h = create_p4_header(buffer, 0, sizeof(buffer));

    sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
    strcpy(sda->table_name, "nat_up");

    a = &(sda->action);
    strcpy(a->description.name, "natTcp_learn");

    netconv_p4_header(h);
    netconv_p4_set_default_action(sda);
    netconv_p4_action(a);

    send_p4_msg(c, buffer, sizeof(buffer));

    printf("\n");
    printf("Table name: %s\n", sda->table_name);
    printf("### Default action: %s\n", a->description.name);
}

void set_default_action_nat_dw()
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_set_default_action* sda;
    struct p4_action* a;

    h = create_p4_header(buffer, 0, sizeof(buffer));

    sda = create_p4_set_default_action(buffer,0,sizeof(buffer));
    strcpy(sda->table_name, "nat_dw");

    a = &(sda->action);
    strcpy(a->description.name, "_drop");

    netconv_p4_header(h);
    netconv_p4_set_default_action(sda);
    netconv_p4_action(a);

    send_p4_msg(c, buffer, sizeof(buffer));

    printf("\n");
    printf("Table name: %s\n", sda->table_name);
    printf("### Default action: %s\n", a->description.name);
}

void fill_if_info(uint8_t is_int_if)
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_add_table_entry* te;

    struct p4_action* a;
    struct p4_action_parameter* ap;

    struct p4_field_match_exact* exact;

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "if_info");

    exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, "standard_metadata.ingress_port");
    exact->bitmap[0] = is_int_if;
    exact->bitmap[1] = 0;
    exact->length = 2*8+0;
    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "set_if_info");

    ap = add_p4_action_parameter(h, a, 2048);
    strcpy(ap->name, "is_int_if");
    ap->bitmap[0] = is_int_if;
    ap->bitmap[1] = 0;
    ap->length = 2*8+0;

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap);

    send_p4_msg(c, buffer, 2048);
}

void fill_smac(uint8_t mac[6])
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_field_match_exact* exact;

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "smac");

    exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, "ethernet.srcAddr"); // key
    memcpy(exact->bitmap, mac, 6);
    exact->length = 6*8+0;

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "_nop");

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);
    send_p4_msg(c, buffer, 2048);
}

void fill_nat_dw(uint16_t dstPort, uint8_t ip[4])
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_action_parameter* ap;

    struct p4_field_match_exact* exact;

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "nat_dw");

    exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, "tcp.dstPort");
    exact->bitmap[1] = (uint8_t)dstPort;
    exact->bitmap[0] = 0;
    exact->length = 2*8+0;

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "nat_hit_ext_to_int");

    ap = add_p4_action_parameter(h, a, 2048);
    strcpy(ap->name, "dstAddr");
    memcpy(ap->bitmap, ip, 4);
    ap->length = 4*8+0;

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap);

    send_p4_msg(c, buffer, 2048);
}

void fill_ipv4_lpm(uint8_t dst_ip[4], uint8_t port, uint8_t dst_mac[6])
{
    char buffer[2048];
    struct p4_header* h;
    struct p4_add_table_entry* te;
    struct p4_action* a;
    struct p4_action_parameter* ap,* ap2;
    struct p4_field_match_exact* exact;

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "ipv4_lpm");

    exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, "ipv4.dstAddr");
    memcpy(exact->bitmap, dst_ip, 4);
    exact->length = 4*8+0;

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "set_nhop");

    ap = add_p4_action_parameter(h, a, 2048);
    strcpy(ap->name, "port");
    ap->bitmap[0] = port;
    ap->bitmap[1] = 0;
    ap->length = 2*8+0;

    ap2 = add_p4_action_parameter(h, a, 2048);
    strcpy(ap2->name, "dstAddr");
    memcpy(ap2->bitmap, dst_mac, 6);
    ap2->length = 6*8+0;

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap);
    netconv_p4_action_parameter(ap2);
    send_p4_msg(c, buffer, 2048);
}

void fill_sendout_table(uint8_t port, uint8_t src_mac[6])
{
    char buffer[2048];
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

    a = add_p4_action(h, 2048);
    strcpy(a->description.name, "rewrite_src_mac");

    ap = add_p4_action_parameter(h, a, 2048);
    strcpy(ap->name, "srcAddr");
    memcpy(ap->bitmap, src_mac, 6);
    ap->length = 6*8+0;

    netconv_p4_header(h);
    netconv_p4_add_table_entry(te);
    netconv_p4_field_match_exact(exact);
    netconv_p4_action(a);
    netconv_p4_action_parameter(ap);
    send_p4_msg(c, buffer, 2048);

// Print the mac address
/*
    for(int i = 0; i < 6; i++){
        printf("%d",ap->bitmap[i]);
    }
*/
}

void dhf(void* b) {
    printf("Unknown digest received\n");
}

void init() {
    int i;
    uint8_t port1 = 1;
    uint8_t ip_dst_dw[4] = {10,0,0,10};
    uint8_t smac_2[6] = {0x00, 0x55, 0x00, 0x00, 0x00, 0x00};
    uint8_t mac_if1[6] = {0xaa, 0xbb, 0xcc, 0xaa, 0xdd, 0xee};
    uint8_t is_ext = 0;
    uint8_t destport = 25;

    printf("Set default actions.\n");
    set_default_action_nat_up();
    set_default_action_nat_dw();

#if 1
    fill_smac(smac_2);
    fill_if_info(is_ext);
    fill_nat_dw(destport, ip_dst_dw);

    fill_ipv4_lpm(ip_dst_dw, port1, ip_dst_dw);
#endif
    printf("calling fill sendout table \n");
    fill_sendout_table(port1, mac_if1);

    for (i=0;i<=mac_count;++i)
    {
//        printf("\n Number of entries to be inserted is %d\n", mac_count);

        fill_smac(macs[i]);
        fill_nat_dw(dtcp_txt[i], ips[i]);
        fill_ipv4_lpm(ips[i], port1, ips[i]);

        if(0 == (i%500)){ printf("inside sleep \n");sleep(1);;}
    }
    printf("<<<Insert loop finished with %d table entries>>>\n", i);
    printf("\n");
}

int main(int argc, char* argv[])
{
    if (argc>1) {
        if (argc!=2) {
            printf("Too many arguments...\nUsage: %s <filename(optional)>\n", argv[0]);
            return -1;
        }
        printf("Command line argument is present...\nLoading configuration data...\n");
        if (read_macs_and_ports_from_file(argv[1])<0) {
            printf("File cannnot be opened...\n");
            return -1;
        }
    }

    printf("Create and configure controller...\n");
    c = create_controller_with_init(11111, 3, dhf, init);
    printf("MACSAD controller started...\n");
    execute_controller(c);

    printf("MACSAD controller terminated\n");
    destroy_controller(c);
    return 0;
}
