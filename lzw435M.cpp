/* Matthew Britton - mrb182
   Algorithms 435 Project 2 - LZW Compression
   Based on code provided on project page */


#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector> 
#include <sys/stat.h>
#include <stdexcept>
/*
  This code is derived from LZW@RosettaCode for UA CS435 
*/ 
 
// Compress a string to a list of output symbols.
// The result will be written to the output iterator
// starting at "result"; the final iterator is returned.
template <typename Iterator>
Iterator compress(const std::string &uncompressed, Iterator result) {
  // Build the dictionary.
  // Add ASCII
  int dictSize = 256;
  std::map<std::string,int> dictionary;
  for (int i = 0; i < 256; i++)
    dictionary[std::string(1, i)] = i;
 

  std::string w;
  for (std::string::const_iterator it = uncompressed.begin();
       it != uncompressed.end(); ++it) {
    char c = *it;
    std::string wc = w + c;
    if (dictionary.count(wc))
      w = wc;
    else {
      *result++ = dictionary[w];
      // Add wc to the dictionary. Assuming the size is 2^16
      if (dictionary.size()<(1<<16))  //1<<n == 2^n
         dictionary[wc] = dictSize++;
      w = std::string(1, c);
    }
  }
 
  // Output the code for w.
  if (!w.empty())
    *result++ = dictionary[w];
  return result;
}
 
// Decompress a list of output ks to a string.
// "begin" and "end" must form a valid range of ints
template <typename Iterator>
std::string decompress(Iterator begin, Iterator end) {
  // Build the dictionary.
  int dictSize = 256;
  std::map<int,std::string> dictionary;
  for (int i = 0; i < 256; i++)
    dictionary[i] = std::string(1, i);
 
  std::string w(1, *begin++);
  std::string result = w;

  std::string entry;
  for ( ; begin != end; begin++) {
    int k = *begin;
    if (dictionary.count(k))
      entry = dictionary[k];
    else if (k == dictSize)
      entry = w + w[0];
    else {
      throw "Bad compressed k";
    }
 
    result += entry;
 
    // Add w+entry[0] to the dictionary.
    if (dictionary.size()<(1<<16))  //1<<n== 2^n
      dictionary[dictSize++] = w + entry[0];
 
    w = entry;
  }

  return result;

}

std::string int2BinaryString(int c, int cl) {
      std::string p = ""; //a binary code string with code length = cl
      int code = c;

      // shift and encode c in binary
      while (c>0) {         
		   if (c%2==0)
            p="0"+p;
         else
            p="1"+p;
         c=c>>1;   
      }

      int zeros = cl-p.size();
      if (zeros<0) {
         std::cout << "\nWarning: Overflow. code " << code <<" is too big to be coded by " << cl <<" bits!\n";
         p = p.substr(p.size()-cl);
      }
      else {
         for (int i=0; i<zeros; i++)  //pad 0s to left of the binary code if needed
            p = "0" + p;
      }
      return p;
}

int binaryString2Int(std::string p) {
   int code = 0;
   if (p.size()>0) {
      if (p.at(0)=='1') 
         code = 1;
      p = p.substr(1);
      while (p.size()>0) { 
         code = code << 1; 
		   if (p.at(0)=='1')
            code++;
         p = p.substr(1);
      }
   }
   return code;
}


void binaryWrite(std::vector<int>& compressed, std::string filename) {
	int bits = 9; //# of bits to use for encoding to start
	int words_written = 0;  //use this to decide when to switch word length
	std::string p;

	//String to store binary code for compressed sequence
	std::string bcode = "";
	for(auto it = compressed.begin(); it != compressed.end(); ++it) {
		//Find the least power of 2 that encodes the data

		//When we've used up all of our new table space, increase by one bit
		//if we're using 16 bits, stop
		if(words_written == (1 << (bits - 1)) && bits < 16) {  // (1 << b) = 2^b
			++bits;
      words_written = 0;
		}
		

		//Encode the value in binary
		p = int2BinaryString(*it, bits);
		bcode += p;
		++words_written;
	}

	filename += ".lzw2";
	std::ofstream outfile(filename, std::ios::binary);

	std::string zeros = "00000000";
	if(bcode.size() % 8 != 0)  //make sure the length o the binary string is a multiple of 8bits = 1byte
		bcode += zeros.substr(0, 8 - bcode.size() % 8);

	int b;
	for(int i = 0; i < bcode.size(); i += 8) {
		b = 1;
		for(int j = 0; j < 8; ++j) {
			b = b << 1;
			if(bcode.at(i + j) == '1')
				b += 1;
		}
		char c = (char) (b & 255); //save the string byte by byte
		outfile.write(&c, 1);
	}
}


