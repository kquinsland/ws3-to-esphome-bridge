/*
  The idea is to do a simple serial passthrough + parse of the WS3 data

  This sketch was developed for the ESP32 with the eventual intention of turning into a custopm ESPHOME
  module. 

  ESP32 pins:
    https://docs.google.com/spreadsheets/d/12qL3ui2BkSn91O0ISJU8QIL2mcG-r_vlX0briknA2QQ/edit#gid=1401515447

  WS3 Data Stream:
    (used google translate on this page)
    https://item.taobao.com/item.htm?spmx=2013.1.0.0.47714274JGdV7o&id=21348003785

  There are two jumpers to configure the WS3 behavior:
    
    JP1 toggles between ARS inch mode and "professional metric mode"
      short circuit = inch, open = metric

    JP2 is baud rate
      short = 2400, open 9600.


    In either case, data is spit out at 2hz (.5 sec). The data looks like this:

        APRS data output format (imperial, default) :
    
        C000s000g000t082r000p000h48b10022*3C
     

        Output 38 bytes every 0.5 seconds including newline at the end of the data ( OD, OA )

    Each field:

      C000 : wind direction angle, unit: degree .
      
      S000 : before . 1 minutes wind speed, unit: mph
      
      G000 : the highest wind speed in the first 5 minutes, in miles per hour
      
      T086 : temperature (Fahrenheit)
      
      R000 : the previous hour's rainfall ( 0.01 inches)
      
      P000 : rainfall during the first 24 hours ( 0.01 inches)
      
      H53 : humidity ( 00 % - 99 %) 
      
      B10020 : air pressure ( 0.1 hpa )

   The last token is deliniated by a *. This is the 2 byte checksum.
    The RAW translation is:

        *20   checksum, between A and * (excluding * ) all character XOR results
        
    This looks like a translation artifact. The code that i put together below works. XOR first character up to the * char.
    The result should equal the last two characters, if they're cast to hex.

    E.G.:
      packet: c000s000g000t072r143p143h52b09932*38
      xOR all this together: c000s000g000t072r143p143h52b09932 and you should get 0x38


  Other things to note:
    The interface board will automatically detect whether the air pressure gauge, temperature and humidity sensor
    are installed, and the sensor data that is not installed will display “...”.

    For example, if the temperature and humidity sensor and the air pressure piece are not installed, the output data is:

    C000s000g000t...r000p000h..b.....

  The WS3 is *not* smart enough to detect if something is plugged in or not, though. So if the wind speed sensor is missing *or* there 
    is no wind, you get 0000

  ----
  The ESP32 has 3 UARTS. 2 of which are usable

  UART0: (GPIO1/TX and GPIO3/RX)  [This is wired to USB]
  UART2: (GPIO 17 devRX and GPIO 16 devTX)    [This is WS3]

  UART1: (GPIO 9 and GPIO10)      [ESP32 SPI flash; see: youtu.be/GwShqW39jlE?]
  
*/

// Some basic DEFS
#define WS3_BAUD 2400
#define USB_BAUD 2400

// There are 38 bytes per packet
#define WS3_PKT_LEN 38

// And the checksum is the last 2 bytes
#define WS3_CHK_LEN 2


// Setup the UART for WS3
HardwareSerial WS3(2); 

// Place to hold the packet
String pkt_str = "";

// flag for packet OK
bool pkt_ok = false;

// After parsing the string of bytes, we'll have an easier to use struct
// TODO: this should be it's own file?
struct WS3Packet {

  // The 1st field is "C000" - wind direction, unit: degree
  int wind_dir;

  // The 2nd field is "S000" - 1 minutes wind speed, unit: mph
  int wind_speed;

  // The 3rd field is "G000" - the highest wind speed in the last 5 minutes, unit: mph
  int wind_speed_5m;

  // The 4th field is "T086" - temperature, unit: degree Fahrenheit
  int temp_f;
  
  // The 5th field is "R000" - the previous hour's rainfall ( 0.01 inches)
  float rain_1h;

