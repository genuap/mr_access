
/*######################################################################

                                mr_access
                        memory and register viewer
                                     
 Purpose:
This Linux based utility is used to dump memory mapped registers values. 
Registers are categorized by IP blocks.  Th mmap is used to map between a 
processess /dev/mem. /dev/mem imem  is  a  character  device file that is an 
image of main memory of the computer.  Byte addresses in /dev/mem are 
interpreted as physical memory  addresses.   
 


######################################################################
 Version  Date       By               | Note                         
                 
---------------------------------------------------------------------
 0.1      12-28-10   Bill Anderson    | Created 
---------------------------------------------------------------------
 0.2      01-24-11   Bill Anderson    | -Change map length to equal page
                                      | size (no longer needs to be specified
                                      | in register input file
                                      | -added command line override for
                                      |  base address
---------------------------------------------------------------------
 0.3      01-27-11   Bill Anderson    | -added raw mem dump mode
                                      | -fixed missing block offset
                                      | -add bar check
---------------------------------------------------------------------
 0.4      06-06-11   Paul Genua       | -added writes via g_write
---------------------------------------------------------------------
 0.5      08-11-11   Bill Anderson    | - added help info for write mode
                                      | - removed debug code
---------------------------------------------------------------------
 1.0     11-15-11   Bill Anderson     | - added default rdf file name
                                      | - added 8-bit and 16-bit register writes
                                      | - added regiser write with confirm
                                      | - added memory write
                                      | - fixed register aliasing bug in write_register
---------------------------------------------------------------------
 1.1      09-20-12   Paul Genua       | -added 36/64bit address support for read  
---------------------------------------------------------------------
 1.2      09-24-12   Paul Genua       | -added 36/64bit address support for write
---------------------------------------------------------------------
 1.3      11-14-12   Bill Anderson    | -merged 32-bit and 36/64 bit address support
                                      |  with conditional compile 
                                      |- added loop option for memory read and writes 

---------------------------------------------------------------------
 2.0      10-30-15   Paul Genua       | -removed 32-bit support
				      | 				-armv8 support only

###################################################################### */




#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#define REV "2.0"

#define FALSE 0
#define TRUE  1
#define MAX_LINES 25

#define ALL_OFF (0x0)
#define ALL_ON  (0x7)

//the following definition is for 36/64 bit addres space 
typedef unsigned long long phys_addr_t;
#define STRTOUX  strtoull
#define ADDR_MASK 0xFFFFFFF000

#define DEBUG printf

//Virtual address that is associated with the physical address

static char g_db_path[128];
static char g_block[16];
static char g_ascii_array[20];

int g_devmem; // this is the "/dev/mem" descriptor
FILE *g_db_file;

/* Globals to indicate functions to be run after parsing command line */
volatile unsigned int g_run = FALSE;
volatile unsigned int g_list = FALSE;
volatile unsigned int g_list_block = FALSE;
volatile unsigned int g_list_registers = FALSE; 
volatile unsigned int g_dump_all = FALSE;
volatile unsigned int g_go = FALSE; 
volatile unsigned int g_bar_found = FALSE; 
volatile unsigned int g_write = FALSE;
volatile unsigned int g_confirm = FALSE;
volatile unsigned int g_bits = FALSE;

uint64_t g_map_base = 0;
//phys_addr_t g_map_base = 0;  /* changed for 64-bit */
//volatile unsigned int g_map_base = 0;
volatile unsigned int g_bar_override = FALSE; 
volatile unsigned int g_mem_dump = FALSE;
volatile unsigned int g_mem_write = FALSE;
volatile unsigned int g_count = 32; 
volatile unsigned int g_width = 32;
volatile unsigned int g_value = 0;
volatile unsigned int g_loop = 1;
volatile unsigned int g_line_count = 0;



int parse_command(int argc, char *argv[])     /* this routine reads in user input */

