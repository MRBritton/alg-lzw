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
      // Add wc to the dictionary. Assuming the size is 4096!!!
      if (dictionary.size()<4096)
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
  //std::cout << result<<"???:::\n";
  std::string entry;
  for ( ; begin != end; begin++) {
    int k = *begin;
    if (dictionary.count(k))
      entry = dictionary[k];
    else if (k == dictSize)
      entry = w + w[0];
    else
      throw "Bad compressed k";
 
    result += entry;
 
    // Add w+entry[0] to the dictionary.
    if (dictionary.size()<4096)
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
		bits = 12;

		//Encode the value in binary
		p = int2BinaryString(*it, bits);
		bcode += p;
		++words_written;
	}

	filename += ".lzw";
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

	std::string zeros = "00000000";
	
	struct stat filestatus;
	stat(filename.c_str(), &filestatus);
	long fsize = filestatus.st_size; //file size in bytes

	char c[fsize];
	infile.read(c, fsize);

	std::string s = "";
	long count = 0;
	while(count < fsize) {
		unsigned char uc = (unsigned char) c[count];
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
	//here n = 12
	int bits = 12;

	//If the string has right-padded 0s, remove them
	if(s.size() % bits != 0) {
		s = std::string(s.data(), (s.size() / bits) * bits);
	}

	std::vector<int> code;

	for(int i = 0; i < s.length(); i += bits) {
		code.push_back(binaryString2Int(s.substr(i, bits)));
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
		std::cout << "Compressing...\n";
		compress(std::string(content, size), std::back_inserter(compressed));

		binaryWrite(compressed, filename);
		std::cout << "Compressed file written to " << filename << ".lzw\n";

		delete content;
	}

	else if(mode == "e" || mode == "E") {
		std::vector<int> code = binaryRead(filename);
		std::cout << "Decompressing...\n";
		std::string decompressed = decompress(code.begin(), code.end());
		//chop off the .lzw
		filename = filename.substr(0, filename.find_last_of("."));
		//insert a 2
    	auto ext = filename.find_first_of(".");
    	if(ext != std::string::npos) {
		  filename.insert(filename.find_first_of("."), "2");
    	}
    	else {
      		filename += "2";
    	}

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