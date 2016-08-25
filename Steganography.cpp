#include <iostream>    
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <cctype>
#include <string>
#include <fstream>

using namespace std;

// Globals
const char *DAY = "20";
const char *MONTH = "December";
const char *YEAR = "2013";
const char *AUTHOR = "Jeremiah Bergkvist";
const char *LICENSE = "MIT";
const char *GROUP = "Jeremiah Bergkvist";
const char *CONTACT = "jeremiah.bergkvist@gmail.com";

// Structs
typedef struct Arguments
{
    int numArgs;        /* number of arguments */
    string name;        /* name of the program */
    bool encode;        /* -e option */
    bool decode;        /* -d option */
    bool verbosity;     /* -v option */
    bool help;          /* -h or -? option */
    string inputFile;    /* input file */
    string outputFile;   /* output file */
    string dataFile;     /* embed files */
} Arguments;

#pragma pack(1)
typedef struct BitmapFileHeader
{
    unsigned short bfType;
    unsigned int   bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int   bfOffBits;        /* Offset */
} BitmapFileHeader;

typedef struct BitmapInfoHeader
{
    unsigned int   biSize;         /* Size of info header */
    int            biWidth;         /* Width of image */
    int            biHeight;        /* Height of image */
    unsigned short biPlanes;       /* Number of color planes */
    unsigned short biBitCount;     /* Number of bits per pixel */
    unsigned int   biCompression;  /* Type of compression to use */
    unsigned int   biSizeImage;    /* Size of image data */
    int            biXPelsPerMeter; /* X pixels per meter */
    int            biYPelsPerMeter; /* Y pixels per meter */
    unsigned int   biClrUsed;      /* Number of colors used */
    unsigned int   biClrImportant; /* Number of important colors */
} BitmapInfoHeader;
#pragma pack()

// Classes
class Data{
public:
    // Constructor
    Data(){
    }
    // Destructor
    ~Data(){
    }
    bool LoadData( string file )
    {
        ifstream inFile;
        
        // Get File Size
        stat( file.c_str(), &this->fileStatus );
        
        // Set name of File
        this->fileName = file;
        
        // Open file
        inFile.open(file.c_str(), ios::in|ios::binary);

        if( inFile.is_open() ){
            // Load BMP Data
            this->data = new char[this->fileStatus.st_size +1];
            inFile.read( this->data, this->fileStatus.st_size );
            if( inFile.fail() ){
                return false;
            }

            // Close BMP file
            inFile.close();
            
            return true;
        }
        else{
            return false;
        }
    }
    string GetFileName(void)
    {
        return(this->fileName);
    }
    size_t GetSize(void)
    {
        return(this->fileStatus.st_size);
    }
    char *GetData(void)
    {
        return(this->data);
    }
private:
    string fileName;
    struct stat fileStatus;
    char *data;
};

class BMP
{
public:
    // Constructor
    BMP()
    {
        this->headerSize = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);
    }
    
    // Destructor
    ~BMP()
    {
    }

    // Functions
    bool ExtractData( string outFileName )
    {
        // Compute number of bytes hidden between headers and image data
        this->dataSize = this->bmpFile.bfOffBits - this->headerSize;
        
        // Either bad BMP or no data hidden
        if(dataSize < 1){
            return false;
        }
        
        // Something Hidden
        ofstream outFile;
        
        // Write the data out to a new file
        outFile.open(outFileName.c_str(), ios::out | ios::binary);
        
        if(outFile.is_open() ){
            outFile.write( this->data, this->dataSize );
            if(outFile.fail() ){
                return false;
            }
            outFile.close();
        }
        else{
            return false;
        }
        
        return true;
    }
    
    bool InsertData( Data *data, string outFileName )
    {
        ofstream outFile;
        
        // Change the file size to include the message size
        this->bmpFile.bfSize += data->GetSize();
        
        // Change the image data offset to start after the message
        this->bmpFile.bfOffBits += data->GetSize();
        
        // Write the data out to a new file
        outFile.open(outFileName.c_str(), ios::out | ios::binary);

        if( outFile.is_open() ){
            // Save modified BMP File Header
            outFile.write( reinterpret_cast<char*>(&this->bmpFile), sizeof(BitmapFileHeader) );
            if( outFile.fail() ){
                return false;
            }
            
            // Save modified BMP File Header
            outFile.write( reinterpret_cast<char*>(&this->bmpInfo), sizeof(BitmapInfoHeader) );
            if( outFile.fail() ){
                return false;
            }
            
            // Save message to file
            outFile.write( data->GetData(), data->GetSize());
            if( outFile.fail() ){
                return false;
            }
            
            // Save BMP Image Data
            outFile.write( this->data, this->fileStatus.st_size - sizeof(BitmapInfoHeader) - sizeof(BitmapFileHeader) );
            if( outFile.fail() ){
                return false;
            }

            // Close BMP file
            outFile.close();
            
            return true;
        }
        else{
            return false;
        }
        
        return true;
    }

    bool LoadBmp( string file )
    {
        ifstream inFile;
        
        // Get File Size
        stat( file.c_str(), &this->fileStatus );
        
        // Set name of File
        this->fileName = file;
        
        // Open file
        inFile.open(file.c_str(), ios::in|ios::binary);

        if( inFile.is_open() ){
            // Load BMP File Header
            inFile.read( (char *) &this->bmpFile, sizeof(BitmapFileHeader) );
            if( inFile.fail() ){
                return false;
            }
            
            // Load BMP Info Header
            inFile.read( (char *) &this->bmpInfo, sizeof(BitmapInfoHeader) );
            if( inFile.fail() ){
                return false;
            }
            
            // Load BMP Data
            this->dataSize = this->fileStatus.st_size - this->headerSize;
            this->data = new char[this->dataSize];
            inFile.read( this->data, this->dataSize );
            if( inFile.fail() ){
                return false;
            }
            
            // Close BMP file
            inFile.close();
            
            return true;
        }
        else{
            return false;
        }
    }
