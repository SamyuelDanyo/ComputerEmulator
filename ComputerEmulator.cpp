//
//  ComputerEmulator.cpp
//      Implements caching
//  Created by Samyuel Danyo on 16/04/2017.
//  Copyright © 2017 Samyuel Danyo. All rights reserved.
//
#include <iostream>
#include <stdint.h>
#include <cstdlib>
//COMPILE USING /Ddebug FOR DEBUG MODE
#ifndef debug
#define debug 0
#endif

namespace Emulator
{
	using namespace std;
	
	class computer												
	{
		public:
		
		uint8_t max_program_address;
		uint8_t min_data_adress;
		uint8_t max_data_address;
		static const uint16_t max_data_cap = 65535;
		
		enum op_Codes      //Operation codes
		{
			LOAD_A = 0,	   //Load from an address
			LOAD_N0,	   //Load the more significant byte of acc with a number
			LOAD_N1,	   //Load the less significant byte of acc with a number
			STORE,		   //Store the accumulator result in an adress	
			ADD_A,		   //ADD data from an adress
			ADD_N,		   //Add a number
			SUB_A,		   //Substract data from an adress	
			JUMP,		   //Jump to an instruction
			SUB_N,		   //Substract a number
			PROCESS,	   //Stop processing, shut down
			DN,			   //Do nothing ofr a cycle
			RESET,		   //Resetting the program from beginning
			RELOAD		   //Reloading the program on the memory
		};
		//Constructor
		computer()
		{
			max_program_address = 127;
			min_data_adress = 128;
			max_data_address = 255;
			
			executing_program = 0;
		}
		//Deconstructor
		~computer()
		{}
		
		private:
		
		uint16_t IR;				//Instruction register
		uint8_t PC;					//Program Counter
		uint16_t ACC;				//Accumulator
		uint16_t RAM[256];			//Ram, 256 adresses orginized in 64x4 blocks
		uint16_t D_CACHE[10][5];	//Cache memory saving 10 data blocks
		uint16_t I_CACHE[10][5];	//Cache memory saving 10 instruction blocks
		bool processing;
		bool overflow;
		bool underflow;
		int temp;
		uint8_t D_cache_counter;
		uint8_t I_cache_counter;
		int executing_program;
		
		void reset_computer()		//Clearing the registers and setting the flags to a valid state
		{
			ACC=0;
			PC=0;
			IR=0;
			temp=0;
			D_cache_counter=0;
			I_cache_counter=0;
			overflow = false;
			underflow = false;
			processing = true;
			if(debug)
				cout<<"Resetting done! Ready for use."<<endl;
		}
		//In case of not recognised instruction
		void error()
		{
			cout<<"Error in processing of instruction: "<<endl;
			display_reg();
		}
		