  // The 6th field is "P000" - rainfall during the first 24 hours ( 0.01 inches)
  float rain_24h;

  // The 7th field is "H53" - humidity ( 00 % - 99 %) 
  int humidity;

  // The 8th field is "B10020" - air pressure ( 0.1 hpa )
  float air_pressure;
      
};

void setup() {
  // First, init the USB serial
  Serial.begin(USB_BAUD);
  Serial.println("[I] - USB UART ALive...");

  // Then move onto the WS3
  WS3.begin(WS3_BAUD, SERIAL_8N1);
  Serial.println("[I] - WS3 UART ALive...");

  // And then allocate some memory for the packet
  pkt_str.reserve(WS3_PKT_LEN);
  Serial.println("[I] - Allocated packet memory...");

}


void loop() {

  // While data comes in and we don't have a pending packet to process...
  while (WS3.available() && pkt_ok !=true) {
    
    // Pull the bytes off the stream
    char inChar = (char)WS3.read();
    
    // And build up the packet
    pkt_str += inChar;
    
    // Until we hit the end
    if (inChar == '\n') {
      pkt_ok = true;
    }
  }

  // Yay, we now have a packet! The string will probably look something like this:
  // c000s000g000t069r000p000h49b09945*38

  // Now, we attempt to parse out the string into a packet that we can work with
  if (pkt_ok) {
    Serial.println("pkt_ok!");

    // At this point, we have a string of characters that was probably a valid packet
    // We set get some memory and attempt to parse the string into the struct
    WS3Packet p = {};
  
    // Validate the payload, then parse it. 
    if (process_packet(pkt_str, &p)) {
      // print results if parse OK
      print_weather(&p);
    } else {
      Serial.println("unable to parse packet :(");
    }

   
    // clear so we can start again
    pkt_str = "";
    pkt_ok = false;
    clear_pkt(&p);
    
  }

}


bool process_packet(String pkt, WS3Packet* p) {
  Serial.println("[D] process_packet - ALive!");
  Serial.println(pkt);

  // The first thing we need to do is split the checksum from the data
  // Packet: c000s000g000t075r000p019h43b09940*32
  // c0 00 s0 00 g0 00 t0 75 r0 00 p0 19 h4 3b 09 94 0* 32

  // Allocate bytes for the payload
  String payload;
  payload.reserve(WS3_PKT_LEN-WS3_CHK_LEN);

  // everything after the * is checksum (2 char long)
  unsigned long chksum;
 
  // Check if the 33rd character is *
  if (pkt.charAt(33) != '*') {
    Serial.println("Packed invalid; no * character at position 33!");    
    return false;
  } else {
    // The character indicating the checksum is coming is in the correct place. Yay.
    // Now, we need to pull the two ascii characters that are transmitted to us
    // and turn them into a single byte. E.G. Char 3, Char D should convert to 0x3D.
    //
    // We can do this with the strtoul() function; we indicate that we wante base 16
    chksum = strtoul(pkt.substring(34, 36).c_str(),NULL,16);
  }

  // We have the checksum, Now we can bother to get the payload
  payload = pkt.substring(0, 33);

  // And try to validate...
  if(validate_packet(payload, chksum)){
    Serial.println("Valid packet!");
    parse_packet(payload, p);
  } else {
    Serial.println("invalid packet! :(");
    return false;
  }
  
}