{

int i;
static char dummy[32];

/*parse command line */

 strcpy(g_db_path, "reg.rdf");

 for ( i=0; i < argc; i++)
 {

    /*register definition file */

    if ( !strcmp(argv[i],"-f") && (i  < (argc - 1)))
    {
        strcpy(g_db_path,argv[i+1]);
        g_run = TRUE;
    }

    /*base address override */
    else if ( !strcmp(argv[i],"-bar") )
    {
        g_map_base = STRTOUX(argv[i+1], NULL, 16); /*64-bit change*/
        g_bar_override = TRUE;
        g_bar_found = TRUE;
    }

  
    /*memory dump */
    else if ( !strcmp(argv[i],"-md.b") ||
            !strcmp(argv[i],"-md.w") ||
            !strcmp(argv[i],"-md.l")  )
    {

        strcpy(dummy,argv[i]);

        if ( i  < (argc - 1) ) {
            if ( argv[i+1][0] != '-') {
            g_map_base = STRTOUX(argv[i+1], NULL, 16); 
            } 
        }

        if ( i  < (argc -2) ) {
            if ( argv[i+2][0] != '-') {
            g_count = strtoul(argv[i+2], NULL, 16);
            }
        }

        g_mem_dump = TRUE;
        if (dummy[4] == 'b') {
            g_width = 8;
        } else if (dummy[4] == 'w') {
            g_width = 16;
        } else if (dummy[4] == 'l') {
            g_width = 32;
        } else {
            g_run = FALSE;
        }
        g_run = TRUE;

    }

    /*memory write */
    else if ( !strcmp(argv[i],"-mw.b") ||
            !strcmp(argv[i],"-mw.w") ||
            !strcmp(argv[i],"-mw.l") ||
            !strcmp(argv[i],"-mc.b") ||
            !strcmp(argv[i],"-mc.w") ||
            !strcmp(argv[i],"-mc.l") )
    {
        DEBUG("parse1\n");
        strcpy(dummy,argv[i]);

 
        g_map_base = STRTOUX(argv[i+1], NULL, 16);  /*64-bit change*/
   
        g_value = strtoul(argv[i+2], NULL, 16);
        DEBUG("parse2\n");
        if ( argv[i+3][0] != '-' ) {
            g_count = strtoul(argv[i+3], NULL, 16);
        }
    
        g_mem_write = TRUE;

        if (dummy[2] == 'c') {
            g_confirm = TRUE;
        }        
        if (dummy[4] == 'b') {
            g_width = 8;
        } else if (dummy[4] == 'w') {
            g_width = 16;
        } else if (dummy[4] == 'l') {
            g_width = 32;
        } else {
            g_run = FALSE;
        }
            exit(1);
    g_run = TRUE;
    //, g_count, g_width );
    }

    /*help */
    else if ( !strcmp(argv[i],"-h") )
    {
        g_run = FALSE;
    }

    /* go without page breaks */
    else if ( !strcmp(argv[i],"-g") )
    {
        g_go = TRUE;
        g_run = TRUE;
    }
  
  /* loop memory commands */
    else if ( !strcmp(argv[i],"-loop") )
    {
        g_loop = strtoul(argv[i+1], NULL, 10);
    }

    /*register block */
    else if ( !strcmp(argv[i],"-b") )
    {
        strcpy(g_block,argv[i+1]);
        g_run = TRUE;
    }

    /*write registers */
    else if ( !strcmp(argv[i],"-w") )
    {
        g_write = i+1;
        g_run = TRUE;
        }

    /*write registers with confirm */
    else if ( !strcmp(argv[i],"-wc") )
    {
        g_write = i+1;
        g_confirm = TRUE;
        g_run = TRUE;
    }

    /*list available ip blocks */
    else if ( !strcmp(argv[i],"-l") )
    {
        g_list = TRUE;
        g_list_block = TRUE;
        g_run = TRUE;
    }
    
    /*list  registers for specified ip block */
    else if ( !strcmp(argv[i],"-lr") )
    {
        strcpy(g_block,argv[i+1]);
        g_list = TRUE;
        g_list_registers = TRUE;
        g_run = TRUE;
    } 

    /*dump all registers */
    else if ( !strcmp(argv[i],"-a") )
    {
        g_dump_all = TRUE;
        g_run = TRUE;
    }   
 }  
 return(0);
}/*end of parse cmd */