private:
    string fileName;
    struct stat fileStatus;
    BitmapFileHeader bmpFile;
    BitmapInfoHeader bmpInfo;
    unsigned int headerSize;
    char *data;
    unsigned int dataSize;
};

// Prototypes
void display_usage( struct Arguments *args );

// Main
int main (int argc, char **argv)
{
    static const struct option longOpts[] = {
        { "help", no_argument, NULL, 'h' },
        { "verbose", no_argument, NULL, 'v' },
        { "embed",  no_argument, NULL, 'e' },
        { "extract",  no_argument, NULL, 'x' },
        { "help", no_argument, NULL, 'h' },
        { "in", required_argument, NULL, 'i' },
        { "out", required_argument, NULL, 'o' },
        { "data", required_argument, NULL, 'd' },
        { NULL, no_argument, NULL, 0 }
    };

    struct Arguments args = {};
    int opt = 0;
    int longIndex = 0;
    BMP image;
    Data dataFile;

    args.numArgs = argc;
    args.name = argv[0];
    while( (opt = getopt_long( argc, argv, "vexh?i:o:d:", longOpts, &longIndex )) != -1 ) {
        switch( opt )
        {
            case 'i':
                args.inputFile = (optarg);
                break;            
            case 'o':
                args.outputFile = (optarg);
                break;
            case 'd':
                args.dataFile = (optarg);
                break;
            case 'e':
                args.encode = true;
                break;
            case 'x':
                args.decode = true;
                break;
            case 'v':
                args.verbosity = true;
                break;
                
            case 'h':
            case '?':
            default:
                args.help = true;
                break;
        }// end switch
    }// end arg parse loop

    // Error if anything is left on the cmdline
    longIndex = optind;
    if( longIndex < argc ){
        cout << "Error: Invalid argument(s):\n";
        args.help = true;
        for (; longIndex < argc; longIndex++){
            cout << "    " << argv[longIndex] << "\n";
        }
    }

    // Ensure all necessary switches have a value if set
    if( args.encode && args.decode ){
        args.help = true;
    }
    // Encode a file
    else if( args.encode && !args.help){
        if( args.inputFile == "" && args.outputFile != "" && args.dataFile == "" ){
            cout << "Encoding requires -i, -o and -d\n";
            args.help = true;
        }
    }
    // Decode a file
    else if( args.decode && !args.help){
        if( args.inputFile == "" && args.outputFile == "" ){
            cout << "Decode requires an input file (-i) and a destination (-d)\n";
            cout << "for the secret message, -o\n";
            args.help = true;
        }
    }
    // Invalid options
    else{
        cout << "Must encode or extract data to/from a file\n";
        args.help = true;
    }

    // Check to see if help menu is to be displayed
    if( args.help ){
        display_usage(&args);
        exit(1);
    }
    
    // Encode the data into the BMP
    if( args.encode ){
        // Load input file from HDD
        if( image.LoadBmp(args.inputFile) == false ){
            cerr << "Error loading " << args.inputFile << "\n";
            exit(1);
        }
        
        // Load message file from HDD
        if( dataFile.LoadData(args.dataFile) == false){
            cerr << "Error loading " << args.dataFile << "\n";
            exit(1);
        }
        
        // Save the new image with the message to the HDD
        if( image.InsertData(&dataFile, args.outputFile ) == false ){
            cerr << "Error saving " << args.outputFile << "\n";
            exit(1);
        }
    }
    
    // Decode the data from the BMP
    else if( args.decode ){
        // Load input file from HDD
        if( image.LoadBmp(args.inputFile) == false ){
            cerr << "Error loading " << args.inputFile << "\n";
            exit(1);
        }
        
        if( image.ExtractData( args.outputFile ) == false ){
            cerr << "Error extracting data to " << args.outputFile << "\n";
            exit(1);
        }
    }
    exit (0);
}


