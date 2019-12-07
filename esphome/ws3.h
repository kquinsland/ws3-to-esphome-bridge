#include "esphome.h"

/*
    
    There are two jumpers that allow you to configure behavior of the unit.

    JP1: Toggles between "Basic" and "Professional" mode.
      Closed circuit for Basic/ARS inch mode
      Open circuit for Professional metric mode

    JP2: Toggles between 2400 and 9600 baud.
      Closed circuit for 2400 baud
      Open circuit for 9600 baud
 


    # See: https://esphome.io/custom/custom_component.html
    # See: https://esphome.io/custom/uart.html
*/

// The WS3 defaults to 2400 baud, can be adjusted to 9600 w/ one of the jumpers
#define WS3_BAUD 2400

// There are 38 bytes per packet in "basic" mode; packets are longer in scientific mode
// Jumper XY toggles between Basic and Scientific mode
#define WS3_PKT_LEN 38

// And the checksum is the last 2 bytes
#define WS3_CHK_LEN 2

// Useful scratch space for building strings for logging
#define BUFSIZE 200
char logbuf[BUFSIZE];

// Log tag
static const char* TAG = "WS3";


// The packets we get will be a string of ascii, easiest to just capture as a String
String pkt_str = "";

// And allocate space for the useful payload, after we strip the chksum from the packet
String payload = "";

// Flag for packet cap status
bool pkt_ok = false;


class WS3 : public PollingComponent, public UARTDevice {

  // This packet assumes "basic" operation
  // TODO: use IFDEF to toggle between the two packet types?
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

  public:
    WS3(UARTComponent *parent) : PollingComponent(15000), UARTDevice(parent) {};

    // For each of the values we wish to export, we define a sensor
    Sensor *temperature_sensor = new Sensor();
    Sensor *pressure_sensor = new Sensor();
    Sensor *humidity_sensor = new Sensor();

    Sensor *wind_speed_current_sensor = new Sensor();
    Sensor *wind_speed_peak_5m_sensor = new Sensor();

    Sensor *wind_direction_sensor = new Sensor();
    Sensor *rain_fall_1h_sensor = new Sensor();
    Sensor *rain_fall_24h_sensor = new Sensor();

    void setup() override {
      // ESPHome takes care of setting up the UART for us, all we need to do is 
      // allocate some memory for the incoming packet.
      ESP_LOGD("ws3", "Setting up...");
      pkt_str.reserve(WS3_PKT_LEN);
      payload.reserve(WS3_PKT_LEN-WS3_CHK_LEN);

      init_buf(logbuf, BUFSIZE);

    }

    void loop() override {

      // While data comes in and we don't have a pending packet to process...
      while (available() && pkt_ok != true) {

        // Pull the bytes off the stream
        char inChar = read();

        // And build up the packet
        pkt_str += inChar;

        // Until we hit the end
        if (inChar == '\n') {
          pkt_ok = true;
        }
      }

      // if pkt_ok is True, we now have a valid packet! The string will look something like this:
      // c000s000g000t069r000p000h49b09945*38

      // Now, we attempt to parse out the string into fields that we can work with
      if (pkt_ok) {
        ESP_LOGD("ws3", "pkt_ok!");

        // At this point, we have a string of characters that was probably a valid packet
        // We now attempt to parse the string into the struct
        if (process_packet(pkt_str, privPkt)) {
          // TODO: this needs to be re-written for ESPHome
          //print_weather(privPkt);
        } 

        // reset pkt_ok flag until we get new data...
        pkt_str = "";
        pkt_ok = false;
        
      } else {
        // Print Packet not OK + what we did get...
        // Don't print the invalid packets; there will be a LOT of them if the uart is floating or the WS3 is not
        //  plugged in.
        
        // ESP_LOGD(TAG, "pkt not ok :(");
        // init_buf(logbuf, BUFSIZE);
        // pkt_str.toCharArray(logbuf,pkt_str.length());
        // ESP_LOGD(TAG, "pkt: [%s]", logbuf);
      }

    }

    void update() override {
      // This is the actual sensor reading logic.
      ESP_LOGD(TAG, "Update has been called...");
      temperature_sensor->publish_state(privPkt->temp_f);
      pressure_sensor->publish_state(privPkt->air_pressure);
      humidity_sensor->publish_state(privPkt->humidity);

      wind_speed_current_sensor->publish_state(privPkt->wind_speed);
      wind_speed_peak_5m_sensor->publish_state(privPkt->wind_speed_5m);
      wind_direction_sensor->publish_state(privPkt->wind_dir);

      rain_fall_1h_sensor->publish_state(privPkt->rain_1h);
      rain_fall_24h_sensor->publish_state(privPkt->rain_24h);
    }