int print_help_screen(char * rev) {
 
    printf("\n# mr_access -- memory and register access tool Rev. %s \n",rev);
    printf("\n# usage (read): mr_access -f <db_file> [-b block] [-l -a -h]  \n");
    printf("# usage (write): mr_access -f <db_file> -w <register name> <value>  \n#\n");
    printf("#         Register Acess:\n");
    printf("#         -f <rdf_file> -- path name to register definition file (required for register dump)\n");
    printf("#         -bar <map base address> -- overrides default base address\n");
    printf("#         -b <block> -- block speicifys the IP block register to dump\n");
    printf("#         -a -- dump all registers\n");
    printf("#         -w <register name> <value> -- write register  \n");
    printf("#         -gwc <register name> <value> -- write registers with read back confirmation  \n");
    printf("#         -l -- list all IP blocks available for dumping\n");
    printf("#         -lr <block>-- list all registers in specified IP block\n#\n");    

    printf("#         Memory Access:\n");
    printf("#         -loop <n> -- loop n times through md|mw|mc commands \n");
    printf("#         -md.[bwl] <address> <count> -- raw memory dump\n#\n");
    printf("#         -mw.[bwl] <address> <value> <count> -- raw memory write\n#\n");
    printf("#         -mc.[bwl] <address> <value> <count> -- raw memory write with confirm\n#\n");
    printf("#         General:\n");  
    printf("#         -h -- print this message\n");
    printf("#         -g -- dump registers without page breaks\n#\n");

    printf("# EXAMPLES:\n#\n");
    printf("#   Dump LAW registers:\n#\n");
    printf("#     >mr_access -f 8315.rdf -b LAW\n#\n");
    printf("#   Write a register:\n#\n");
    printf("#     >mr_access -f 8315.rdf -w TIMING_CFG2 deadbeef CS2_BNDS abcd1234\n#\n");
    printf("#   Dump all registers:\n#\n");
    printf("#     >mr_access -f 8315.rdf -a \n#\n");
    printf("#   List all IP Blocks:\n#\n");
    printf("#     >mr_access -f 8315.rdf -l \n#\n");
    printf("#   dump 16 32-bit aligned memory locations starting at address E0000000:\n#\n");
    printf("#     >mr_access -md.l E0000000 10 \n#\n");
    printf("#   write 12345678 to 32, 32-bit aligned memory addresses starting at E0000000 :\n#\n");
    printf("#     >mr_access -mw.l E0000000 12345678 20 \n#\n");
    return (0);
}/* end print_help_screen */



void open_files ()
{
       if ((g_db_file = fopen(g_db_path,"r")) == NULL)  /* bad file */
       {
        printf("ERROR: Can't open file %s\n",g_db_path);
        exit(1);                   
       }
} /*end of open files */


void check_for_bar() {
  if ( !g_bar_found) {

     printf ("ERROR: No Map Base Address defined,\n");
     printf ("Specify base address on the command line using -bar switch or\n");
     printf ("add MAP BASE parameter on line 1 of the register definition file \n");
     exit(1);
  } 
}


/*stop display output after LINE_MAX lines*/
void wait_MAX_LINES (int lines) {
     
static char tmp[1];

      g_line_count++;

      if (g_line_count > lines && !g_go) {           
        printf("Hit enter to continue > ");
        scanf("%c",&tmp);
        if(tmp[0] == 'g') {
         g_go = TRUE;
        }
        g_line_count = 0;
       }
}/* end of wait_MAX_LINES */


void capture_ascii( volatile char *mapped_addr, unsigned long offset, unsigned int index, unsigned int count) {

uint8_t                addr8;
unsigned int j;
unsigned int mod;

mod = 4; //32-bit width

           
     
     for (j = 0; j < (g_width/8); j++ ) {
	//addr8 = *(uint8_t *)mapped_addr;
        addr8 = *(uint8_t *)(mapped_addr + offset + index + j);
        //printf("    DEBUG:0x%08X\n",*(mapped_addr + offset + index + j));
        //printf("\nvalue %x = count = %d array_index 0x%08X\n",addr8,  count, (mapped_addr + offset + index + j) ); 
         if (addr8 > 31 && addr8 <= 127)
         {
             g_ascii_array[(count % mod) * (g_width/8) + j] = addr8;
         } else {
             g_ascii_array[(count % mod) * (g_width/8) + j] = '.';
         }
     }
}


