#include <stdint.h>
#include <stdbool.h>
#include "resources.h"

// TFT module connections
unsigned int TFT_DataPort at GPIOE_ODR;
sbit TFT_RST at GPIOE_ODR.B8;
sbit TFT_RS at GPIOE_ODR.B12;
sbit TFT_CS at GPIOE_ODR.B15;
sbit TFT_RD at GPIOE_ODR.B10;
sbit TFT_WR at GPIOE_ODR.B11;
sbit TFT_BLED at GPIOE_ODR.B9;
// End TFT module connections


volatile uint32_t bfr_ctr = 0;
volatile uint8_t serial_buffer[1000] = {0};
bool response_finished;
bool ok_response;
bool first_flag;

void clear_serial_buffer()
{
  uint32_t i;

  NVIC_IntDisable( IVT_INT_USART3 );

  for (i=0; i<1000; i++)
      serial_buffer[i] = 0;

  bfr_ctr = 0;

  NVIC_IntEnable( IVT_INT_USART3 );
}

void flags_false ()
{
    response_finished = false;
    ok_response = false;
    first_flag = false;
}

void display_init()
{
    TFT_Init_ILI9341_8bit( 320, 240 );
    TFT_BLED = 1;
    TFT_Set_Pen( CL_WHITE, 1 );
    TFT_Set_Brush( 1,CL_WHITE,0,0,0,0 );
    TFT_Set_Font( TFT_defaultFont, CL_BLACK, FO_HORIZONTAL );
    TFT_Fill_Screen( CL_WHITE );
    TFT_Set_Pen( CL_BLACK, 1 );
    TFT_Line( 20,  40, 300,  40 );
    TFT_Line( 20, 220, 300, 220 );
    TFT_Set_Font( &HandelGothic_BT21x22_Regular , CL_RED, FO_HORIZONTAL );
    TFT_Write_Text( "WiFi 5 click", 95, 14 );
    TFT_Set_Font( &Verdana12x13_Regular, CL_BLACK, FO_HORIZONTAL );
    TFT_Write_Text("EasyMx PRO v7 for STM32", 19, 223);
    TFT_Set_Font( &Verdana12x13_Regular, CL_RED, FO_HORIZONTAL );
    TFT_Write_Text( "www.mikroe.com", 200, 223 );
    TFT_Set_Font( TFT_defaultFont, CL_BLACK, FO_HORIZONTAL );
    Delay_ms(1000);
}

void parse_data ()
{
   static volatile char *xml_ptr;
   char city_str[9];
   char temperature_string[12];
   char humidity_string[8];
   char pressure_string[10];
   int temp;


   xml_ptr = strstr(serial_buffer, "name");

   strncpy(city_str, xml_ptr+6, 8);
   city_str[8]='\0';

   TFT_Write_Text("City: ",10,50);
   TFT_Write_Text(city_str, 50,50);

   xml_ptr = strstr(serial_buffer, "temperature value");
   strncpy(temperature_string, xml_ptr+19, 5);
   temperature_string[5] = '\0';

   temp = atoi(temperature_string);
   temp = temp - 273;
   inttostr(temp, temperature_string);
   TFT_Write_Text("Temperature: ", 10,80);
   TFT_Write_Text(temperature_string, 100,80);
   TFT_Write_Text("Celsius", 200,80);

   xml_ptr = strstr(serial_buffer,"humidity value=");
   strncpy(humidity_string, xml_ptr+16, 2);
   humidity_string[2] = '\0';
   TFT_Write_Text("Humidity: ", 10,110);
   TFT_Write_Text(humidity_string, 110,110);
   TFT_Write_Text("%", 200,110);

   xml_ptr = strstr(serial_buffer,"pressure value=");
   strncpy(pressure_string, xml_ptr+16, 4);
   pressure_string [4] = '\0';
   TFT_Write_Text("Pressure: ", 10,140);
   TFT_Write_Text(pressure_string, 110,140);
   TFT_Write_Text("hPa ", 200,140);

}