bool validate_packet(String pay, unsigned long chk) {

  // Print the payload and the checksum we want
  Serial.println("validate_packet:");
  Serial.print(pay);
  Serial.print(" * ");
  Serial.println(chk);

  // TEST DATA (actual packets)
  // c000s000g000t075r000p019h43b09940*32
  // String pay = "c000s000g000t075r000p019h43b09940";
  // byte chk = 0x32; // this will be in HEX)

  // c000s000g000t075r000p019h42b09940*33
  // String pay = "c000s000g000t075r000p019h42b09940";
  // byte chk = 0x33; // this will be in HEX)
  
  // c000s000g000t075r000p019h42b09939*3D
  // String pay = "c000s000g000t075r000p019h42b09939";
  // byte chk = 0x3D; // this will be in HEX)

  // SUPER grateful for the helpful https://toolslick.com/math/bitwise/xor-calculator to validate my
  // code!

  // Current byte
  byte i1=0;

  // the intermediate checksum
  byte tmp = 0;

  // starting from the second character, we begin XORing
  for (int x = 0; x < pay.length() ; x++) {

    i1=pay[x];
        
    // Do the xOR
    tmp = tmp^i1;

  }

  // do the check
  if(tmp == chk){
    return true;
  } else {
    Serial.println("INVALID!");
    Serial.print("calculated:");
    Serial.println(tmp);
    return false;
  }
}


void parse_packet(String payload, WS3Packet* p) { 

  // E.G.: c000 s000 g000 t075 r000 p019 h43 b09940 
  
  // Parse in order, starting with c000 (wind dir, degrees)
  int wind_dir_idx = payload.indexOf('c');
  p->wind_dir = payload.substring(wind_dir_idx+1, wind_dir_idx+4).toInt();

  // Then move on to s000 - wind speed
  int wind_speed_idx = payload.indexOf('s');
  p->wind_speed = payload.substring(wind_speed_idx+1, wind_speed_idx+4).toInt();

  // Then move on to g000 - wind speed over the last 5 min
  int wind_speed_5_idx = payload.indexOf('g');
  p->wind_speed_5m = payload.substring(wind_speed_5_idx+1, wind_speed_5_idx+4).toInt();

  // Then move on to t075 - temp
  int temp_idx = payload.indexOf('t');
  p->temp_f = payload.substring(temp_idx+1, temp_idx+4).toInt();

  // Then move on to R000 - rain last hour
  int rain1h_idx = payload.indexOf('r');
  p->rain_1h = payload.substring(rain1h_idx+1, rain1h_idx+4).toInt() *.01;

  // Then move on to P000 - rain last hour
  int rain24h_idx = payload.indexOf('p');
  p->rain_24h = payload.substring(rain24h_idx+1, rain24h_idx+4).toInt()*.01;

  // Then move on to H53 - Humidity
  int humidity_idx = payload.indexOf('h');
  p->humidity = payload.substring(humidity_idx+1, humidity_idx+3).toInt();

  // Then move on to B10020 - air pressure
  int pressure_idx = payload.indexOf('b');
  p->air_pressure = payload.substring(pressure_idx+1, pressure_idx+6).toInt()*.1;

}

void print_weather(WS3Packet* p){
  Serial.print("Wind Direction: ");
  Serial.print(p->wind_dir, DEC);
  Serial.println(" degrees");

  Serial.print("wind_speed: ");
  Serial.print(p->wind_speed, DEC);
  Serial.println(" mph");
  
  Serial.print("wind_speed_5m: ");
  Serial.print(p->wind_speed_5m, DEC);
  Serial.println(" mph");

  Serial.print("temp_f: ");
  Serial.print(p->temp_f, DEC); 
  Serial.println(" deg. F.");

  Serial.print("rain_1h: ");
  Serial.print(p->rain_1h, DEC); 
  Serial.println(" Inches");

  Serial.print("rain_24h: ");
  Serial.print(p->rain_24h, DEC); 
  Serial.println(" Inches");

  Serial.print("humidity: ");
  Serial.print(p->humidity, DEC); 
  Serial.println(" %");

  Serial.print("air_pressure: ");
  Serial.print(p->air_pressure, DEC); 
  Serial.println(" hpa");
 }

void clear_pkt(WS3Packet* p) {
  
  p->wind_dir = 0;
  p->wind_speed = 0;
  p->wind_speed_5m = 0;
  p->temp_f = 0;
  p->rain_1h = 0;
  p->rain_24h = 0;
  p->humidity = 0;
  p->air_pressure = 0;
  
}