std::vector<int> binaryRead(std::string filename) {
	std::ifstream infile(filename, std::ios::binary);

  if(!infile.is_open()) 
    throw std::runtime_error("could not open file to decompress\n");

	std::string zeros = "00000000";
	
	struct stat filestatus;
	stat(filename.c_str(), &filestatus);
	long fsize = filestatus.st_size; //file size in bytes

	char c[fsize];
	infile.read(c, fsize);

	std::string s = "";  //full binary string file input
	long count = 0;  //count how many bytes have been read

	while(count < fsize) {
		unsigned char uc = (unsigned char) c[count];

    //Convert a byte to a binary string
		std::string p = ""; //binary string
		for(int j = 0; j < 8 && uc > 0; ++j) {
			if(uc % 2 == 0)
				p = "0" + p;
			else
				p = "1" + p;
			uc = uc >> 1;
		}
		p = zeros.substr(0, 8 - p.size()) + p; //pad 0s to the left if needed
		s += p;
		count++;
	}

	//Have a string of binary digits as bytes
	//Need to divide into sets of n bits and decode
	//start reading with 9 bits
	int bits = 9;
  int segments = 0;

  std::vector<int> code;

  // A segment of n-bit length codes occurs at maximum 2^(n-1) times
  for(int current_bit = 0; current_bit < s.size(); current_bit += bits) {

    //check to see if we need to add a bit
    if(bits < 16 && segments == 1 << (bits - 1)) {
      ++bits;
      segments = 0;
    }

    // Check to see if we have a n-bit segment left to read
    // if not, we can just ignore the rest- it's padding
    if(s.size() - current_bit + 1 < bits) {
      break;
    }

    // Construct a string starting from the current bit and taking the appropriate number of bits
    std::string segment(&s[current_bit], bits);
    // Convert the segment to an integer and add it to the code
    code.push_back(binaryString2Int(segment));
    // Note that we've consumed a segment
    ++segments;
  }
	
	return code;
}


char* BlockIO(std::string filename, int& size_out) {
	//Read bytes
   std::ifstream myfile (filename.c_str(), std::ios::binary);

   if(!myfile.is_open())
   		throw std::runtime_error("could not open file");

   std::streampos begin,end;
   begin = myfile.tellg();
   myfile.seekg (0, std::ios::end);
   end = myfile.tellg();
   std::streampos size = end-begin; //size of the file in bytes   
   myfile.seekg (0, std::ios::beg);
   
   //Make sure to also return how many bytes were read
   //Otherwise, if the std::String constructer hits a null terminator,
   // 	it stops construction, causing bugs
   size_out = size;

   char * memblock = new char[size];
   myfile.read (memblock, size); //read the entire file
   memblock[size]='\0'; //add a terminator
   myfile.close();

   return memblock;
}

int main(int argc, char** argv) {

	
	if(argc != 3) {
		std::cout << "Invalid number of arguments.\n";
		return 0;
	}

	std::string mode(argv[1]);
	std::string filename(argv[2]);

	if(mode == "C" || mode == "c") {

		int size;
		char* content = BlockIO(filename, size);

		std::vector<int> compressed;
		compress(std::string(content, size), std::back_inserter(compressed));

		binaryWrite(compressed, filename);
		std::cout << "Compressed file written to " << filename << ".lzw2\n";

		delete content;
	}

	else if(mode == "e" || mode == "E") {
		std::vector<int> code = binaryRead(filename);

		std::string decompressed = decompress(code.begin(), code.end());

		//chop off the .lzw
		filename = filename.substr(0, filename.find_last_of("."));
		//insert a 2M
		filename.insert(filename.find_first_of("."), "2M");

    std::ofstream outfile(filename, std::ios::binary);
    outfile.write(decompressed.c_str(), decompressed.size());
    std::cout << "Decompressed file written to " << filename << std::endl;
	}

	else {
		std::cout << "Unrecognized mode. Use c to compress or e to decompress.\n";
		return 0;
	}
  return 0;
}