void main()
{
     // initialize the UART for monitoring the program
     UART1_Init_Advanced( 9600,  _UART_8_BIT_DATA,
                                  _UART_NOPARITY,
                                 _UART_ONE_STOPBIT,
                                 &_GPIO_MODULE_USART1_PA9_10 );
     Delay_ms(300);
     UART1_Write_Text("Uart initialized\r\n");

     Delay_ms(5000);
     // initialize the UART for communication between the MCU and the WiFi 5 Click
     UART3_Init_Advanced( 9600, _UART_8_BIT_DATA,
                                _UART_NOPARITY,
                                _UART_ONE_STOPBIT,
                                &_GPIO_MODULE_USART3_PD89);

     Delay_ms(5000);


     UART1_Write_Text("Uarts ready\r\n");

     Delay_ms(1000);
     
     // set up the UART interrupt
     RXNEIE_USART3_CR1_bit = 1;
     NVIC_IntEnable( IVT_INT_USART3 );
     EnableInterrupts();

     // clear the buffer and the flags, setting them ready for the program to start
     clear_serial_buffer();
     flags_false();
     UART3_WRITE_TEXT("AT+WPAPSK=MikroE Public,mikroe.guest\r\n");   // compute the PSK from SSID and PassPhrase
     while(response_finished == false);                              // wait for the "OK" response

     UART1_WRITE_TEXT(serial_buffer);                                // print out the response

     clear_serial_buffer();                                          // clear the buffer and flags
     flags_false();
     Delay_ms(300);
     UART3_WRITE_TEXT("AT+WA=MikroE Public\r\n");                    // try to connect to MikroE Public network
     while (response_finished == false);                             // wait for the "OK" response

     UART1_WRITE_TEXT(serial_buffer);                                // print out the response

     clear_serial_buffer();                                          // clear the buffer and flags
     flags_false();
     UART3_WRITE_TEXT("at+httpopen=api.openweathermap.org\r\n");     // open the api.openweathermap.org web page
     while (response_finished == false);                             // wait for the "OK" response

     UART1_WRITE_TEXT(serial_buffer);                                // print out the response

     clear_serial_buffer();                                          // clear the buffer and flags
     flags_false();
     UART3_WRITE_TEXT("at+httpconf=11,api.openweathermap.org\r\n");  // configure the right http parameters
     while (response_finished == false);                             // wait for the "OK" response

     UART1_WRITE_TEXT(serial_buffer);                                // print out the response

     clear_serial_buffer();                                          // clear the buffer and flags
     flags_false();
     UART3_WRITE_TEXT("at+httpsend=0,1,5,/data/2.5/weather?q=Belgrade&mode=xml&appid=438f494400615270b4d2c1f9d563bf19\r\n");   // send the http request


     while ( bfr_ctr <  834 );                                       // wait until the buffer fills up with data
     UART1_write_text("Data fetched \r\n\r\n");                      // notify the user that we have fetched the data from the web

//********** PARSING PART***********************//

       display_init();                                               // initialize the TFT display
       parse_data();                                                 // parse the data and display it on the TFT

}

void wifi5_rx_isr( char rx_input )
{

        if (rx_input == 'K')
        {
            if (serial_buffer[bfr_ctr - 1] == 'O')
               ok_response = true;
        }

        if (rx_input == '\n' && ok_response == true && serial_buffer[bfr_ctr - 1] == '\r')
        {
            if (first_flag != true) first_flag = true;
            else if (rx_input == '\n' && first_flag == true && serial_buffer[bfr_ctr - 1] == '\r')
                response_finished = true;
        }
        serial_buffer[bfr_ctr++] = rx_input;

}


void WF_RX() iv IVT_INT_USART3 ics ICS_AUTO
{
     char tmp;
     if( RXNE_USART3_SR_bit == 1)
     {
         if (bfr_ctr == 999)
         {
           UART1_WRITE_TEXT("CTR OVERFLOW");
           while(1);
         }
       tmp = USART3_DR;
       wifi5_rx_isr(tmp);
     }
}