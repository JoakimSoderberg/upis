/*
  Name:     upis.c
  Author:   Graham Single
  Revision: 5.0.2
  Date:     02-Nov-14
  Purpose:  Reports UPiS PICo interface values in Raspberry Pi command line
            and controls UPiS from Raspberry Pi command line
*/

#include <linux/i2c-dev.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <argp.h>

/* Prototypes */
void strlower(char *string);
int readi2cbyte(unsigned int i2cbus, unsigned int i2caddr, unsigned int i2creg);
int readi2cword(unsigned int i2cbus, unsigned int i2caddr, unsigned int i2creg);
void writei2cbyte(unsigned int i2cbus, unsigned int i2caddr, unsigned int i2creg, unsigned int i2cval);
int bcdbyte2dec(unsigned int bcd);
int bcdword2dec(unsigned int bcd);

/* ARGP Setup */
const char *argp_program_version = "upis 5.0.2";
const char *argp_program_bug_address = "<graham@the-singles.co.uk>";
/* ARGP Argument and Parameters */  
struct arguments {
  int rtc;      /* The -R   & --rtc flag */
  int rtcfactor;   /* The -F   & --rtcfactor */ 
  int pwrsrc;        /* The -s   & --pwrsrc flag */
  int batvolt;      /* The -b   & --batvolt flag */
  int rpivolt;      /* The -p   & --rpivolt flag */
  int eprvolt;      /* The -e   & --eprvolt flag */
  int usbvolt;      /* The -u   & --usbvolt flag */
  int current;      /* The -a   & --current flag */
  int centigrade;   /* The -c   & --centigrade flag */
  int fahrenheit;   /* The -f   & --fahrenheit flag */
  int fwver;      /* The -Q   & --fwver flag */
  int factory;      /* The -Z   & --factory flag */     
  int reset;      /* The -z   & --reset flag */
  int bootloader;   /* The -l   & --bootloader flag */
  int errorno;      /* The -E    & --errorno flag */
  int watchdog;      /* The -w    & --watchdog flag */
  int fssd;         /* The -S    & --fssd flag */
  int fssdtimeout;   /* The -t    & --fssdtimeout flag */
  int fssdtype;      /* The -T   & --fssdtype flag */
  int fssdbatime;   /* The -B    & --fssdbatime flag */
  int starttimer;   /* The -o    & --starttimer flag */
  int stoptimer;   /* The -O   & --stoptimer flag */
  int lprtimer;      /* The -L   & --lprtimer flag */
  int relay;      /* The -r    & --relay flag */
  int eprlowv;      /* The -h   & --eprlowv flag */ 
  int minlprtime;   /* The -m   & --minlprtime flag */
  int lprcurrent;   /* The -I   & --lprcurrent flag */
  int iomode;      /* The -i   & --iomode flag */
  int iovalue;      /* The -V   & --iovalue flag */
  int yes;         /* The -y   & --yes flag */
  int verbose;      /* The -v   & --verbose flag */
  char *RTCF;      /* Argument for -F */
  char *WDTIM;      /* Argument for -w */
  char *FSSDTIM;   /* Argument for -t */
  char *FSSDACT;   /* Argument for -T */
  char *BATTIM;      /* Argument for -B */
  char *ONTIM;      /* Argument for -o */
  char *OFFTIM;      /* Argument for -O */
  char *LPRTIM;      /* Argument for -L */
  char *RLYSTAT;   /* Argument for -r */     
  char *EPRLOWV;   /* Argument for -h */
  char *MINLPRTIM;   /* Argument for -m */
  char *LPRAMP;      /* Argument for -I */        
  char *IOMODE;      /* Argument for -i */
};
/* OPTIONS.  Field 1 in ARGP. Order of fields: {NAME, KEY, ARG, FLAGS, DOC}. */
static struct argp_option options[] =
{
  {"rtc",'R',0,0,"Display time from the UPiS RTC in DD-MM-YYY HH:MM:SS (DOW) format"},
  {"rtcfactor",'F',"RTCF",OPTION_ARG_OPTIONAL,"Display, or set, the Real Time Clock correction factor. Valid values are between 0 and 255. Changes the RTC timer in multiples of 1 tick per second where a timer tick is 1/32768 HZ or 0.000030517578125 Seconds. Use 0 or 128 to let the clock run at its normal rate. Values between 1 and 127 will deduct the number of ticks specified per second and make the clock run progressively slower. Values between 129 and 255 will make the clock run progressive faster, where the number of ticks added will the specified value minus 128. In a 24 hour period adding or subtractng one tick changes the RTC by 86400 * 0.000030517578125 = 2.63671875 Seconds"},
  {"pwrsrc",'s',0,0,"Display the current UPiS power source:\n1=EPR,2=USB,3=RPI,4=BAT,5=LPR,6=CPR and 7=BPR\nWhen combined with -v displays power source name rather than number"},
  {"batvolt",'b',0,0,"Display the current UPiS battery voltage in Volts"},
  {"rpivolt",'p',0,0,"Display the voltage from the Raspberry Pi over the GPIO header in Volts"},
  {"eprvolt",'e',0,0,"Display the voltage at the UPiS EPR connector in Volts"},
  {"usbvolt",'u',0,0,"Display the voltage at the UPiS USB connector in Volts"},
  {"current",'a',0,0,"Display the mean current supplying both the UPiS and Raspberry Pi in mA"},
  {"centigrade",'c',0,0,"Display the UPiS temperature in Centigrade"},
  {"fahrenheit",'f',0,0,"Display the UPiS temperature in Fahrenheit"},
  {"fwver",'Q',0,0,"Display the UPiS firmware version number"},
  {"factory",'Z',0,0,"Perform a factory reset of the UPiS. Requires confirmation if not used with -y argument"},
  {"reset",'z',0,0,"Reset the UPiS CPU, apply startup values and reset RTC to 01/01/2012"},
  {"bootloader",'l',0,0,"Place the UPiS in bootloader mode (Red LED will flash). Requires confirmation if not used with -y argument"},
  {"errorno",'E',0,0,"Display the last UPiS error code, where 0 equals no error"},
  {"watchdog",'w',"WDTIM",OPTION_ARG_OPTIONAL,"Display or set the UPiS watchdog countdown timer in seconds. Setting the timer to 255 will disable it. When the timer reaches 0 seconds file safe shutdown will be triggered"},
  {"fssd",'S',0,0,"Trigger a file safe shutdown"}, 
  {"fssdtimeout",'t',"FSSDTIM",OPTION_ARG_OPTIONAL,"Display or set the file safe shutdown power off timer. This is the amount of time the UPiS will wait after initiating file safe shutdown, before power is removed from the Raspberry Pi"},
  {"fssdtype",'T',"FSSDACT",OPTION_ARG_OPTIONAL,"Display, or set, the UPiS action to be taken upon File Safe Shutdown, 0 will cut power and 1 will leave the Raspberry Pi powered on"},
  {"fssdbatime",'B',"BATTIM",OPTION_ARG_OPTIONAL,"Display, or set, a timer in seconds that will unconditionally cause file safe shutdown in battery mode when it reaches 0. Set to 255 to disable the timer"}, 
  {"starttimer",'o',"ONTIM",OPTION_ARG_OPTIONAL,"Display, or set, a timer that will cause the UPiS to wake up from LPR mode after it has been asleep for the sepcified number of seconds"},
  {"stoptimer",'O',"OFFTIM",OPTION_ARG_OPTIONAL,"Display, or set, a timer that will cause the UPiS to initiate file safe shutdown after it has been awake (out of LPR mode) for the specified number of seconds"},
  {"lprtimer",'L',"LPRTIM",OPTION_ARG_OPTIONAL,"Display, or set, the interval at which the UPiS will check for the presence of power and wakeup while in LPR mode"}, 
  {"relay",'r',"RLYSTAT",OPTION_ARG_OPTIONAL,"Display, or set, the relay state. Permissable value are: 1, 0, on, off, open or closed"},
  {"eprlowv",'h',"EPRLOWV",OPTION_ARG_OPTIONAL,"Display, or set, the EPR supply voltage below which the UPiS will switch to battery mode"},
  {"minlprtime",'m',"MINLPRTIM",OPTION_ARG_OPTIONAL,"Display, or set, the minimum interval that the UPiS will run in battery mode before resuming EPR power. This can be used to prevent the UPiS toggling between BAT and EPR power unecessarly when the ERP supply is unstable, such as solar power"},
  {"lprcurrent",'I',"LPRAMP",OPTION_ARG_OPTIONAL,"Display, or set, the current in miliamps drawn by the Raspberry Pi below which the UPiS to switch to LPR mode. Tune this value so that the UPiS correctly switches to LPR mode once the Raspberry Pi is shutdown. The exact current depands on what boards are attached to the Raspberry Pi and USB peripherals"},
  {"iomode",'i',"IOMODE",OPTION_ARG_OPTIONAL,"Display, or set, the mode of the 1 wire io pin of the UPiS.\n0=none\n1=1 wire temp value\n2=8 bit A to D convertor value\n3= Status of forced On-Change (Advanced Only)"}, 
  {"iovalue",'V',0,0,"Display the value read from the 1 wire IO pin based on the mode set by -i"},
  {"yes",'y',0,0,"Perform the reset -z, factory default -Z or bootloader -l options without prompting for confirmation"},
  {"verbose",'v',0,0,"Be verbose. Values will be suffixed by units and power modes are described by their name rather than mode number"},
  {0}
};
/* PARSER. Field 2 in ARGP. Order of parameters: KEY, ARG, STATE. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct arguments *arguments = state->input;
  switch (key) {
    case 'R': arguments->rtc=1; break;
    case 'F': arguments->rtcfactor=1; arguments->RTCF=arg; break;
    case 's': arguments->pwrsrc=1; break;
    case 'b': arguments->batvolt=1; break;
    case 'p': arguments->rpivolt=1; break;
    case 'e': arguments->eprvolt=1; break;
    case 'u': arguments->usbvolt=1; break;
    case 'a': arguments->current=1; break;
    case 'c': arguments->centigrade=1; break;
    case 'f': arguments->fahrenheit=1; break;
    case 'Q': arguments->fwver=1; break;
    case 'Z': arguments->factory=1; break;
    case 'z': arguments->reset=1; break;
    case 'l': arguments->bootloader=1; break;
    case 'E': arguments->errorno=1; break;
    case 'w': arguments->watchdog=1; arguments->WDTIM=arg; break;
    case 'S': arguments->fssd=1; break;
    case 't': arguments->fssdtimeout=1; arguments->FSSDTIM=arg; break;
    case 'T': arguments->fssdtype=1; arguments->FSSDACT=arg; break;
    case 'B': arguments->fssdbatime=1; arguments->BATTIM=arg; break;
    case 'o': arguments->starttimer=1; arguments->ONTIM=arg; break;
    case 'O': arguments->stoptimer=1; arguments->OFFTIM=arg; break;
    case 'L': arguments->lprtimer=1; arguments->LPRTIM=arg; break;
    case 'r': arguments->relay=1; arguments->RLYSTAT=arg; break;
    case 'h': arguments->eprlowv=1; arguments->EPRLOWV=arg; break;
    case 'm': arguments->minlprtime=1; arguments->MINLPRTIM=arg; break;
    case 'I': arguments->lprcurrent=1; arguments->LPRAMP=arg; break;
    case 'i': arguments->iomode=1; arguments->IOMODE=arg; break;
    case 'V': arguments->iovalue=1; break;
    case 'y': arguments->yes=1; break;
    case 'v': arguments->verbose=1; break;
    default: return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
/* ARGS_DOC. Field 3 in ARGP. A description of the non-option command-line arguments that we accept.  */
static char args_doc[] = "";
/* DOC. Field 4 in ARGP. Program documentation. */
static char doc[] = "A program to control the pimodules (www.pimodules.com) Raspberry Pi UPiS power supply via its PiCo (I2C) inteface.";
/* The ARGP structure itself. */
static struct argp argp = {options, parse_opt, args_doc, doc};