		//Fetch for instruction cycle
		void fetch()
		{
			IR=load_instruction(PC);
		}
		//Load from an address function
		void load_a()
		{
			ACC=load_data(IR&0xFF);
			if(debug)
			{
				display_reg();
				display_mem();
			}
			//Program counter incrementing
			PC++;
		}
		//Load the less significant byte of acc with a number func
		void load_n1()
		{
			temp=(ACC&0xFF00);
			ACC=IR&0xFF+temp;
			if(debug)
			{
				display_reg();
			}
			PC++;
		}
		//Load the more significant byte of acc with a number func
		void load_n0()
		{
			temp=(ACC&0xFF);
			ACC=(IR&0xFF)*256+temp;
			if(debug)
			{
				display_reg();
			}
			PC++;
		}
		//Store the accumulator result in an adress func
		void store()
		{
			store_data_instruction(ACC);
			if(debug)
			{
				display_reg();
				display_mem();
			}
			PC++;
		}
		//ADD data from an adress func
		void add_a()
		{
			//Verification of a valid data adress
			if((IR&0xFF)<=max_program_address)
			{
				cout<<"INVALID DATA ADRESS!!"<<endl;
				display_reg();
				process();
				//If not a valid adress-shut down, it can be extended to try to reload ad than try again
				return;
			}//in case the program has overwritten its code and made a mistake
			//Sum of two, two-byte numbers. The more significant bytes are multiplied by 256=2^8
			temp=ACC+load_data(IR&0xFF);
			//Check for overflow
			if(temp>max_data_cap)
			{
				//Masking off the more significant byte in IR (OP) -> getting the adress only
				overflow=true;
				if(debug)
				{
					display_reg();
					display_mem();
				}
				ACC=max_data_cap;
			}else
			{
				ACC=temp;						
				if(debug)
				{
					display_reg();
					display_mem();
				}
			}

			PC++;
		}
		//Add a number func
		void add_n()
		{
			temp=ACC+(IR&0xFF);
			if(temp>max_data_cap)
			{
				overflow=true;
				if(debug)
					display_reg();
				ACC=max_data_cap;
			}else 
			{
				ACC=temp;
				if(debug)
					display_reg();
			}
			PC++;
		}
		//Substract data from an adress func
		void sub_a()
		{
			//Verification of a valid data adress
			if((IR&0xFF)<=max_program_address)
			{
				cout<<"INVALID DATA ADRESS!!"<<endl;
				display_reg();
				//If not a valid adress-shut down, it can be extended to try to reload ad than try again
				process();
				//in case the program has overwritten its code and made a mistake
				return;
			}
			temp=ACC-load_data(IR&0xFF);
			if(temp<0)  //Underflow check
			{
				underflow=true;	//Shut down on end of cycle
				if(debug)
				{
					display_reg();
					display_mem();
				}
				ACC=0;
			}else 
			{
				ACC=temp;
				if(debug)
				{
					display_reg();
					display_mem();
				}
			}
				PC++;
		}
		//Substract a number func
		void sub_n()
		{
			temp=ACC-(IR&0xFF);
			if(temp<0)
			{
				underflow=true;
				if(debug)
					display_reg();
				ACC=0;			
			}else
				{
					ACC=temp;
					if(debug)
						display_reg();
				}
				PC++;
		}
		//Jump to an instruction func
		void jump()
		{
			//Verification of a valid program address
			if((IR&0xFF)>max_program_address)
			{
				cout<<"Jump to unvalid adress"<<endl;
				display_reg();
				process();
				return;
			}
			if(debug)
				display_reg();
			PC=(IR&0xFF);
		}
		//Stop processing, shut down func
		void process()
		{
			processing=false;
			//Instead of writing in every function/execution, the PC shouldn;t increment, when
			if(overflow||underflow||(PC>max_program_address))
				// one of these happens I take care of it here for all of them.
				PC--;
			if(debug)
				display_reg();
			cout<<"COMPUTING STOPPED"<<endl;
		}
		//Do nothing for a cycle func NOP
		void do_nothing()
		{
			if(debug)
				display_reg();
			PC++;
		}
		//Resetting the program from beginning func
		void reset()
		{
			
			if(debug)
				display_reg();
			reset_computer();
		
		}
		
		void initialise_cache_unused_ram()
		{
			for(int i=min_data_adress;i<256;i++)							//Initialising the rest of the memory to 0;
			{
				RAM[i]=NULL;
			}
			for(int i=0;i<10;i++)
				for(int k=0;k<5;k++)
				{
					D_CACHE[i][k]=max_data_cap;			//initilizing to the max value - since it is unvalid for an address
					I_CACHE[i][k]=max_data_cap;			//In c++ NULL=0 -- soo yea...
				}			
		}
		
		
		void reload()											//Reloading the program on the memory func
		{
			load_program(executing_program);
			if(debug)
				display_reg();
			PC++;
		}
		
		
		uint16_t load_data(uint16_t address)		//Loading data through the cache memory
		{
			for(int i=0;i<10;i++)
			{
				if((address/4)==D_CACHE[i][0])		//Searching for the tag of mem block of the address
				{
					if(debug)
						cout<<"Loaded data from cache."<<endl;
					return D_CACHE[i][address-(address/4)*4+1];	//If found, the data of the address is returned
				}								//I find the right address thorugh (mem block tag)x4=to the first address 
			}									//in the mem block, then adding 1-cos space [0] is for the tag and then subs 
												////from the actual address=the place in the mem block +1=place in the cache line.
			D_CACHE[D_cache_counter][0]=address/4;		//If the mem block is not in the cache, the tag is saved in the chosen line
			for(int i=1;i<5;i++)						//I rotate the cache lines on last updated.
			{
				D_CACHE[D_cache_counter][i]=RAM[(address/4)*4+i-1];//After saving the mem tag I fill in the mem block in the cache line
			}
			D_cache_counter++;				//Incrementing the counter
			if(D_cache_counter>9)			//Since my cache has 10 lines, I ress it
				D_cache_counter=0;
			if(debug)
				cout<<"Loaded data from RAM."<<endl;
			return D_CACHE[D_cache_counter-1][address-(address/4)*4+1];	//After adding the new entry, I return it
		}
		
