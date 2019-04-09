
#define OUT_BUFF_STATUS 0x01
#define IN_BUF_STATUS 0x02


char asccode[58][2] =       /* Array containing ascii codes for
			       appropriate scan codes */
     {
       {   0,0   } ,
       { 0,0 } ,
       { '1','!' } ,
       { '2','@' } ,
       { '3','#' } ,
       { '4','$' } ,
       { '5','%' } ,
       { '6','^' } ,
       { '7','&' } ,
       { '8','*' } ,
       { '9','(' } ,
       { '0',')' } ,
       { '-','_' } ,
       { '=','+' } ,
       {   8,8   } ,
       {   9,9   } ,
       { 'q','Q' } ,
       { 'w','W' } ,
       { 'e','E' } ,
       { 'r','R' } ,
       { 't','T' } ,
       { 'y','Y' } ,
       { 'u','U' } ,
       { 'i','I' } ,
       { 'o','O' } ,
       { 'p','P' } ,
       { '[','{' } ,
       { ']','}' } ,
       {  10,10  } ,
       {   0,0   } ,
       { 'a','A' } ,
       { 's','S' } ,
       { 'd','D' } ,
       { 'f','F' } ,
       { 'g','G' } ,
       { 'h','H' } ,
       { 'j','J' } ,
       { 'k','K' } ,
       { 'l','L' } ,
       { ';',':' } ,
       {  39,34  } ,
       { '`','~' } ,
       {   0,0   } ,
       { '\\','|'} ,
       { 'z','Z' } ,
       { 'x','X' } ,
       { 'c','C' } ,
       { 'v','V' } ,
       { 'b','B' } ,
       { 'n','N' } ,
       { 'm','M' } ,
       { ',','<' } ,
       { '.','>' } ,
       { '/','?' } ,
       {   0,0   } ,
       {   0,0   } ,
       {   0,0   } ,
       { ' ',' ' } ,
   };

char getStatus()
{
    char status;
    asm volatile ("in $0x64, %0" : "=a" (status));
    return status;
}

void sendCommand(char command)
{
    while(getStatus() & IN_BUF_STATUS); //wait until ready for input
    asm volatile ("out %0, $0x64" :: "a" (command));
}

char readData()
{
    while(!(getStatus() & OUT_BUFF_STATUS)); //wait for data to be ready
    char data;
    asm volatile ("in $0x60, %0" : "=a" (data));
    return data;
}

void writeData(char data)
{
    while(getStatus() & IN_BUF_STATUS); //wait until ready for input
    asm volatile ("out %0, $0x60" :: "a" (data));
}

void initializeKeyboard()
{
    /*
    Disable port 1 (see the command table in the previous link)
    Disable port 2
    Read byte 0 from RAM (current configuration)
    Modify the bits in the configuration so the first clock and first interrupt are enabled, but the second clk + interrupt are not.
    Write the configuration byte back to the controller. Note that you will need to poll the status bit to determine when it is safe to write the configuration byte.
    */
    char resp;

    sendCommand(0xAD);
    sendCommand(0xA7);
    sendCommand(0x20);
    char config = readData();

    config |= 0x11; //turn on first ps/2 port
    config &= (~0x22); //turn off second ps/2 port

    sendCommand(0x60);
    writeData(config);


    //reset keyboard
    writeData(0xFF);
    resp = readData(); //TODO receive response

    //set scan set 1
    writeData(0xF0);
    writeData(0x01);
    resp = readData(); //TODO receive ACK

    //turn on scan codes
    writeData(0xF4);
    resp = readData(); //TODO receive ACK
    while(resp == 0xFE) resp = readData();
}

unsigned char getKey()
{
    unsigned int code;
    unsigned char c;
    do{
        while((code = readData()) >= 58);
        c = asccode[code][0];
    }while(!c);
    return c;
}