/* Main */
int main(int argc, char *argv[]) {

  struct arguments arguments;
  char strresp[BUFSIZ]; 

  /* Set ARGP Argument Defaults */
  arguments.rtc=0;
  arguments.rtcfactor=0; arguments.RTCF="";
  arguments.pwrsrc=0;
  arguments.batvolt=0;
  arguments.rpivolt=0;
  arguments.eprvolt=0;
  arguments.usbvolt=0;
  arguments.current=0;
  arguments.centigrade=0;
  arguments.fahrenheit=0;
  arguments.fwver=0;
  arguments.factory=0;
  arguments.reset=0;
  arguments.bootloader=0;
  arguments.errorno=0;
  arguments.watchdog=0; arguments.WDTIM="";
  arguments.fssd=0;
  arguments.fssdtimeout=0; arguments.FSSDTIM="";
  arguments.fssdtype=0; arguments.FSSDACT="";
  arguments.fssdbatime=0; arguments.BATTIM="";
  arguments.starttimer=0; arguments.ONTIM="";
  arguments.stoptimer=0; arguments.OFFTIM="";
  arguments.lprtimer=0; arguments.LPRTIM="";
  arguments.relay=0; arguments.RLYSTAT="";
  arguments.eprlowv=0; arguments.EPRLOWV="";
  arguments.minlprtime=0; arguments.MINLPRTIM="";
  arguments.lprcurrent=0; arguments.LPRAMP="";
  arguments.iomode=0; arguments.IOMODE="";
  arguments.iovalue=0;
  arguments.yes=0;
  arguments.verbose=0;
  
  /* Setup ARGP */
  argp_parse (&argp, argc, argv, 0, 0, &arguments);

  /* Count the number of arguments specified */
  int arg_count = arguments.rtc +
              arguments.rtcfactor + 
              arguments.pwrsrc +
              arguments.batvolt +
              arguments.rpivolt +
              arguments.eprvolt +
              arguments.usbvolt +
              arguments.current +
              arguments.centigrade +
              arguments.fahrenheit +
              arguments.fwver +
              arguments.factory +
              arguments.reset +
              arguments.bootloader +
              arguments.errorno +
              arguments.watchdog +
              arguments.fssd +
              arguments.fssdtimeout +
              arguments.fssdtype +
              arguments.fssdbatime +
              arguments.starttimer +
              arguments.stoptimer +
              arguments.lprtimer +
              arguments.relay +
              arguments.eprlowv +
              arguments.minlprtime +
              arguments.lprcurrent +
              arguments.iomode +
              arguments.iovalue;
  
  /* Display the time from the RTC */
  if (arguments.rtc) {
    if (arg_count > 1) {
      printf("RTC Date/Time: ");
    }
    printf("%02d-",bcdbyte2dec(readi2cbyte(0x01,0x69,0x04)));
    printf("%02d-",bcdbyte2dec(readi2cbyte(0x01,0x69,0x05)));
    printf("20%02d ",bcdbyte2dec(readi2cbyte(0x01,0x69,0x06)));
    printf("%02d:",bcdbyte2dec(readi2cbyte(0x01,0x69,0x02)));
    printf("%02d:",bcdbyte2dec(readi2cbyte(0x01,0x69,0x01)));
    printf("%02d ",bcdbyte2dec(readi2cbyte(0x01,0x69,0x00)));
    switch (bcdbyte2dec(readi2cbyte(0x01,0x69,0x03)))
    {
      case 1: printf("(Sunday)\n"); break;
      case 2: printf("(Monday)\n"); break;
      case 3: printf("(Tuesday)\n"); break;
      case 4: printf("(Wednesday)\n"); break;
      case 5: printf("(Thursday)\n"); break;
      case 6: printf("(Friday)\n"); break;
      case 7: printf("(Saturday)\n"); break;
    }
  }

  /* Display/Set RTC correction Factor */
  if (arguments.rtcfactor) {
    if (!arguments.RTCF) {
      if (arg_count > 1) {
        printf("RTC Correction Factor: ");
      }
      printf("%i",readi2cbyte(0x01,0x69,0x07));
      if (arguments.verbose)
      {
        printf(" (0x%02x)",readi2cbyte(0x01,0x69,0x07));
      }
      printf("\n");
    } else {
      if (is_intstr(arguments.RTCF) && atoi(arguments.RTCF) >= 0 && atoi(arguments.RTCF) <= 255)
      {
        writei2cbyte(0x01,0x69,0x07,atoi(arguments.RTCF));
        printf("RTC Correction Factor set to: %i (0x%02x)\n",readi2cbyte(0x01,0x69,0x07),readi2cbyte(0x01,0x69,0x07));
      } else {
        printf("Invalid argument '%s' for RTC clock factor - use an integer between 0 and 255\n",arguments.RTCF);
      }
    }
  }

  /* Display Power Source */ 
  if (arguments.pwrsrc) {
    if (arguments.verbose) {
      if (arg_count > 1) {
        printf("Power source: ");
      }
      int i2cresult = readi2cbyte(0x01,0x6A,0x00);
      if (i2cresult < 1 || i2cresult > 7 ) {
        printf("Error: Unexpected result for power mode: u\n",i2cresult);
      } else {
        switch (i2cresult) {
          case 1: printf("External Power [EPR]\n"); break;
          case 2: printf("UPiS USB Power [USB]\n"); break;
          case 3: printf("Raspberry Pi USB Power [RPI]\n"); break;
          case 4: printf("Battery Power [BAT]\n"); break;
          case 5: printf("Low Power [LPR]\n"); break;
          case 6: printf("[CPR]\n"); break;
          case 7: printf("[BPR]\n"); break;
        }
      }
    } else {
      if (arg_count > 1) {
        printf("Power source: ");
      }
      printf("%u\n",readi2cbyte(0x01,0x6A,0x00));
    }
  }

  /* Display the Battery Voltage */
  if (arguments.batvolt) {
    if (arg_count > 1) {
      printf("BAT voltage: ");
    }
    printf("%g",((float)bcdword2dec(readi2cword(0x01,0x6A,0x01))/(float)100));
    if (arguments.verbose) {
      printf("V");
    }
    printf("\n");
  }

  /* Display RPI Voltage */
  if (arguments.rpivolt) {
    if (arg_count > 1) {
      printf("RPI Voltage: ");
    }
    printf("%g",((float)bcdword2dec(readi2cword(0x01,0x6A,0x03))/(float)100));
    if (arguments.verbose) {
      printf("V");
    }
    printf("\n");
  }

  /* Display EPR Voltage */
  if (arguments.eprvolt) {
    if (arg_count > 1) {
      printf("EPR Voltage: ");
    }
    printf("%g",((float)bcdword2dec(readi2cword(0x01,0x6A,0x07))/(float)100));
    if (arguments.verbose) {
      printf("V");
    }
    printf("\n");
  }

  /* Display USB Voltage */
  if (arguments.usbvolt) {
    if (arg_count > 1) {
      printf("USB Voltage: ");
    }
    printf("%g",((float)bcdword2dec(readi2cword(0x01,0x6A,0x05))/(float)100));
    if (arguments.verbose) {
      printf("V");
    }
    printf("\n");
  }

  /* Current Draw */
  if (arguments.current) {
    if (arg_count > 1) {
      printf("Average Current Draw: ");
    }
    printf("%i",bcdword2dec(readi2cword(0x01,0x6A,0x09)));
    if (arguments.verbose) {
      printf("mA");
    }
    printf("\n");
  }
  
  /* Temp in C */
  if (arguments.centigrade) {
    if (arg_count > 1) {
      printf("Centigrade Temperature: ");
    }
    printf("%u",bcdbyte2dec(readi2cbyte(0x01,0x6A,0x0B)));
    if (arguments.verbose) {
      printf("C");
    }
    printf("\n");
  }

  /* Temp in F */
  if (arguments.fahrenheit) {
    if (arg_count > 1) {
      printf("Fahrenheit Temperature: ");
    }
    printf("%u",bcdword2dec(readi2cword(0x01,0x6A,0x0C)));
    if (arguments.verbose) {
      printf("F");
    }
    printf("\n");
  }

  /* Firmware Version */
  if (arguments.fwver) {
    if (arg_count > 1) {
      printf("Firmware Version: ");
    }
    printf("%i\n",readi2cword(0x01,0x6B,0x00));
  }

  /* Factory Reset */
  if (arguments.factory) {
    if (!arguments.yes) {
      printf("WARNING: The UPiS will be returned to factory default and reset.\n");
      printf("This probably isn't a good idea as the Raspberry Pi will also be reset\n");
      printf("without a file safe shutdown, resulting in possible file system corruption.\n");
      printf("Type Y/y to proceed: ");
      scanf("%s",strresp);
    }
    if (arguments.yes || strcmp(strresp,"y") == 0 || strcmp(strresp,"Y") == 0)
    {
      /* Do the deed */
      writei2cbyte(0x01,0x69,0x07,0xdd);
    } else {
      printf("Factory reset aborted.\n");
    }
  }

  /* Reset */
  if (arguments.reset) {
    if (!arguments.yes) {
      printf("WARNING: The UPiS processor and RTC will be reset.\n");
      printf("This probably isn't a good idea as the Raspberry Pi will also be reset\n");
      printf("without a file safe shutdown, resulting in possible file system corruption.\n");
      printf("Type Y/y to proceed: ");
      scanf("%s",strresp);
    }
    if (arguments.yes || strcmp(strresp,"y") == 0 || strcmp(strresp,"Y") == 0)
    {
      /* Do the deed */
      writei2cbyte(0x01,0x69,0x07,0xee);
    } else {
      printf("Reset aborted.\n");
    }
  }

  /* Bootloader */
  if (arguments.bootloader) {
    if (!arguments.yes) {
      printf("WARNING: The UPiS will be placed in bootloader mode.\n");
      printf("1. The Red LED on the UPiS will light.\n");
      printf("2. Recovery from this state is only possible by pressing the RST button\n");
      printf("   or uploding new firmware.\n");
      printf("3. Bootloader mode should be used with the RPi firmware upload script.\n");
      printf("3. All interrupts are disabled during this procedure and the normal\n");
      printf("   operation of the UPiS is suspended.\n");
      printf("5. Both the UPiS and RPi must be powered via RPi micro USB during the\n");
      printf("   boot loading process because the UPiS resets after the firmware is\n");
      printf("   uploaded.\n");
      printf("Type Y/y to proceed: ");
      scanf("%s",strresp);
    }
    if (arguments.yes || strcmp(strresp,"y") == 0 || strcmp(strresp,"Y") == 0)
    {
      /* Do the deed */
      writei2cbyte(0x01,0x69,0x07,0xff);
    } else {
      printf("Bootloader aborted.\n");
    }
  }

  /* Last Error */
  if (arguments.errorno) {
    if (arg_count > 1) {
      printf("Last Error No: ");
    }
    printf("%i\n",readi2cbyte(0x01,0x6B,0x01));
  }

  /* Display/Set Watchdog Timer */
  if (arguments.watchdog) {
    if (!arguments.WDTIM) {
      if (arg_count > 1) {
        printf("Watchdog Timer: ");
      }
      printf("%i",readi2cbyte(0x01,0x6B,0x02));
      if (arguments.verbose)
      {
        printf(" (0x%02x)",readi2cbyte(0x01,0x6B,0x02));
      }
      printf("\n");
    } else {
      if (is_intstr(arguments.WDTIM) && atoi(arguments.WDTIM) >= 0 && atoi(arguments.WDTIM) <= 255)
      {
        writei2cbyte(0x01,0x6B,0x02,atoi(arguments.WDTIM));
        printf("Watchdog Timer set to: %i (0x%02x)\n",readi2cbyte(0x01,0x6B,0x02),readi2cbyte(0x01,0x6B,0x02));
      } else {
        printf("Invalid argument '%s' for watchdog timer - use an integer between 0 and 255\n",arguments.WDTIM);
      }
    }
  }

  /* File Safe Shutdown [FSSD] */
  if (arguments.fssd) {
    writei2cbyte(0x01,0x6B,0x02,0x00);
    printf("File safe shutdown initiated\n");
  }

  /* Display/Set FSSD Timer */
  if (arguments.fssdtimeout) {
    if (!arguments.FSSDTIM) {
      if (arg_count > 1) {
        printf("File Safe Shutdown Timer: ");
      }
      printf("%i",readi2cbyte(0x01,0x6B,0x03));
      if (arguments.verbose)
      {
        printf(" (0x%02x)",readi2cbyte(0x01,0x6B,0x03));
      }
      printf("\n");
    } else {
      if (is_intstr(arguments.FSSDTIM) && atoi(arguments.FSSDTIM) >= 15 && atoi(arguments.FSSDTIM) <= 255)
      {
        writei2cbyte(0x01,0x6B,0x03,atoi(arguments.FSSDTIM));
        printf("File Safe Shutdown Timer set to: %i (0x%02x)\n",readi2cbyte(0x01,0x6B,0x03),readi2cbyte(0x01,0x6B,0x03));
      } else {
        printf("Invalid argument '%s' for file safe shutdown timer - use an integer between 15 and 255\n",arguments.FSSDTIM);
      }
    }
  }

  /* Display/Set FSSD Type */
  if (arguments.fssdtype) {
    if (!arguments.FSSDACT) {
      if (arg_count > 1) {
        printf("File Safe Shutdown Type: ");
      }
      printf("%i",readi2cbyte(0x01,0x6B,0x04));
      if (arguments.verbose)
      {
        printf(" (0x%02x)",readi2cbyte(0x01,0x6B,0x04));
      }
      printf("\n");
    } else {
      if (is_intstr(arguments.FSSDACT) && atoi(arguments.FSSDACT) >= 0 && atoi(arguments.FSSDACT) <= 2)
      {
        writei2cbyte(0x01,0x6B,0x04,atoi(arguments.FSSDACT));
        printf("File Safe Shutdown Type set to: %i (0x%02x)\n",readi2cbyte(0x01,0x6B,0x04),readi2cbyte(0x01,0x6B,0x04));
      } else {
        printf("Invalid argument '%s' for file safe shutdown type - use an integer between 0 and 2\n",arguments.FSSDACT);
      }
    }
  }

  /* Display/Set FSSD Battery Mode Timer */
  if (arguments.fssdbatime) {
    if (!arguments.BATTIM) {
      if (arg_count > 1) {
        printf("File Safe Shutdown BAT Timer: ");
      }
      printf("%i",readi2cbyte(0x01,0x6B,0x05));
      if (arguments.verbose)
      {
        printf(" (0x%02x)",readi2cbyte(0x01,0x6B,0x05));
      }
      printf("\n");
    } else {
      if (is_intstr(arguments.BATTIM) && atoi(arguments.BATTIM) >= 0 && atoi(arguments.BATTIM) <= 255)
      {
        writei2cbyte(0x01,0x6B,0x05,atoi(arguments.BATTIM));
        printf("File Safe Shutdown BAT Timer set to: %i (0x%02x)\n",readi2cbyte(0x01,0x6B,0x05),readi2cbyte(0x01,0x6B,0x05));
      } else {
        printf("Invalid argument '%s' for file safe shutdown BAT timer - use an integer between 0 and 255\n",arguments.BATTIM);
      }
    }
  }

  /* Start Timer */
  if (arguments.starttimer) {
    printf("*** Not implemented yet ***\n");
  }

  /* Stop Timer */
  if (arguments.stoptimer) {
    printf("*** Not implemented yet ***\n");
  }

  /* LPR Wekeup Polling Timer */
  if (arguments.lprtimer) {
    if (!arguments.LPRTIM) {
      if (arg_count > 1) {
        printf("LPR Wakeup Polling Timer: ");
      }
      printf("%i",readi2cbyte(0x01,0x6B,0x0A));
      if (arguments.verbose)
      {
        printf(" (0x%02x)",readi2cbyte(0x01,0x6B,0x0A));
      }
      printf("\n");
    } else {
      if (is_intstr(arguments.LPRTIM) && atoi(arguments.LPRTIM) >= 0 && atoi(arguments.LPRTIM) <= 255)
      {
        writei2cbyte(0x01,0x6B,0x0A,atoi(arguments.LPRTIM));
        printf("LPR Wakeup Polling Timer set to: %i (0x%02x)\n",readi2cbyte(0x01,0x6B,0x0A),readi2cbyte(0x01,0x6B,0x0A));
      } else {
        printf("Invalid argument '%s' for LPR Wakeup Polling timer - use an integer between 0 and 255\n",arguments.LPRTIM);
      }
    }
  }

  /* Relay Control */
  if (arguments.relay) {
    if (!arguments.RLYSTAT) {
      if (arg_count > 1) {
        printf("Relay Status: ");
      }
      if (arguments.verbose)
      {
        if (readi2cbyte(0x01,0x6B,0x0B) == 0) {
          printf("on/closed");
        } else {
          printf("off/open");
        }
      } else {
        printf("%i",readi2cbyte(0x01,0x6B,0x0B));
       /* Relay Reversal Mod */
    /* if (readi2cbyte(0x01,0x6B,0x0B) == 0) {
          printf("%i",1);
        } else {
          printf("%i",0);
   }*/
      }
      printf("\n");
    } else {
      strlower(arguments.RLYSTAT);
      if (strcmp(arguments.RLYSTAT,"closed") == 0 || strcmp(arguments.RLYSTAT,"1") == 0 || strcmp(arguments.RLYSTAT,"on") == 0) {
        writei2cbyte(0x01,0x6B,0x0B,0x01);
        printf("Relay set to: on/closed\n");
      } else if (strcmp(arguments.RLYSTAT,"open") == 0 || strcmp(arguments.RLYSTAT,"0") == 0 || strcmp(arguments.RLYSTAT,"off") == 0) {
        writei2cbyte(0x01,0x6B,0x0B,0x00);
        printf("Relay set to: off/open\n");
      } else {
        printf("Invalid argument '%s' for relay state - use 0,1,open,closed,off or on\n",arguments.RLYSTAT);
      }
    }
  }

  /* EPR Switch to battery Threshold */
  if (arguments.eprlowv) {
    printf("*** Not implemented yet ***\n");
  }

  /* EPR Hysteresis */
  if (arguments.minlprtime) {
    printf("*** Not implemented yet ***\n");
  }

  /* LPR Switch Current Threshold */
  if (arguments.lprcurrent) {
    printf("*** Not implemented yet ***\n");
  }

  /* IO Pin Function */
  if (arguments.iomode) {
    if (!arguments.IOMODE) {
      if (arg_count > 1) {
        printf("IO Pin Mode: ");
      }
      printf("%i",readi2cbyte(0x01,0x6B,0x10));
      if (arguments.verbose)
      {
        printf(" (0x%02x)",readi2cbyte(0x01,0x6B,0x10));
      }
      printf("\n");
    } else {
      if (is_intstr(arguments.IOMODE) && atoi(arguments.IOMODE) >= 0 && atoi(arguments.IOMODE) <= 3)
      {
        writei2cbyte(0x01,0x6B,0x10,atoi(arguments.IOMODE));
        printf("IP Pin Mode set to: %i (0x%02x)\n",readi2cbyte(0x01,0x6B,0x10),readi2cbyte(0x01,0x6B,0x10));
      } else {
        printf("Invalid argument '%s' for io pin mode - use an integer between 0 and 3\n",arguments.IOMODE);
      }
    }
  }

  /* IO Pin Value */
  if (arguments.iovalue) {
    int i2cresult = readi2cbyte(0x01,0x6B,0x10);
    if (i2cresult < 0 || i2cresult > 3 ) {
      printf("Error: Unexpected io pin mode: %i\n",i2cresult);
    } else if (i2cresult == 0) {
        printf("IO Pin mode is not set\n");
    } else {
      if (arguments.verbose || arg_count > 1) {
        printf("IO Pin Value: ");
      }
      switch (i2cresult) {
        case 1: /* 1 wire temp value (word) */
                printf("%i",readi2cword(0x01,0x6B,0x11));
                break;
        case 2: /* 8 bit A/D value (byte) */
                printf("%i",readi2cbyte(0x01,0x6B,0x11));
                break;
        case 3: /* Staus of forced on-change (byte?) */
                printf("%i",readi2cbyte(0x01,0x6B,0x11));
                break;
      }
      printf("\n");
    }
  }

  return 0;
}