		uint16_t load_instruction(uint8_t address)			//Analogical to the data cache, but for instructions 
		{													//in the instruction cache
			for(int i=0;i<10;i++)
			{
				if((address/4)==I_CACHE[i][0])
				{
					if(debug)
						cout<<endl<<"<<NEW CYCLE>>"<<endl<<"Fetched instrction from cache."<<endl;
					return I_CACHE[i][address-(address/4)*4+1];
				}
			}
			I_CACHE[I_cache_counter][0]=address/4;
			for(int i=1;i<5;i++)
			{
				I_CACHE[I_cache_counter][i]=RAM[(address/4)*4+i-1];
			}
			I_cache_counter++;
			if(I_cache_counter>9)
				I_cache_counter=0;
			if(debug)
				cout<<endl<<"<<NEW CYCLE>>"<<endl<<"Loaded instruction from RAM."<<endl;
			return I_CACHE[I_cache_counter-1][address-(address/4)*4+1];
		}
		
		void store_data_instruction(uint16_t acc)	//On storing, if the address data is in cache, it needs to be updated
		{
			RAM[IR&0xFF]=acc;				//Storing in memory
			for(int i=0;i<10;i++)
			{
				if(((IR&0xFF)/4)==I_CACHE[i][0])		//Searching for the mem block tag
				{
					I_CACHE[i][(IR&0xFF)-((IR&0xFF)/4)*4+1]=acc;	//Analogical to the load_ () but the address is in the less sign byte of IR
					if(debug)
						cout<<"Instruction updated in memory and cache."<<endl;	//When storing new data may be entered-
					return;										//which qould result in storage in RAM
				}												//Data may be updated or an instruction may be overwritten
				if(((IR&0xFF)/4)==D_CACHE[i][0])				//that is why I search tag by tag in both the data and instr caches.
				{
					D_CACHE[i][(IR&0xFF)-((IR&0xFF)/4)*4+1]=acc;
					if(debug)
						cout<<"Data updated in memory and cache."<<endl;
					return;
				}
			}
			if(debug)
				cout<<"Stored in memory."<<endl;
		}
		
		
		uint8_t size_check(int n)		//Making sure the number entred in the program is a valid address/number
		{
			if((0>n)||(n>255))
			{
				cout<<"Unvalid number in an Instruction. Program corrupted!"<<endl;
				process();
				return NULL;
			}
			return (uint8_t)n;
		}
		
		uint16_t enter_instruction(op_Codes op,int n)		//Func, which makes loading the inst, faster and using assembly.
		{									
			size_check(n);
			return op*256+n;
		}
		