int reg_write (int argc, char *argv[]) {

void *mapped_addr;

//volatile unsigned int   i;
volatile unsigned int   first_time = TRUE;
volatile unsigned int   found_register = FALSE;
//volatile unsigned int   name_length;
volatile unsigned int   addr_index=0;
volatile unsigned int   map_size;
//volatile unsigned int   index_mode = FALSE;
//volatile unsigned int   end_of_block = FALSE;
volatile unsigned long  addr;     
//volatile unsigned int   byte_count;


//static char             yes_or_no[2];
//static char             block_name[64];
static char             line[128];
static char             reg_name[64];
//static char             reg_namei[64];
//static char             dummy[32];
//static char             dummy1[32];
//static char             dummy2[32];
static char             permissions[2];

unsigned long		    new_value;
unsigned long           reg_offset;
unsigned long           block_offset = 0;
//unsigned int            reg_index;
unsigned int            reg_size = 32;
phys_addr_t             offset;     
phys_addr_t             page_aligned_address;
size_t page_size = sysconf (_SC_PAGESIZE);

int j;
    /*mmap base and length must be aligned to the page size */
       map_size = page_size;

 

	for(g_write;g_write<argc;g_write=g_write+2)
	{
        /* First, find the register in our rdf */
        fseek(g_db_file,0L,0);
        found_register = FALSE;
        while ( fgets(line, sizeof line ,g_db_file) != NULL)  { 

            sscanf(line,"%lx %d %s %s",&reg_offset, &reg_size,permissions,reg_name);
            /* armv8 we change reg size to bytes while mr_access expects bits */

            reg_size = reg_size * 8;

        
            if(!strncmp(line,"MAP",strlen("MAP")) && !g_bar_override && first_time ) {
                g_bar_found = TRUE;
                first_time = FALSE;

            } else if(!strcmp(reg_name,argv[g_write])) {
          
                found_register = TRUE;
                check_for_bar();
            
                /* read in the line */
                sscanf(argv[g_write+1], "%lx", &new_value);
                DEBUG("new value: %lx  %d\n",new_value,reg_size);

                page_aligned_address = (g_map_base + block_offset + reg_offset) & ADDR_MASK;  /*changed mask for 64-bit*/
                mapped_addr = mmap( NULL, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, g_devmem, page_aligned_address );
                if (mapped_addr == MAP_FAILED) {
                    printf("\nCould not map registers (%d) %s", errno,strerror(errno));
                    close(g_devmem);
                    return -1;
                }
                offset = g_map_base + reg_offset + block_offset + addr_index - page_aligned_address;
                if (reg_size == 8 ) {
                     *(uint8_t *)(mapped_addr + offset) = new_value;
                  
                      printf(">Wrote: 0x%08lX  to  %-20s (0x%0lX) \n",new_value, reg_name,g_map_base + reg_offset + block_offset + addr_index);

                      if (g_confirm) {
                            addr = *(uint8_t *)(mapped_addr + offset);
                   
                            printf(">Read:  0x%02lX from %-20s (0x%lX) \n", addr, reg_name,g_map_base + reg_offset + block_offset + addr_index);                      

                      }
                } else if (reg_size == 16) { 
                    *(uint16_t *)(mapped_addr + offset) = new_value;
                 
                     printf(">Wrote:0x%08lX  to   %-20s (0x%lX) \n", new_value,reg_name,g_map_base + reg_offset + block_offset + addr_index);

                     if (g_confirm) {
                        addr = *(uint16_t *)(mapped_addr + offset);                    
                 
                        printf(">Read:  0x%04lX from %-20s to (0x%lX) \n", addr,reg_name,g_map_base + reg_offset + block_offset + addr_index);

                     }
                } else if (reg_size == 32) { 
                    *(uint32_t *)(mapped_addr + offset) = new_value;
                 
                     printf(">Wrote: 0x%08lX  to  %-20s (0x%lX)\n", new_value,reg_name,g_map_base + reg_offset + block_offset + addr_index);              

                     if (g_confirm) {
                        addr = *(uint32_t *)(mapped_addr + offset);
                  
                        printf(">Read:  0x%08X from %-20s (0x%llX) \n", addr, reg_name,g_map_base + reg_offset + block_offset + addr_index);

                     }
	
	          } else if (reg_size == 64) { 
        	     *(uint64_t *)(mapped_addr + offset) = new_value;
                  
                     printf(">Wrote: 0x%08lX  to  %-20s (0x%lX)\n", new_value,reg_name,g_map_base + reg_offset + block_offset + addr_index);

                     if (g_confirm) {	         	          	          
                         addr = *(uint64_t *)(mapped_addr + offset);
                
                         printf(">Read:  0x%08lX from %-20s (0x%lX) \n", addr, reg_name,g_map_base + reg_offset + block_offset + addr_index);

                     }
                  }

             //Cleanup
             munmap( (void *)page_aligned_address, map_size);
          }/*end of output */

        } /*end of while loop */
          if (!found_register) {
            printf("ERROR:Register %s not found in register definition file %s\n", argv[g_write],g_db_path);

          }
       } /* end of for loop */
    return (0);
}/* end of reg_write */