/* Converts a string to lowercase */
void strlower(char *string)
{
  while(*string) {
    if (*string >= 'A' && *string <= 'Z' ) {
      *string = *string + 32;
    }
    string++;
  }
}

/* Tests if a string is a valid representation of and integer */ 
int is_intstr(char *intstr) 
{
  int counter;  
  /* No strings that start with 0i allowed */
  if (intstr[0] == 48 && strlen(intstr) > 1) { return 0; }
  /* Are all the chars in the string numric? */
  for(counter = 0; counter < strlen(intstr); counter++)
  {
    if (intstr[counter] < 48 || intstr[counter] > 57) { return 0; }
  }
  return 1;
}

/* Procedure to read and retun an 8 bit (byte) integer from an I2C register at a given I2C address on a given I2C bus */
int readi2cbyte(unsigned int i2cbus, unsigned int i2caddr, unsigned int i2creg)
{
  int i2cfile;
  char i2cdev[20];

  snprintf(i2cdev, 19, "/dev/i2c-%d", i2cbus);
  i2cfile = open(i2cdev, O_RDWR);
  if (i2cfile < 0)
  {
    /* Unable to open I2C device */
    printf("Error: Unable to open i2c bus %u\n",i2cbus);
    exit(1);
  }

  if (ioctl(i2cfile, I2C_SLAVE, i2caddr) < 0)
  {
    /* Unable to read the PiCO interface */
    printf("Error: Unable to access the PiCO interface at address 0x%02x\n",i2cbus);
    exit(2);
  }

  __u8 i2c_register = i2creg; /* Device register to access */
  __s32 i2cresult;
  char buf[10];

  i2cresult = i2c_smbus_read_byte_data(i2cfile, i2c_register);
  if (i2cresult < 0 )
  {
    printf("Error: Unexpected result: u\n",i2cresult);
    exit(2);
  }
  return i2cresult;
}