		void load_program(int n)									//Test programs - can make them froma file with a machine code or build assembler.
		{	
			switch(n)
			{
				case 1:
				{
					RAM[0]=enter_instruction(LOAD_N1,8);	//The program tests: load from number,			//Load number 8 in less sign. byte
					RAM[1]=enter_instruction(LOAD_N0,7);	//load from address,							//Load 7 in the more significant byte
					RAM[2]=enter_instruction(STORE,22);		//adding, substracting,							//Store the value of acc - 1800 - in data address [22].
					RAM[3]=enter_instruction(LOAD_N0,0);	//storing, program changing
					RAM[4]=enter_instruction(LOAD_N1,32);	//its own code, jumping, skipping
					RAM[5]=enter_instruction(STORE,23);		// a cycle & shuttign down.						//Store the value of the acc - 32 - in data address [23]
					RAM[6]=enter_instruction(LOAD_N1,20);								
					RAM[7]=enter_instruction(STORE,24);												//Store the value of the acc - 20 - in data address [24]
					RAM[8]=enter_instruction(LOAD_A,22);											//Load data from address [22] ->1800
					RAM[9]=enter_instruction(ADD_A,23);									//Add data from adress [23] ->32
					RAM[10]=enter_instruction(SUB_A,24);								//Substract data from address [24] ->20							
					RAM[11]=enter_instruction(STORE,16);								//Store the result - 1812 - in address [16], that overrides the program code
					RAM[12]=enter_instruction(JUMP,16);													
					RAM[13]=enter_instruction(DN,0);
					RAM[14]=enter_instruction(DN,0);								
					RAM[15]=enter_instruction(DN,0);
					RAM[16]=enter_instruction(RESET,0);							//This instruction will not be reset any more but 7=JUMP to address [20]
					RAM[17]=enter_instruction(RESET,0);							//Those are being skipped
					RAM[18]=enter_instruction(RESET,0);
					RAM[19]=enter_instruction(RESET,0);
					RAM[20]=enter_instruction(DN,0);								//Arriving here after the jump, we do nothing for a cylce
					RAM[21]=enter_instruction(PROCESS,0);							//Shuts down the execution							
					max_program_address=21;							//Updating the address intervals for the loaded program
					min_data_adress=22;
					max_data_address=24;
					initialise_cache_unused_ram();
					break;
				}
				case 2:
				{
					RAM[0]=enter_instruction(LOAD_N1,3);		//THIS IS ENDLESS LOOP	//Loading number 3
					RAM[1]=enter_instruction(ADD_N,8);			//NOTE ON TESTING		//Adding number 8
					RAM[2]=enter_instruction(SUB_N,4);		//RESET, RELOAD OPERATION	/Substracting number 4
					RAM[3]=enter_instruction(STORE,8);			//TESTING				//Storing in address[8]
					RAM[4]=enter_instruction(LOAD_A,8);							//Loading the data in address [8] - number 7
					RAM[5]=enter_instruction(RELOAD,0);							//Reloading program
					RAM[6]=enter_instruction(LOAD_A,8);							//Loading the data in address [8] - NULL	
					RAM[7]=enter_instruction(RESET,0);							//Resetting the program from the start - endless loop									
					max_program_address=7;
					min_data_adress=8;
					max_data_address=8;
					initialise_cache_unused_ram();
					break;
				}				
				case 3:
				{
					RAM[0]=enter_instruction(LOAD_N1,255);	//Load from number,			
					RAM[1]=enter_instruction(LOAD_N0,255);		//load from address,	
					RAM[2]=enter_instruction(STORE,5);	//adding, storing and overflow are tested 					
					RAM[3]=enter_instruction(LOAD_A,5);		// 
					RAM[4]=enter_instruction(ADD_N,1);							
					max_program_address=4;
					min_data_adress=5;
					max_data_address=5;
					initialise_cache_unused_ram();
					break;
				}
				case 4:
				{
					RAM[0]=enter_instruction(LOAD_N1,0);	//Load from number,			
					RAM[1]=enter_instruction(LOAD_N0,0);		//load from address,	
					RAM[2]=enter_instruction(STORE,5);	//substacting, storing and underrflow are tested 					
					RAM[3]=enter_instruction(LOAD_A,5);		// 
					RAM[4]=enter_instruction(SUB_N,1);							
					max_program_address=4;
					min_data_adress=5;
					max_data_address=5;
					initialise_cache_unused_ram();
					break;					
				}
				case 5:
				{
					RAM[0]=enter_instruction(LOAD_N1,5);	//Load from number,			
					RAM[1]=enter_instruction(LOAD_N0,3);	//load from address,	
					RAM[2]=enter_instruction(STORE,5);		//adding, storing and normal EOP are tested 					
					RAM[3]=enter_instruction(LOAD_A,5);		
					RAM[4]=enter_instruction(ADD_N,45);							
					max_program_address=4;
					min_data_adress=5;
					max_data_address=5;
					initialise_cache_unused_ram();
					break;
				}
				case 6:
				{
					RAM[0]=enter_instruction(LOAD_N0,300);		//The program tests the size_check func
					max_program_address=1;
					min_data_adress=1;
					max_data_address=1;
					initialise_cache_unused_ram();
					break;
				}
				case 7:
				{
					RAM[0]=enter_instruction(JUMP,10);			//This program tests the check of valid jump address
					max_program_address=0;
					min_data_adress=1;
					max_data_address=1;
					initialise_cache_unused_ram();
					break;
				}
				case 8:
				{
					RAM[0]=enter_instruction(LOAD_N1,5);		//This program tests the valid data adress
					RAM[1]=enter_instruction(SUB_A,2);			//when substracting from an address
					RAM[2]=enter_instruction(DN,0);
					max_program_address=2;
					min_data_adress=3;
					max_data_address=3;
					initialise_cache_unused_ram();
					break;
				}
				case 9:
				{
					RAM[0]=enter_instruction(LOAD_N1,5);		//This program tests the valid data adress
					RAM[1]=enter_instruction(ADD_A,2);			//when adding from an address
					RAM[2]=enter_instruction(DN,0);
					max_program_address=2;
					min_data_adress=3;
					max_data_address=3;
					initialise_cache_unused_ram();
					break;
				}
				default:
				{
					cout<<"There is not such program. Loading aborted."<<endl;
					processing = false;
					return;
				}
				
			}
			if(debug)
				cout<<"Program loaded"<<endl;
		}
		