// Usage
void display_usage( struct Arguments *args )
{
    cout << "\nUsage:\n";
    cout << args->name << " [-e|-x] [-i <in.bmp>] [-o <out.bmp> | -d <data>]\n";
    cout << "\t-h --help      This menu\n";
    cout << "\t-v --verbose   Extra information during embedding process\n";
    cout << "\t-e --embed     Indicate that an embed operation must occur\n";
    cout << "\t-x --extract   Extract embedded information\n";
    cout << "\t-i --in        Bitmap to hide data in\n";
    cout << "\t-o --out       Resulting bitmap save name\n";
    cout << "\t-d --data      Data file to hide\n\n";
    cout << "Examples:\n";
    cout << "\t" << args->name << " -e -i original.bmp -o secret.bmp -d data.txt\n";
    cout << "\t" << args->name << " -x -i original.bmp -o secretdata.txt\n\n";

     // spit out copyright / license information
    cout  << "\n Created on " << DAY << " " << MONTH << " " << YEAR << " by " << AUTHOR << ".\n"
        << " Licensed under " << LICENSE << " by " << GROUP << ".\n"
        << " Copyright (c) " << YEAR << " " << GROUP << "\n"
        << " Contact: " << CONTACT << "\n\n";
} 
       
/*
cout << "Encode : " << args.encode << endl;
cout << "Decode : " << args.decode << endl;
cout << "Verbose: " << args.verbosity << endl;
cout << "In     : " << args.inputFile << endl;
cout << "Out    : " << args.outputFile << endl;
cout << "Data   : " << args.embedFile << endl;
cout << endl;


 // check to see if it's time to encode or decode
 if( EncodeMode )
 {
  // open the input BMP file
  
  BMP Image;
  Image.ReadFromFile( argv[3] );
  int MaxNumberOfPixels = Image.TellWidth() * Image.TellHeight() - 2;
  int k=1;
  
  // set image to 32 bpp
  
  Image.SetBitDepth( 32 );
  
  // open the input text file
  
  FILE* fp;
  fp = fopen( argv[2] , "rb" );
  if( !fp )
  {
   cout << "Error: unable to read file " << argv[2] << " for text input!" << endl;
   return -1; 
  }

  // figure out what we need to encode as an internal header
  
  EasyBMPstegoInternalHeader IH;
  IH.InitializeFromFile( argv[2] , Image.TellWidth() , Image.TellHeight() );
  if( IH.FileNameSize == 0 || IH.NumberOfCharsToEncode == 0 )
  { return -1; }
  
  // write the internal header;
  k=0;
  int i=0;
  int j=0;
  while( !feof( fp ) && k < IH.NumberOfCharsToEncode )
  {
   // decompose the character 
   
   unsigned int T = (unsigned int) IH.CharsToEncode[k];
   
   int R1 = T % 2;
   T = (T-R1)/2;
   int G1 = T % 2;
   T = (T-G1)/2;
   int B1 = T % 2;
   T = (T-B1)/2;
   int A1 = T % 2;
   T = (T-A1)/2;
   
   int R2 = T % 2;
   T = (T-R2)/2;
   int G2 = T % 2;
   T = (T-G2)/2;
   int B2 = T % 2;
   T = (T-B2)/2;
   int A2 = T % 2;
   T = (T-A2)/2;

   RGBApixel Pixel1 = *Image(i,j);
   Pixel1.Red += ( -Pixel1.Red%2 + R1 );
   Pixel1.Green += ( -Pixel1.Green%2 + G1 );
   Pixel1.Blue += ( -Pixel1.Blue%2 + B1 );
   Pixel1.Alpha += ( -Pixel1.Alpha%2 + A1 );
   *Image(i,j) = Pixel1;
   
   i++; 
   if( i== Image.TellWidth() ){ i=0; j++; }
   
   RGBApixel Pixel2 = *Image(i,j);
   Pixel2.Red += ( -Pixel2.Red%2 + R2 );
   Pixel2.Green += ( -Pixel2.Green%2 + G2 );
   Pixel2.Blue += ( -Pixel2.Blue%2 + B2 );
   Pixel2.Alpha += ( -Pixel2.Alpha%2 + A2 );
   *Image(i,j) = Pixel2;
   
   i++; k++;
   if( i== Image.TellWidth() ){ i=0; j++; }
  }

  // encode the actual data 
  k=0;
  while( !feof( fp ) && k < 2*IH.FileSize )
  {
   char c;
   fread( &c , 1, 1, fp );

   // decompose the character 
   
   unsigned int T = (unsigned int) c;
   
   int R1 = T % 2;
   T = (T-R1)/2;
   int G1 = T % 2;
   T = (T-G1)/2;
   int B1 = T % 2;
   T = (T-B1)/2;
   int A1 = T % 2;
   T = (T-A1)/2;
   
   int R2 = T % 2;
   T = (T-R2)/2;
   int G2 = T % 2;
   T = (T-G2)/2;
   int B2 = T % 2;
   T = (T-B2)/2;
   int A2 = T % 2;
   T = (T-A2)/2;

   RGBApixel Pixel1 = *Image(i,j);
   Pixel1.Red += ( -Pixel1.Red%2 + R1 );
   Pixel1.Green += ( -Pixel1.Green%2 + G1 );
   Pixel1.Blue += ( -Pixel1.Blue%2 + B1 );
   Pixel1.Alpha += ( -Pixel1.Alpha%2 + A1 );
   *Image(i,j) = Pixel1;
   
   i++; k++;
   if( i== Image.TellWidth() ){ i=0; j++; }
   
   RGBApixel Pixel2 = *Image(i,j);
   Pixel2.Red += ( -Pixel2.Red%2 + R2 );
   Pixel2.Green += ( -Pixel2.Green%2 + G2 );
   Pixel2.Blue += ( -Pixel2.Blue%2 + B2 );
   Pixel2.Alpha += ( -Pixel2.Alpha%2 + A2 );
   *Image(i,j) = Pixel2;
   
   
   i++; k++;
   if( i== Image.TellWidth() ){ i=0; j++; }
  }
  
  fclose( fp );
  Image.WriteToFile( argv[4] );
  
  return 0;
 }
 
 if( DecodeMode )
 {
  BMP Image;
 
  Image.ReadFromFile( argv[2] );
  if( Image.TellBitDepth() != 32 )
  {
   cout << "Error: File " << argv[2] << " not encoded with this program." << endl;
   return 1;
  }
  
  EasyBMPstegoInternalHeader IH;
  IH.InitializeFromImage( Image );
  if( IH.FileSize == 0 || IH.FileNameSize == 0 || IH.NumberOfCharsToEncode == 0 )
  {
   cout << "No hiddent data detected. Exiting ... " << endl;
   return -1;
  }

  cout << "Hidden data detected! Outputting to file " << IH.FileName << " ... " << endl;
  
  FILE* fp;
  fp = fopen( IH.FileName , "wb" );
  if( !fp )
  {
   cout << "Error: Unable to open file " << IH.FileName << " for output!\n";
   return -1;
  }
  
  int MaxNumberOfPixels = Image.TellWidth() * Image.TellHeight();
  
  int k=0;
  int i=0;
  int j=0;

  // set the starting pixel to skip the internal header
  i = 2*IH.NumberOfCharsToEncode;
  while( i >= Image.TellWidth() )
  {
   i -= Image.TellWidth(); j++;
  }

  while( k < 2*IH.FileSize )
  {
   // read the two pixels
   
   RGBApixel Pixel1 = *Image(i,j);
   i++; k++;
   if( i == Image.TellWidth() ){ i=0; j++; }
   
   RGBApixel Pixel2 = *Image(i,j);
   i++; k++;
   if( i == Image.TellWidth() ){ i=0; j++; }
   
   // convert the two pixels to a character
   
   unsigned int T = 0;
   T += (Pixel1.Red%2);
   T += (2* (Pixel1.Green%2));
   T += (4* (Pixel1.Blue%2));
   T += (8* (Pixel1.Alpha%2));
   
   T += (16*  (Pixel2.Red%2));
   T += (32*  (Pixel2.Green%2));
   T += (64*  (Pixel2.Blue%2));
   T += (128* (Pixel2.Alpha%2));
   
   char c = (char) T;
   
   fwrite( &c , 1 , 1 , fp ); 
  }
  fclose( fp ); 
  return 0;
 }
    */