int reg_dump () {

void *mapped_addr;

volatile unsigned int   i;
volatile unsigned int   first_time = TRUE;
volatile unsigned int   addr_index;
volatile unsigned int   map_size;
volatile unsigned int   index_mode = FALSE;
volatile unsigned int   end_of_block = FALSE;
volatile unsigned long  addr;     

size_t page_size = sysconf (_SC_PAGESIZE);

static char             yes_or_no[2];
static char             block_name[64];
static char             line[128];
static char             reg_name[64];
static char             reg_namei[64];
static char             dummy[32];
static char             dummy1[32];
static char             bits[6];
static char             description[32];
static char             permissions[2];

unsigned long           reg_offset;
unsigned long           block_offset = 0;
unsigned int            reg_index=0;
unsigned int            reg_size = 32;
unsigned long           offset;     
phys_addr_t             page_aligned_address;

// Variables for bitfields
unsigned int            bit1, bit2;
char *token;
const char s[2]=":";
static int          bitfield[32];
unsigned int        hex;

int j;

     /*mmap base and length must be aligned to the page size */
       map_size = page_size;

       /* read in the rdf file */
        fseek(g_db_file,0L,0);
       
        while ( fgets(line, sizeof line ,g_db_file) != NULL)  { 
        //DEBUG ("line !%s!\n",line); 
        
        /* Check first to see if it's a MAP line */
         if(!strncmp(line,"MAP",strlen("MAP")) && !g_bar_override ) {
            //DEBUG (">>>>>>>Found MAP\n");
            g_bar_found = TRUE;
            //DEBUG (">>>>>>>Doing Read\n");

            /* read in the base address of the CCSRBAR */
            sscanf(line,"%s %s %lX",dummy,dummy1,&g_map_base); /*change for 64_bit*/
            //DEBUG (">>>>>>>Did Read\n");

            /* check if it's a BASE */
            if (!strncmp(dummy1,"BASE_DEFAULT",strlen("BASE_DEFAULT"))) {
              printf ("Warning: Using the default Map Base Address, 0x%lX,",g_map_base); /*change for 64_bit*/
              printf ("specified in the Register Definition File\n");
              printf ("Note:This prompt can be eliminated by specifing\n");
              printf ("the Map Base address on the command line or\n");
              printf ("by changinng MAP BASE_DEFAULT in the Register\n");
              printf ("Definition File to MAP BASE\n");
              printf ("Do you want to keep the default ? (Y or N)>");
              scanf("%s",yes_or_no);
              if ( !strcmp(yes_or_no,"Y")) {
               //continue 
              } else {
                exit(1);
              }
            }

           /* check if it's a block line */
         } else if(!strncmp(line,"BLOCK",strlen("BLOCK"))) {
           
                sscanf(line,"%s %s %s",dummy,dummy1,block_name);
                end_of_block = FALSE;
                //DEBUG("### found block: %s, looking for: %s\n",block_name,g_block);
                
          } else if(!strncmp(line,"BLOCK END",strlen("BLOCK END"))) {

             end_of_block = TRUE; 
             index_mode = FALSE;
             first_time = TRUE;
             block_offset = 0;
             
          } else if(!strncmp(line,"      BITS",strlen("      BITS"))) {
                sscanf(line,"%s %s %s",dummy,bits,description);
                /* assuming that bits can be a single decimal number or range seperated by colon */
                token=strtok(bits,s);
                bit1=atoi(token);
                if(token !=NULL)
                {
                    token=strtok(NULL,s);
                    bit2=atoi(token);
                } else {
                    bit2=bit1;
                }
                
                for (i=0;i<32;i++)
                {
                    // assuming the value is in an int called hex
                    bitfield[i]=(hex>>i)&1;
                }
               
          
          //} else if((!isalpha(line[0]) && !strcmp(block_name,g_block)  && !end_of_block) || 
         //        (!isalpha(line[0]) && g_dump_all)) {
         } else if ((!strcmp(block_name,g_block)  && (!end_of_block)) || (g_dump_all)) {         

              check_for_bar();
              //DEBUG("checked the BAR\n");
              sscanf(line,"%lx %d %s %s",&reg_offset, &reg_size,permissions,reg_name);
              //DEBUG ("reg name: %s\n",reg_name);
              page_aligned_address = (g_map_base + block_offset + reg_offset) & ADDR_MASK; 

              /* ARMv8 lists register size as bytes
               * multiply by 8 to get bits */
              reg_size = reg_size*8;
 
 
              mapped_addr = mmap( NULL, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, g_devmem, page_aligned_address ); 
              if (mapped_addr == MAP_FAILED) {
                printf("\nCould not map registers (%d) %s", errno,strerror(errno));
                close(g_devmem);
                return -1;
              }
 
            //DEBUG("Performed Memory MAP\n");

              if (first_time) {
                // Print out the block name heading followed by the values
                printf("\n#%s Registers:\n\n",block_name);
                first_time = FALSE;
              }
          
              if (index_mode ) {
                for (i = 0; i <= reg_index-1; i++ ) {
                /* calculate offset from start of mapped address  */
                  offset = g_map_base + reg_offset + block_offset + addr_index - page_aligned_address;
                     
                  
                  sprintf(dummy,"%d",i);
                  strcpy(reg_namei,reg_name);
                  strcat(reg_namei,dummy);
                  if (reg_size == 8 ) {
                     addr = *(uint8_t *)(mapped_addr + offset);
                                      
                     printf(" %-20s 0x%lX = 0x%02lX\n", reg_namei,g_map_base + reg_offset + block_offset + addr_index,addr); /*change for 64-bit */
                     hex=addr;
                            
                     addr_index++;
                  } else if (reg_size == 16) { 
                     addr = *(uint16_t *)(mapped_addr + offset);
                                                           
                     printf(" %-20s 0x%lX = 0x%04lX\n", reg_namei,g_map_base + reg_offset + block_offset + addr_index,addr); /*change for 64-bit */
                     hex=addr;
                     addr_index = addr_index + 2;                                         
                  } else if (reg_size == 32) { 
                     addr = *(uint32_t *)(mapped_addr + offset);
                                     
                     printf(" %-20s 0x%lX = 0x%08lX\n", reg_namei,g_map_base + reg_offset + block_offset + addr_index,addr); /*change for 64-bit */
                     hex=addr;
                  
                     addr_index = addr_index + 4;                                           
                  } else if (reg_size == 64) { 
                     addr = *(uint64_t *)(mapped_addr + offset);
                                      
                     printf(" %-20s 0x%lX = 0x%016lX\n", reg_namei,g_map_base + reg_offset + block_offset + addr_index,addr); /*change for 64-bit */
                     hex=addr;

                     addr_index = addr_index + 8; 
                                         
                  }
                  wait_MAX_LINES(MAX_LINES);
                } /* end for loop */
                index_mode = FALSE;

              } else {
                 /* calculate offset from start of mapped address  */
                 offset = (g_map_base + reg_offset + block_offset) - page_aligned_address;          
      
                 if (reg_size == 8) {
                    addr = *(uint8_t *)(mapped_addr + offset);
                                        
                    printf(" %-20s 0x%lX = 0x%02lX\n", reg_name,g_map_base + block_offset + reg_offset,addr); /*change for 64-bit */
                    hex=addr;

                 } else if(reg_size == 16) { 
                    addr = *(uint16_t *)(mapped_addr + offset);
                                      
                    printf(" %-20s 0x%lX = 0x%04lX\n", reg_name,g_map_base + block_offset + reg_offset,addr); /*change for 64-bit */
                    hex=addr;

                 } else if(reg_size == 32) { 
                    addr = *(uint32_t *)(mapped_addr + offset); 
                                       
                    printf(" %-20s 0x%lX = 0x%08lX\n", reg_name,g_map_base + block_offset + reg_offset,addr); /*change for 64-bit */
                    hex=addr;

                 } else if(reg_size == 64) { 
                    addr = *(uint64_t *)(mapped_addr + offset); 
                                       
                    printf(" %-20s 0x%lX = 0x%016lX\n", reg_name,g_map_base + block_offset + reg_offset,addr);/*change for 64-bit */
                    hex=addr;

                 }
               }

               /* if MAX_LINES printed wait for CR */
               wait_MAX_LINES(MAX_LINES);
         //Cleanup
          munmap( (void *)page_aligned_address, map_size);

          }/*end of output */

        } /*end of while loop */
    return(0);
}/* end of reg_dump */