		void display_reg()
		{
			cout << "<<<CPU Registers>>>" << endl
			<< "PC: " << (int)PC << endl
			<< "IR->" <<"OP: "<< (int)((IR&0xFF00)/256) <<" A/N: " << (int)(IR&0xFF) << endl
			<< "ACC: " << (int)ACC << endl
			<< "Temp: " << temp << endl
			<< "Processing: " << processing << endl
			<< "Overflow: " << overflow << endl
			<< "Underflow: " << underflow << endl;
		}
		
		void display_mem()
		{
		cout<<"RAM data on the adress: "<< (int)RAM[IR&0xFF]<<endl;
		}
		public:
		
		void display_ram()
		{
			cout<<"<<RAM>>"<<endl<<"Program Memory"<<endl<<"Addr OP A/N";
			for(int i=0;i<=max_program_address;i++)
			{
				cout<<endl<<i;
				if(i<10)
					cout<<"    ";
				else if(i<100)
					cout<<"   ";
				else cout<<"  ";
					cout<<(int)((RAM[i]&0xFF00)/256)<<"  "<<(RAM[i]&0xFF);
			}
			
			cout<<endl<<"Data Memory"<<endl<<"Addr Value";
			for(int i=min_data_adress;i<=max_data_address;i++)
			{
				cout<<endl<<i;
				if(i<10)
					cout<<"    ";
				else if(i<100)
					cout<<"   ";
				else cout<<"  ";
				cout<<(int)RAM[i];
			}
			cout<<endl<<endl;
		}
		
		void run_computer()
		{
			cout<<endl<<"Which program would you like to run?"<<endl
			<<"1 tests: load from number, load from address, adding, substracting, storing, program changing its code, jumping, do nothing & shuttign down."<<endl
			<<"2 tests: THIS IS ENDLESS LOOP, reset, reload"<<endl
			<<"3 tests: load from number, load from address, adding, storing and overflow"<<endl
			<<"4 tests: load from number, load from address, substacting, storing and underrflow"<<endl
			<<"5 tests: load from number, load from address, adding, storing and normal EOP"<<endl
			<<"6 tests: size_check func"<<endl
			<<"7 tests: check for valid jump address"<<endl
			<<"8 tests: validation of address when substracting from an address"<<endl
			<<"9 tests: vaidation of address when adding from an addres"<<endl;
			cin>>executing_program;
			exe(executing_program);
		}
		
		void exe(int n)
		{
			cout<<endl<<endl<<"Wellcome to Samyuel Danyo's simple computer emulator!" <<endl;
			reset_computer();										//Resseting it - to be ready for processing
			load_program(n);											//After turning on - loading the program
			if(debug)
				cout<<"DEBUG MODE ON"<<endl;
			while(processing)
			{
				fetch();											//Cycle starts with fethcing
				switch((IR&0xFF00)/256)										//Decoding
				{													//Execution
					case LOAD_A:
						load_a();
						break;
					case LOAD_N0:
						load_n0();
						break;
					case LOAD_N1:
						load_n1();
						break;
					case ADD_A:
						add_a();
						break;
					case ADD_N:
						add_n();
						break;
					case SUB_A:
						sub_a();
						break;
					case SUB_N:
						sub_n();
						break;
					case STORE:
						store();
						break;
					case JUMP:
						jump();
						break;
					case PROCESS:
						process();
						break;
					case DN:
						do_nothing();
						break;
					case RESET:
						reset();
						break;
					case RELOAD:
						reload();
						break;
					default:
						error();
						return;
				}
				if(overflow)
				{
					cout<<"Overflow on instrction->"<<"OP: "<< (int)((IR&0xFF00)/256) <<" A/N: " << (int)(IR&0xFF)<<" with adress: " <<(int)PC-1<< endl;
					process();
				}
				if(underflow)
				{
					cout<<"Underflow on instrction->"<<"OP: "<< (int)((IR&0xFF00)/256) <<" A/N: " << (int)(IR&0xFF)<<" with adress: " <<(int)PC-1<< endl;
					process();
				}
				if(PC>max_program_address)								//When the program ends, the cycle is stopped
				{
					cout<<"Program Over"<<endl;
					process();
				}
				
			}
			if(debug)
			{
				display_ram();
			}
		}
		
	};
}

using namespace Emulator;

int main()
{
	computer* comp = new computer();
	comp->run_computer();	
	delete comp;
	std::system("pause");
	return 0;
}