#include "controller.h"                                                         
#include "messages.h"                                                           
#include <unistd.h>                                                             
#include <stdio.h>                                                              
#include <string.h>                                                             
#include <time.h>                                                               
                                                                                
#define MAX_MACS 2000000                                                        
                                                                                
controller c;                                                                   
                                                                                
void fill_ipv6_fib_lpm_table(uint8_t ip[16], uint8_t port, uint8_t mac[6])      
{                                                                               
    char buffer[2048]; /* TODO: ugly */                                         
    struct p4_header* h;                                                        
    struct p4_add_table_entry* te;                                              
    struct p4_action* a;                                                        
    struct p4_action_parameter* ap,* ap2;                                       
    struct p4_field_match_exact* exact; // TODO: replace to lpm                 
                                                                                
  //  printf("LPM table update \n");                                            
                                                                                
    h = create_p4_header(buffer, 0, 2048);                                      
    te = create_p4_add_table_entry(buffer,0,2048);                              
    strcpy(te->table_name, "ipv6_fib_lpm");                                     
                                                                                
    exact = add_p4_field_match_exact(te, 2048);                                 
    strcpy(exact->header.name, "ipv6.dstAddr");                                 
    memcpy(exact->bitmap, ip, 16);                                              
    exact->length = 16*8+0;                                                     
                                                                                
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
    //printf("Sendout table update \n");

    h = create_p4_header(buffer, 0, 2048);
    te = create_p4_add_table_entry(buffer,0,2048);
    strcpy(te->table_name, "sendout");

    exact = add_p4_field_match_exact(te, 2048);
    strcpy(exact->header.name, "standard_metadata.egress_port");
    exact->bitmap[0] = port;
    exact->bitmap[1] = 0;
    exact->length = 2*8+0;

     //printf("exact bitmap %d:%d, exact length %d \n",exact->bitmap[0],exact->bitmap[1],exact->length);

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

uint8_t macs[MAX_MACS][6];
uint8_t portmap[MAX_MACS];
uint8_t ips[MAX_MACS][16];
int mac_count = -1;

int read_macs_and_ports_from_file(char *filename) {
    FILE *f;
    char line[200];
    int values[6];
    int values_ip[16];
    int port;
    int i;

    f = fopen(filename, "r");
    if (f == NULL) return -1;

    while (fgets(line, sizeof(line), f)) {
        line[strlen(line)-1] = '\0';
        //TODO why %c?
        if (23 == sscanf(line, "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x %x:%x:%x:%x:%x:%x %d",
                    &values_ip[0], &values_ip[1], &values_ip[2], &values_ip[3],
                    &values_ip[4], &values_ip[5], &values_ip[6], &values_ip[7],
                    &values_ip[8], &values_ip[9], &values_ip[10], &values_ip[11],
                    &values_ip[12], &values_ip[13], &values_ip[14], &values_ip[15],
                    &values[0], &values[1], &values[2],
                    &values[3], &values[4], &values[5], &port) )
        {
            if (mac_count==MAX_MACS-1)
            {
                printf("Too many entries...\n");
                break;
            }

            ++mac_count;
            for( i = 0; i < 6; ++i )
                macs[mac_count][i] = (uint8_t) values[i];
            for( i = 0; i < 16; ++i )
                ips[mac_count][i] = (uint8_t) values_ip[i];
            portmap[mac_count] = (uint8_t) port;

        } else {
            printf("Wrong format error in line %d : %s\n", mac_count+2, line);
            fclose(f);
            return -1;
        }

    }

    fclose(f);
    return 0;
}

void dhf(void* b) {
    printf("Unknown digest received\n");
}

void init() {
    int i;
    uint8_t smac[6] = {0xd0, 0x69, 0x0f, 0xa8, 0x39, 0x90};
    printf("INIT");
    //TODO
    //printf("Set default actions.\n");
    //set_default_action_smac();
    //set_default_action_dmac();
    clock_t begin = clock();
    for (i=0;i<=mac_count;++i)
    {
        //printf("Filling tables IPV6 lpm_table/sendout_table "
        //  "PORT: %d "
        //  "MAC: %02x:%02x:%02x:%02x:%02x:%02x "
        //  "IP: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
        //      portmap[i], macs[i][0],macs[i][1],macs[i][2],macs[i][3],macs[i][4],macs[i][5],
        //      ips[i][0], ips[i][1], ips[i][2], ips[i][3],
        //      ips[i][4], ips[i][5], ips[i][6], ips[i][7],
        //      ips[i][8], ips[i][9], ips[i][10], ips[i][11],
        //      ips[i][12], ips[i][13], ips[i][14], ips[i][15]);
        fill_ipv6_fib_lpm_table(ips[i], portmap[i], macs[i]);

            if(0 == (i%1000)){ printf("%d inside sleep \n", i);sleep(1);;}
        fill_sendout_table(portmap[i], smac);
            usleep(1000);
    }

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

        printf ("ctrl Total entries sent %d time %f\n",i,time_spent);

}

int main(int argc, char* argv[])
{
    uint8_t ip[16] = {0x20,0x01,0x0d,0xb8,0x85,0xa3,0x08,0xd3,0x13,0x19,0x8a,0x2e,0x03,0x70,0x73,0x34};
    uint8_t mac[6] = {0xa0, 0x36, 0x9f, 0x3e, 0x94, 0xea};
    uint8_t port = 1;

    uint8_t ip2[16] = {0x13,0x19,0x8a,0x2e,0x03,0x70,0x73,0x35,0x20,0x01,0x0d,0xb8,0xff,0xff,0x08,0xd3};
    uint8_t mac2[6] = {0xa0, 0x36, 0x9f, 0x3e, 0x94, 0xe8};
    uint8_t port2 = 0;

    uint8_t smac[6] = {0xd0, 0x69, 0x0f, 0xa8, 0x39, 0x90};

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

    printf("Create and configure l3 test controller...\n");
    //c = create_controller(11111, 3, dhf, init);
    c = create_controller_with_init(11111, 3, dhf, init);
    //fill_ipv6_fib_lpm_table(ip, port, mac);
    //fill_ipv6_fib_lpm_table(ip2, port2, mac2);

    //fill_sendout_table(port, smac);
    //fill_sendout_table(port2, smac);

    printf("Launching controller's main loop...\n");
    execute_controller(c);

    printf("Destroy controller\n");
    destroy_controller(c);

    return 0;
}