/* list all blocks available in rdf file */
int list () {
static char             line[128];
static char             dummy[32];
static char             dummy1[32];
static char             block_name[64];
volatile unsigned int   end_of_block = FALSE;
volatile unsigned int   first_time = TRUE;

        if ( g_list_block ) {
          printf ("# Blocks in register definition file -- \n");
          printf ("#  Use the -b option with the block name \n");
          printf ("#  to dump the registers associate with that block \n");
          printf ("# Blocks:\n");
        } else  if ( g_list_registers ) {
          printf ("# Registers associated with \n#%s: \n",g_block);
        }

        while ( fgets(line, sizeof line ,g_db_file) != NULL)  {
          sscanf(line,"%s %s",dummy,dummy1);
          if(!strncmp(line,"BLOCK NAME",strlen("BLOCK NAME"))) {
           
            sscanf(line,"%s %s %s",dummy,dummy1,block_name);
            end_of_block = FALSE;
            if ( g_list_block  ) {
              first_time = FALSE;
              printf("#    %s\n",block_name );
            }
          } else if(isalpha(line[0]) && \
                    !strcmp(block_name,g_block) && \
                     strcmp(dummy,"REG") && \
 		     strcmp(dummy,"BLOCK") && \
                    !end_of_block && g_list_registers) {
              
              printf("#    %-25s  %-s\n",dummy,dummy1 );
          } else if(!strncmp(line,"BLOCK END",strlen("BLOCK END"))) {
              end_of_block = TRUE;
          }
        }
    return (0);
}