/* Procedure to read and retun an 16 bit (word) integer from an I2C register at a given I2C address on a given I2C bus */
int readi2cword(unsigned int i2cbus, unsigned int i2caddr, unsigned int i2creg)
{
  int i2cfile;
  char i2cdev[20];

  snprintf(i2cdev, 19, "/dev/i2c-%d", i2cbus);
  i2cfile = open(i2cdev, O_RDWR);
  if (i2cfile < 0)
  {
    /* Unable to open I2C device */
    printf("Error: Unable to open i2c bus %u\n",i2cbus);
    exit(1);
  }

  if (ioctl(i2cfile, I2C_SLAVE, i2caddr) < 0)
  {
    /* Unable to read the PiCO interface */
    printf("Error: Unable to access the PiCO interface at address 0x%02x\n",i2cbus);
    exit(2);
  }

  __u8 i2c_register = i2creg; /* Device register to access */
  __s32 i2cresult;
  char buf[10];

  i2cresult = i2c_smbus_read_word_data(i2cfile, i2c_register);
  if (i2cresult < 0 )
  {
    printf("Error: Unexpected result: %u\n",i2cresult);
    exit(2);
  }
  return i2cresult;
}

/* Procedure to write a 8 bit (byte) inetger to an I2C register at a giving I2C address on a given I2C bus */
void writei2cbyte(unsigned int i2cbus, unsigned int i2caddr, unsigned int i2creg, unsigned int i2cval)
{
  int i2cfile;
  char i2cdev[20];

  snprintf(i2cdev, 19, "/dev/i2c-%d", i2cbus);
  i2cfile = open(i2cdev, O_RDWR);
  if (i2cfile < 0)
  {
    /* Unable to open I2C device */
    printf("Error: Unable to open i2c bus %u\n",i2cbus);
    exit(1);
  }

  if (ioctl(i2cfile, I2C_SLAVE, i2caddr) < 0)
  {
    /* Unable to read the PiCO interface */
    printf("Error: Unable to access the PiCO interface at address 0x%02x\n",i2cbus);
    exit(2);
  }

  __u8 i2c_register = i2creg; /* Device register to access */
  __s32 i2cresult;
  char buf[1] = { i2cval };
  i2cresult = i2c_smbus_write_byte_data(i2cfile, i2c_register, buf[0]);
  if (i2cresult < 0 ) {
    printf("Error: Unexpected result %i\n",i2cresult);
  }
}

/* Takes an unsigned int (16 bit) in Binary Coded Decimal (BCD) and returns a regular integer */
int bcdword2dec(unsigned int bcd)
{
  int a;
  int b;
  int c;
  int d;
  a = (bcd >> 12)&0x000F;
  b = (bcd >> 8)&0x000F;
  c = (bcd >> 4)&0x000F;
  d = bcd&0x000F;
  return((a*1000)+(b*100)+(c*10)+d);
}

/* Takes an unsigned small int (8 bit) in Binary Coded Decimal (BCD) and returns a regular integer */
int bcdbyte2dec(unsigned int bcd)
{
  int a;
  int b;
  a = (bcd >> 4)&0x000F;
  b = bcd&0x000F;
  return((a*10)+b);
}

