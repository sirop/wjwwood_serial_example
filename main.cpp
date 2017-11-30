
#include <string>
#include <iostream>
#include <iomanip> // for std::setfill
#include <cstdio>

#include <chrono>

// OS Specific sleep
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "serial/serial.h"

using std::string;
using std::exception;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;

void my_sleep(unsigned long milliseconds) {
#ifdef _WIN32
      Sleep(milliseconds); // 100 ms
#else
      usleep(milliseconds*1000); // 100 ms
#endif
}

std::string string_to_hex(const std::string& input)
{
    static const char* const lut = "0123456789ABCDEF";
    size_t len = input.length();

    std::string output;
    output.reserve(2 * len);
    for (size_t i = 0; i < len; ++i)
    {
        const unsigned char c = input[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

std::string string_to_hex2(const std::string& in) {
    std::stringstream ss;

    ss << std::hex << std::setfill('0');
    for (size_t i = 0; in.length() > i; ++i) {
        ss << std::setw(2) << (static_cast<unsigned int>(static_cast<uint8_t>(in[i])))<<" ";
    }

    return ss.str();
}

std::string find_frame(const std::string& in) {
    int i ;
    for (i=0;  i< (int) in.length(); i++)
    {
        if ((unsigned char)(in[i])=='0xAA')
            break;

    }

    if (i<=in.length())
    {
        //if (check_sum(in)== (uin16_t*) &in[34])
        return in.substr(i-1, 24);
    }
    else return std::string("NO FRAME");
}

//std::string find_frame2(const std::string& in) {
//    // const  char* search_cstr = 0xAA;
//     char prefix [] = {0xAA, 0x55 };
////     prefix[0]=0xAA;
////     prefix[1]=0x55;
//      std::string search_str = std::string( prefix);

//    int i = in.find(search_str);

//    if (i<in.length()-36)
//       return in.substr(i, 36);
//    else return std::string(1, 0x00);
//}

size_t find_frame3(const std::string& in) {

    uint8_t arr[] = {0xAA, 0x55};

    std::string search_str(arr, arr+2);
    size_t i = in.find(search_str);


    if (i==std::string::npos)
       return std::string::npos;
    else
        return i;
}


uint16_t checksum(const std::string& in)
{
    uint16_t temp = 0;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(in.c_str());
    for (int i=2; i<(in.length()-2); i++)
        temp = temp + p[i];

    return temp;
}

uint16_t checksum2(const uint8_t* buffer)
{
    uint16_t temp = 0;
    //const uint8_t* p = reinterpret_cast<const uint8_t*>(in.c_str());
    for (int i=2; i< 34 ; i++)
        temp = temp + (uint16_t)buffer[i];

    return temp;
}



void enumerate_ports()
{
    vector<serial::PortInfo> devices_found = serial::list_ports();

    vector<serial::PortInfo>::iterator iter = devices_found.begin();

    while( iter != devices_found.end() )
    {
        serial::PortInfo device = *iter++;

        printf( "(%s, %s, %s)\n", device.port.c_str(), device.description.c_str(),
     device.hardware_id.c_str() );
    }
}

void print_usage()
{
    cerr << "Usage: test_serial {-e|<serial port address>} ";
    cerr << "<baudrate>" << endl;
}

int run(int argc, char **argv)
{
  if(argc < 2) {
      print_usage();
    return 0;
  }

  // Argument 1 is the serial port or enumerate flag
  string port(argv[1]);

  if( port == "-e" ) {
      enumerate_ports();
      return 0;
  }
  else if( argc < 3 ) {
      print_usage();
      return 1;
  }

  // Argument 2 is the baudrate
  unsigned long baud = 0;
#if defined(WIN32) && !defined(__MINGW32__)
  sscanf_s(argv[2], "%lu", &baud);
#else
  sscanf(argv[2], "%lu", &baud);
#endif

  // port, baudrate, timeout in milliseconds
  serial::Serial my_serial(port, baud, serial::Timeout::Timeout(0,1,0,0,0));

 // cout << "Is the serial port open?" << endl;
  if(my_serial.isOpen()) {
    cout << port << " opened? " << " Yes." << endl;

    my_serial.flushInput();  // see https://github.com/wjwwood/serial/issues/161
    // https://github.com/wjwwood/serial/pull/153/files
    // https://github.com/wjwwood/serial/issues/148
    // see also https://github.com/wjwwood/serial/pull/154/files

    my_sleep(5); // 5 milliseconds

  }
//  else
//    cout << port << " opened? " << " No." << endl;

  int cycles = 10;

  size_t length = 0;
  uint8_t* buffer = new uint8_t[288];
  std::vector<std::string> resString;
  std::vector<std::string> *resBuffer = new std::vector<std::string>(cycles+1);
  string bString;

  int nFrames = 0;

  auto start = std::chrono::system_clock::now();

  for (int k=0; k< cycles; k++)
  {
      size_t bytes = my_serial.read(&buffer[length],36);
      // std::cout <<"Bytes: " << bytes << endl;
      // std::cout <<"bytes + length : " << (bytes + length) << endl;
      string result = std::string (&buffer[0], &buffer[0] + length + bytes );

      resString.push_back(result);

      // std::cout << string_to_hex(result) << endl;
      if ( (bytes + length) < 36 )
      {
          length = length + bytes;
      }
      else
      {
          // std::cout << "Result length: " << result.length() << endl;
          size_t i = find_frame3(result);
          //std::cout << "I Frame: " << std::dec << i << endl;
          if (i!=std::string::npos) // 0xAAx55 found
          {
              std::memmove(&buffer[0], &buffer[i], result.length()-i); // -1 ?
              length = result.length()-i ;

              bString = std::string(buffer, &buffer[0]+length);
              resBuffer->at(k)=bString;

              //std::cout << string_to_hex2(std::string(buffer, buffer + length) ) <<endl;
              if ((length) >= 36)
              {


                  //  std::cout << string_to_hex2(std::string(buffer, buffer + 36) ) <<endl;
                  uint16_t temp = checksum2(buffer);
                  //          std::cout << "Checksum swapped: " << std::hex << temp << endl;
                  uint16_t temp2 = *(uint16_t*)&buffer[34];
                  //  std::cout << "Checksum as is: " << std::hex << temp2 << endl;

#if _MSC_VER
                  if (_byteswap_ushort(temp)==temp2)
                  {
                      temp = *(uint16_t*)&buffer[2];
                      uint16_t time = _byteswap_ushort(temp);
                      //                  std::cout << "Timestamp as is: " << std::dec << temp << "  " << std::hex << temp <<  endl;
                      //                  std::cout << "Timestamp: " << std::dec << time << "  " << std::hex << time <<  endl;
                      nFrames++;


                      uint8_t Pan_SpeedPot = (uint8_t) buffer[4];
                      //uint16_t temp3 = *(uint16_t*)&buffer[5];
                      int16_t Pan_Velocity = _byteswap_ushort(*(uint16_t*)&buffer[5]);
                      //uint32_t temp4 = *(uint32_t*)&buffer[7];
                      int32_t Pan_Position = _byteswap_ulong(*(uint32_t*)&buffer[7]);

                      uint8_t Tilt_SpeedPot = buffer[11];
                      //temp3 = *(uint16_t*)&buffer[12];
                      int16_t Tilt_Velocity =  _byteswap_ushort(*(uint16_t*)&buffer[12]);
                      //temp4 = *(uint32_t*)&buffer[14];
                      int32_t Tilt_Position = _byteswap_ulong(*(uint32_t*)&buffer[14]);

                      uint8_t Roll_SpeedPot = buffer[18];
                      //temp3 = *(uint16_t*)&buffer[19];
                      int16_t Roll_Velocity =  _byteswap_ushort(*(uint16_t*)&buffer[19]);
                      //temp4 = *(uint32_t*)&buffer[21];
                      int32_t Roll_Position = _byteswap_ulong(*(uint32_t*)&buffer[21]);

                      uint8_t Direction_Switches = buffer[25];
                      uint8_t Auxilliary_Switches = buffer[26];



                      //              std::cout << "Pan_SpeedPot: " << std::dec <<static_cast<uint16_t>( Pan_SpeedPot) << endl;
                      //              // std::cout << "Pan_SpeedPot 2: " << std::hex << string_to_hex2(std::string(1,static_cast<unsigned int>( buffer[4]))) << endl;
                      //              std::cout <<std::dec << "Pan_Velocity: "<< Pan_Velocity <<  endl;
                      //              std::cout <<std::dec << "Pan_Position: "<< Pan_Position <<  endl;

                      //              std::cout << "Tilt_SpeedPot: " << std::dec <<static_cast<uint16_t>(Tilt_SpeedPot) << endl;
                      //              std::cout <<std::dec << "Tilt_Velocity: "<< Tilt_Velocity <<  endl;
                      //              std::cout <<std::dec << "Tilt_Position: "<< Tilt_Position <<  endl;

                      //              std::cout << "Roll_SpeedPot: " << std::dec <<static_cast<uint16_t>(Roll_SpeedPot) << endl;
                      //              std::cout <<std::dec << "Roll_Velocity: "<< Roll_Velocity <<  endl;
                      //              std::cout <<std::dec << "Roll_Position: "<< Roll_Position <<  endl;

                                     std::cout << "Direction_Switches: " << std::dec <<static_cast<uint16_t>(Direction_Switches) << endl;
                      //               std::cout << "Auxilliary_Switches: " << std::dec <<static_cast<uint16_t>(Auxilliary_Switches) << endl;



                      //              std::cout << "\n" << endl;
                  }
#endif

#if __clang__
                if (__builtin_bswap16(temp)==*(uint16_t*)&buffer[34])
                {
                   // temp = *(uint16_t*)&buffer[2];
                    uint16_t Timestamp = __builtin_bswap16(*(uint16_t*)&buffer[2]);
                   // URHO3D_LOGINFO("Timestamp: " + Urho3D::String(Timestamp));
                    //                  std::cout << "Timestamp as is: " << std::dec << temp << "  " << std::hex << temp <<  endl;
                    //                  std::cout << "Timestamp: " << std::dec << time << "  " << std::hex << time <<  endl;
                    //nFrames++;


                    uint8_t Pan_SpeedPot = (uint8_t) buffer[4];
                    int16_t Pan_Velocity = __builtin_bswap16(*(uint16_t*)&buffer[5]);
                    int32_t Pan_Position = (int32_t) __builtin_bswap32(*(uint32_t*)&buffer[7]);

                    uint8_t Tilt_SpeedPot = buffer[11];
                    int16_t Tilt_Velocity = __builtin_bswap16(*(uint16_t*)&buffer[12]);
                    int32_t Tilt_Position = (int32_t) __builtin_bswap32(*(uint32_t*)&buffer[14]);

                    uint8_t Roll_SpeedPot = buffer[18];
                    int16_t Roll_Velocity = __builtin_bswap16(*(uint16_t*)&buffer[19]);
                    int32_t Roll_Position = (int32_t) __builtin_bswap32(*(uint32_t*)&buffer[21]);

                    uint8_t Direction_Switches = buffer[25];
                    temp = Direction_Switches & 0x0003;
                    if (temp==0x0001)
                        Direction_Switches_Pan = 1.0f;
                    else {
                        if (temp==0x0002)
                            Direction_Switches_Pan = -1.0f;
                        else
                            Direction_Switches_Pan = 0.0f;
                    }

                    temp = Direction_Switches & (0x0003<<2);
                    if (temp==0x0004)
                        Direction_Switches_Tilt = 1.0f;
                    else {
                        if (temp==0x0008)
                            Direction_Switches_Tilt = -1.0f;
                        else
                            Direction_Switches_Tilt = 0.0f;
                    }

                    temp = Direction_Switches & (0x0030);
                    if (temp==0x0010)
                        Direction_Switches_Roll = 1.0f;
                    else {
                        if (temp==0x0020)
                            Direction_Switches_Roll = -1.0f;
                        else
                            Direction_Switches_Roll = 0.0f;
                    }


                    uint8_t Auxilliary_Switches = buffer[26];

                    newFrame = true;


                }
#endif
                  memmove(buffer, &buffer[36], length-36); // -1 ?

                  length = length - 36;
               }

          }
          else
          {
              // std::memmove(&buffer[0], &buffer[i], result.length()-i); // -1 ?
              // length = 0;

              bString = std::string(1,'0');
              resBuffer->at(k) = bString;

              length = 0;
          }


      }

      my_sleep(7);
  }

  auto end = std::chrono::system_clock::now();

  std::chrono::duration<double> elapsed_seconds = end-start;

  std::cout << "number frames: " << nFrames << "\n";


  std::cout << "elapsed time: " << elapsed_seconds.count() << "s\n";



//  for(int i=0; i<resString.size(); ++i)
//    {
//    std::cout << std::dec <<  i << ' ' << string_to_hex2(resString[i]) << " " <<std::dec << "Size: " << resString[i].length() << endl;
//    std::cout << "Found 0xAA0x55 at: " << find_frame3(resString[i])<< endl;
//    std::cout << std::dec <<  i << ' ' << string_to_hex2(resBuffer->at(i)) << endl;
//     std::cout << "\n" << endl;
//  }

  delete resBuffer;

  return 0;
}

int main(int argc, char **argv) {
  try {
    return run(argc, argv);
  } catch (exception &e) {
    cerr << "Unhandled Exception: " << e.what() << endl;
  }
}