int memory_dump (phys_addr_t map_base,  unsigned int width, unsigned int count) {


void          	        *mapped_addr;
volatile char            ascii_array[20];

phys_addr_t              page_aligned_address; /*change for 64-bit */
phys_addr_t              offset;                /*change for 64-bit */


size_t page_size = sysconf (_SC_PAGESIZE);
volatile unsigned int  addr_index;
volatile unsigned int  addr;    /*changed from long to int???? for 64-bit*/
volatile unsigned int  byte_count;
volatile unsigned int  i;
volatile unsigned int  j;
volatile unsigned int  map_size;


  page_aligned_address = g_map_base & ~(page_size -1);
  offset = g_map_base - page_aligned_address;
  byte_count = g_count * g_width/8;
  map_size = ( offset + byte_count + page_size) & ~(page_size -1);
 
  mapped_addr = mmap( NULL, map_size, PROT_READ, MAP_SHARED, g_devmem, page_aligned_address );

  /* test to see if the mapping worked */
   if (mapped_addr == MAP_FAILED) {
      printf("\nCould not map memory (%d) %s", errno,strerror(errno));
      close(g_devmem);
      return -1;
   }
   
   addr_index = 0;
   for ( i = 0; i < g_count; i++ ) {
    /* g_count is global from the command line (e.g. number to dump) */
      if (g_width == 8) {

          addr = *(uint8_t *)(mapped_addr + offset + addr_index);

          if ( !(i % 16) ) {
             capture_ascii( mapped_addr,offset,addr_index,i);  /*  save values for ascii output */
             printf("\n");
             wait_MAX_LINES(MAX_LINES);

             printf("%10llX: %02X",page_aligned_address + offset + addr_index,addr);/*change for 64-bit */
          
         } else if (i % 16 < 15) {

            capture_ascii( mapped_addr,offset,addr_index,i);
            printf(" %02X",addr);
         } else {
            capture_ascii( mapped_addr,offset,addr_index,i);
            printf(" %02X",addr);
            printf("    %s",g_ascii_array); 
          }
          addr_index++;

/* Word */
       } else if(g_width == 16) { 
          addr = *(uint16_t *)(mapped_addr + offset + addr_index);
         if ( !(i % 8) ) {
              capture_ascii( mapped_addr,offset,addr_index,i);
              printf("\n");
              wait_MAX_LINES(MAX_LINES);
            
              printf("%10llX: %04X",page_aligned_address + offset + addr_index,addr);

           } else if (i % 8 < 7) {
               capture_ascii( mapped_addr,offset,addr_index,i);
               printf(" %04X",addr);

           } else {
              capture_ascii( mapped_addr,offset,addr_index,i);
              printf(" %04X",addr);
              printf("    %s",g_ascii_array);
           }
           addr_index = addr_index + 2;
        

/* Long */
       } else if(g_width == 32) { 
          addr = *(uint32_t *)(mapped_addr + offset + addr_index);
           if ( !(i % 4) ) {
              capture_ascii( mapped_addr,offset,addr_index,i);
              printf("\n");
              wait_MAX_LINES(MAX_LINES); 

              printf("%10llX: %08X",page_aligned_address + offset + addr_index,addr); /*change for 64-bit */
      
           } else if (i % 4 < 3) {
             capture_ascii( mapped_addr,offset,addr_index,i);
             printf(" %08X",addr);
           } else {
             capture_ascii( mapped_addr,offset,addr_index,i);
             printf(" %08X",addr);
             printf("    %s",g_ascii_array);
           }
           addr_index = addr_index + 4;
       
       }
   }/* end of for loop */
      
   printf("\n");
   munmap((void *)page_aligned_address, map_size);
   return (0);
}/*end of memory_dump */