  private:

    // We store the packet privately and pull values from it when update() is called
    WS3Packet *privPkt = new WS3Packet();

    // Helper function to allocate buffers
    void init_buf(char *buf, size_t size) {
      int i;
      for(i=0; i<size; i++){
          buf[i] = i + '0'; // int to char conversion
      }
    }

    // Packet parse function
    // Note: Assumes BASIC packet format, not scientific!
    void parse_packet(String payload, WS3Packet* p) { 

      // E.G.: c000 s000 g000 t075 r000 p019 h43 b09940 
      
      // Parse in order, starting with c000 (wind direction in degrees)
      int wind_dir_idx = payload.indexOf('c');
      // Degrees will be 0 through 360, INT is fine.
      p->wind_dir = payload.substring(wind_dir_idx+1, wind_dir_idx+4).toInt();

      // Then move on to s000 - wind speed in MPH
      int wind_speed_idx = payload.indexOf('s');
      // It's not clear if the MPH comes in sub 1 mph units or not; I assume not, so INT is fine.
      p->wind_speed = payload.substring(wind_speed_idx+1, wind_speed_idx+4).toInt();

      // Then move on to g000 - wind speed over the last 5 min, in MPH
      int wind_speed_5_idx = payload.indexOf('g');
      // Just like w/ the `s` unit, i don't expect MPH to be in sub 1 units, INT is fine.
      p->wind_speed_5m = payload.substring(wind_speed_5_idx+1, wind_speed_5_idx+4).toInt();

      // Then move on to t075 - temp
      int temp_idx = payload.indexOf('t');
      // It's not clear if the temperature comes in sub 1 degree units, assume NO. INT is fine.
      p->temp_f = payload.substring(temp_idx+1, temp_idx+4).toInt();

      // Then move on to R000 - rain last hour
      int rain1h_idx = payload.indexOf('r');
      // Documents indicate that resolution is .01 inches, FLOAT used.
      p->rain_1h  = payload.substring(rain1h_idx+1, rain1h_idx+4).toInt() *.01;

      // Then move on to P000 - rain last hour
      int rain24h_idx = payload.indexOf('p');
      // Like with `r` token, resolution is .01 inches, FLOAT used.
      p->rain_24h  = payload.substring(rain24h_idx+1, rain24h_idx+4).toInt()*.01;

      // Then move on to H53 - Humidity
      int humidity_idx = payload.indexOf('h');
      // values from 0 to 99, INT is fine.
      p->humidity  = payload.substring(humidity_idx+1, humidity_idx+3).toInt();

      // Then move on to B10020 - air pressure
      int pressure_idx = payload.indexOf('b');
      // Documents indicate that resolution 0.1 hpa, FLOAT used
      p->air_pressure  = payload.substring(pressure_idx+1, pressure_idx+6).toInt()*.1;

    }

    bool validate_packet(String pay, unsigned long chk) {

      // Print the payload and the checksum we want
      init_buf(logbuf, BUFSIZE);
      pay.toCharArray(logbuf,pay.length());
      ESP_LOGD(TAG, "Validating: %s", logbuf);

      // SUPER grateful for this helpful tool for double checking my math!
      // https://toolslick.com/math/bitwise/xor-calculator

      // Current byte
      byte i1 = 0;

      // the intermediate checksum
      byte tmp = 0;

      // starting from the first char, cumulative XOR until * 
      for (int x = 0; x < pay.length() ; x++) {

        i1=pay[x];

        // Do the xOR
        tmp = tmp^i1;

      }

      // do the check
      if(tmp == chk) {
        return true;
      } else {
        init_buf(logbuf, BUFSIZE);
        pay.toCharArray(logbuf,pay.length());
        ESP_LOGE("ws3", "Invalid: %lX Calculated: %X", chk, tmp);
        return false;
      }
    }

    // Takes the raw packet and slices/dices/validates it
    bool process_packet(String pkt, WS3Packet* p) {
      // The first thing we need to do is split the checksum from the data
      // Packet: c000s000g000t075r000p019h43b09940*32
      // c0 00 s0 00 g0 00 t0 75 r0 00 p0 19 h4 3b 09 94 0* 32

      // everything after the * is checksum (2 char long)
      unsigned long chksum;
    
      // Check if the 33rd character is *
      if (pkt.charAt(33) != '*') {
        init_buf(logbuf, BUFSIZE);
        pkt.toCharArray(logbuf,pkt.length());
        ESP_LOGE(TAG, "Invalid, no * at position 33. Packet: [%s]", logbuf);
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
        parse_packet(payload, p);
        return true;
      }
      return false;
    }

};