int memory_write (phys_addr_t map_base,unsigned int value, unsigned int width, unsigned int count) {


void                    *mapped_addr;
volatile char            ascii_array[20];

phys_addr_t              page_aligned_address; /*change for 64-bit */
phys_addr_t              offset; /*change for 64-bit */
size_t page_size = sysconf (_SC_PAGESIZE);

volatile unsigned int  addr_index;
volatile unsigned long addr; 
volatile unsigned int  byte_count;
volatile unsigned int  i;
volatile unsigned int  j;
volatile unsigned int  map_size;



  page_aligned_address = map_base & ~(page_size -1);
  offset = g_map_base - page_aligned_address;
  byte_count = count * width/8;
  map_size = ( offset + byte_count + page_size) & ~(page_size -1);
  DEBUG(">>>>>>>>inside write\n");
   /* glibc has mmap and mmap64 mapped the same */
   mapped_addr = mmap( NULL, map_size, PROT_READ|PROT_WRITE, MAP_SHARED, g_devmem, page_aligned_address ); 

   if (mapped_addr == MAP_FAILED) {
      printf("\nCould not map memory (%d) %s", errno,strerror(errno));
      close(g_devmem);
      return -1;
   }
   addr_index = 0;
exit(1);
//write loop
   for ( i = 0; i < count; i++ ) {
        
       if (width == 8) {


          *(uint8_t *)(mapped_addr + offset + addr_index) = value;
         
           printf(">Wrote: 0x%02X  to  0x%llX\n", value, page_aligned_address + offset + addr_index); /*change for 64-bit */

          addr_index++;
/* Word */
       } else if(width == 16) { 
 
          *(uint16_t *)(mapped_addr + offset + addr_index) = value;
        
           printf(">Wrote: 0x%04X  to  0x%llX\n", value, page_aligned_address + offset + addr_index); /*change for 64-bit */

           addr_index = addr_index + 2;
     

/* Long */
       } else if(width == 32) { 

          *(uint32_t *)(mapped_addr + offset + addr_index) = value;
        
           printf(">Wrote: 0x%08X  to  0x%llX\n", value, page_aligned_address + offset + addr_index); /*change for 64-bit */

           addr_index = addr_index + 4;
        }
      
   }/* end of write loop */

//read loop
   addr_index = 0;
   if (g_confirm ) {
     printf("\n\nRead Back:\n");

     memory_dump(map_base,width, count); 

   }


   printf("\n");
   munmap((void *)page_aligned_address, map_size);
   return (0);
}/*end of memory_write */



 

int main(int argc, char *argv[])
{

volatile unsigned int  i;


 /*parse command line arguements */       
      
   parse_command(argc,argv);
       

   if (g_run) {
 
       g_devmem = open("/dev/mem", O_RDWR | O_SYNC);      
      
       if (g_devmem < 0) {
          printf("\nOpening of /dev/mem failed with (%d) %s\n",errno,strerror(errno));
          return -1;
       }
      
       if (g_write ) {  /* register dump */
       
            open_files();
            reg_write(argc,argv);
       
       } else if (g_mem_dump) { /* raw memory dump */
            for ( i = 0; i < g_loop; i++ ) {
                memory_dump(g_map_base,g_width,g_count);
            }
       } else if (g_mem_write) { /* raw memory write */
       
            for ( i = 0; i < g_loop; i++ ) {
                memory_write(g_map_base,g_value,g_width,g_count);
            }
       } else if (g_list) { /*list IP blocks*/
            open_files();
            list();
       } else { /*open register database file for reading */
 
            open_files();
            reg_dump();   
       }

        //Cleanup
        
        close(g_devmem);
        return 0;

   } else { /* end of g_run */
   
        print_help_screen(REV);

  }
   
    return(0);
} /* end of main */
