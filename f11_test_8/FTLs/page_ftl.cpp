/* Copyright 2011 Matias Bjørling */

/* page_ftl.cpp  */

/* FlashSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version. */

/* FlashSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details. */

/* You should have received a copy of the GNU General Public License
 * along with FlashSim.  If not, see <http://www.gnu.org/licenses/>. */

/****************************************************************************/

/* Implements a very simple page-level FTL without merge */

#include <new>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "../ssd.h"

using namespace ssd;
//////////////////////////////////////////////////////////////////////////

class Pagearr
{
	public:
		enum page_state state;
		char data;
};
class Blockarr
{
	public:
		int invalid_pages;
		int erase_number;
		int free;
		bool is_rs;
};

Pagearr *pages;
Blockarr *Blocks;

int *MT;
//number of last free page that can use for wirte data
int last_free_page;
int free_pages;
int *rs_blocks;
int rs_block_num;
int *arr_max;

void FtlImpl_Page::up_arr_max(int block_num)
{
	int i = 0;
	int tmp;
	
	while(1){	
		if (arr_max[i] == block_num) {
			tmp = arr_max[i];
			break;
		}	
		i ++;
	}
	
	for(int j = i; j > 0; j--)
	{
		arr_max[j] = arr_max[j-1];
	}
	arr_max[0] = tmp;
	
	return;
}

void FtlImpl_Page::down_arr_max(int block_num)
{
	
	for(int i = 0; i < NUMBER_OF_ADDRESSABLE_BLOCKS; i++)
	{
		arr_max[i] = arr_max[i+1];
	}

	arr_max[NUMBER_OF_ADDRESSABLE_BLOCKS] = block_num;
	
	return;
}

int FtlImpl_Page::find_free_page(Event &event)
{
	//-------------------------------------------------------
	if (last_free_page == NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE) {
			last_free_page = 0;
		}
	
	if (pages[last_free_page].state == EMPTY && Blocks[last_free_page / BLOCK_SIZE].is_rs == false) {
		printf("-----------------------yes\n");
		printf("-----------------------%d\n", last_free_page);
		Blocks[last_free_page / BLOCK_SIZE].free --;
		last_free_page ++;
		//printf("--------------------- last_free_page ----  %d\n", last_free_page);
		free_pages --;
		printf("-----------------------%d\n", last_free_page);
		return last_free_page - 1;
	}
	else
	{
		
		if (free_pages == 0) {

			find_erasable_block(event);	
			if (last_free_page == NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE) {
				last_free_page = 0;
			}
			
			
			int i = last_free_page / BLOCK_SIZE;
			//printf("-------------------------- i ----- %d\n", i);
			//printf("-------------------------- Blocks[i].free ----- %d\n", Blocks[i].free);
			while(Blocks[i].free == 0){ 
				i++; 
				if (i == NUMBER_OF_ADDRESSABLE_BLOCKS - 1) {
					i = 0;
				}
				
			}
			//printf("-------------------------- i ----- %d\n", i);
			
			for(int j = 0; j < BLOCK_SIZE; j++)
			{
				//printf("--------------------- last_free_page ----  %d\n", last_free_page);
				if (pages[i * BLOCK_SIZE + j].state == EMPTY) {
					last_free_page = i * BLOCK_SIZE + j;
					Blocks[last_free_page / BLOCK_SIZE].free --;
					free_pages --;
					last_free_page ++;
					return last_free_page - 1;
				}
			}

		}
		else
		{
			int i = last_free_page / BLOCK_SIZE;
			while(Blocks[i].free == 0){ 
				i++; 
				if (i == NUMBER_OF_ADDRESSABLE_BLOCKS - 1) {
					i = 0;
				}
				
			}
			
			for(int j = 0; j < BLOCK_SIZE; j++)
			{
				if (pages[j].state == EMPTY) {
					last_free_page = i * BLOCK_SIZE + j;
					Blocks[last_free_page / BLOCK_SIZE].free --;
					free_pages --;
					last_free_page ++;
					//printf("--------------------- last_free_page ----  %d\n", last_free_page);
					return last_free_page - 1;
				}
			}
			
		}
	}
	
	// --------------------------------------------- need to add a error if there isnt any return of ppn
}

void FtlImpl_Page::find_erasable_block(Event &event)
{

	
	for(int i = 0; i < rs_block_num; i++)
	{
		int max = arr_max[i];
		int rs = rs_blocks[i];
		int k = 0;
		
		
		for(int j = 0; j < BLOCK_SIZE; j++)
		{
			
			int a = max * 4 + j;		
			
			if (pages[a].state == VALID) {
				pages[rs * 4 + k].data = pages[a].data;
				pages[rs * 4 + k].state = VALID;
				Blocks[rs].free --;
				k ++;
			}

			int p = 0;
			while(p <  (NUMBER_OF_ADDRESSABLE_BLOCKS - rs_block_num - 1) * BLOCK_SIZE){
				if (MT[p] == a) {
					MT[p] = rs * 4 + k;
					exit;
				}
				p++;
			}	

			pages[a].data = '-';
			pages[a].state = EMPTY;
			/////////////////////////////////////////////////////////////////
			//printf("-------------------- error %d\n" , pages[a].state);
		}
		//printf("---------------- max %d\n", max);
		Blocks[max].is_rs = true;
		Blocks[rs].is_rs = false;          /////////////////////////////////////////
		
		Blocks[max].free = 4;
		Blocks[max].erase_number ++;
		Blocks[max].invalid_pages = 0;
		//----------------------------------
		
		down_arr_max(max);
		//----------------------------------
		rs_blocks[i] = max;

		int counter = 0;	
		for(int k = 0; k < NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE; k++)
		{
			if (k > 9) {
			printf("%d[%c][%d]  " , k, pages[k].data, pages[k].state);
			}
			else
			{
				printf("%d [%c][%d]  " , k, pages[k].data, pages[k].state);
			}
			counter ++;
			if (counter == 4) {
				printf("-- Blocks[k].free => %d", Blocks[k / BLOCK_SIZE].free);
				printf(" \n");
				counter = 0;
			}
		}
		printf("---------------------------------------------\n");
		printf("---------------------------------------------\n");
		printf("---------------------------------------------\n");
	
	
	printf("------------------------------------------------------------------------------\n");
	for(int i = 0; i < (NUMBER_OF_ADDRESSABLE_BLOCKS - rs_block_num - 1) * BLOCK_SIZE; i++)
	{
		printf("MT[%d]>%d | ", i , MT[i]);
	}
	printf("\n");
	printf("------------------------------------------------------------------------------\n");

		// -------------------------------------------------------------- is that cor??
		Event eraseEvent = Event(ERASE, event.get_logical_address(), 1, event.get_start_time(), NULL);  
		eraseEvent.set_address(Address(0, PAGE));
		if (controller.issue(eraseEvent) == FAILURE) printf("Erase failed");
		event.incr_time_taken(eraseEvent.get_time_taken());
		controller.stats.numFTLErase++;
	}
	free_pages = free_pages + BLOCK_SIZE * rs_block_num;
	//printf("------------------- free_pages  %d\n", free_pages);
	//-------------------------------------------------------
	return;
}


/////////////////////////////////////////////////////////////////////////

FtlImpl_Page::FtlImpl_Page(Controller &controller):
	FtlParent(controller)
{
	last_free_page = 0;

	trim_map = new bool[NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE];

	/////////////////////////////////////////////////////////////////////
	//total number of pages
	pages = new Pagearr[NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE]; 

	// its fixed but must be dynamic in config file! ---> 25%
	rs_block_num = NUMBER_OF_ADDRESSABLE_BLOCKS * 0.25; 

	//number of map table with rs block
	// keep one block free (... - 1) for rewrite process in pages!
	MT = new int[ (NUMBER_OF_ADDRESSABLE_BLOCKS - rs_block_num - 1) * BLOCK_SIZE];  

	free_pages = (NUMBER_OF_ADDRESSABLE_BLOCKS - rs_block_num) * BLOCK_SIZE;

	//total number of blocks
	Blocks = new Blockarr[NUMBER_OF_ADDRESSABLE_BLOCKS];
	
	//init pages and mapping table 
	for(int i = 0; i < NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE; i++)
	{
		pages[i].state = EMPTY;
		pages[i].data = '-';
		MT[i] = -1;
	}
	
	for(int i = 0; i < NUMBER_OF_ADDRESSABLE_BLOCKS; i++)
	{
		Blocks[i].invalid_pages = 0;
		Blocks[i].erase_number = 0;
		Blocks[i].free = 4;
		Blocks[i].is_rs = false;
	}

	// save block numbers that reserved
	rs_blocks = new int[rs_block_num];

	int tmp = NUMBER_OF_ADDRESSABLE_BLOCKS;
	for(int i = 0; i < rs_block_num; i++)
	{	
		tmp --;
		rs_blocks[i] = tmp;
		Blocks[tmp].is_rs = true;
	}
	
	arr_max = new int[NUMBER_OF_ADDRESSABLE_BLOCKS];
	
	for(int i = 0; i < NUMBER_OF_ADDRESSABLE_BLOCKS; i++)
	{
		arr_max[i] = i;
	}
	

	/////////////////////////////////////////////////////////////////////
	numPagesActive = 0;

	return;
}

FtlImpl_Page::~FtlImpl_Page(void)
{

	return;
}

enum status FtlImpl_Page::read(Event &event)
{
	event.set_address(Address(0, PAGE));
	event.set_noop(true);

	/////////////////////////////////////////////////////////////////////

	
	if (MT[event.get_logical_address()] != -1) {
		int ppn = MT[(int)event.get_logical_address()] ;

		printf("data is : %c\n", pages[ppn].data);
	}
	else
	{
		printf("--------------there is no data in this lpn!\n");
	}
	
	/////////////////////////////////////////////////////////////////////	

	controller.stats.numFTLRead++;
	//printf("------------------------------------------------\n");
	return controller.issue(event);
}

enum status FtlImpl_Page::write(Event &event)
{
	event.set_address(Address(1, PAGE));
	event.set_noop(true);
	/////////////////////////////////////////////////////////////////////

	int ppn;

	printf("------------------------------------------------------------------------------\n");
	for(int i = 0; i < (NUMBER_OF_ADDRESSABLE_BLOCKS - rs_block_num - 1) * BLOCK_SIZE; i++)
	{
		printf("MT[%d]>%d | ", i , MT[i]);
	}
	printf("\n");
	printf("------------------------------------------------------------------------------\n");


	////////////////////////////////// assert
	assert(event.get_logical_address() < (NUMBER_OF_ADDRESSABLE_BLOCKS - rs_block_num - 1) * BLOCK_SIZE);


	if (MT[event.get_logical_address()] != -1)
	{
		

		ppn = find_free_page(event);
		// printf("------------------ ppn %d ---------- past pbn %d\n", ppn, MT[event.get_logical_address()] / BLOCK_SIZE );
		pages[MT[event.get_logical_address()]].state = INVALID;
		Blocks[MT[event.get_logical_address()] / BLOCK_SIZE].invalid_pages ++;
		up_arr_max( MT[event.get_logical_address()] / BLOCK_SIZE );
		
		pages[ppn].state = VALID;
		pages[ppn].data = event.get_data();
		MT[event.get_logical_address()] = ppn;
		
		
		//printf("------------------- if\n");
	}
	else
	{
		ppn = find_free_page(event);

		MT[event.get_logical_address()] = ppn;
		pages[ppn].data = event.get_data();
		pages[ppn].state = VALID;

		//printf("------------------- else\n");
	}

	//printf("<< %c >> write to %d(lg_addr) -- ppn:%d pbn:%d\n", pages[ppn].data, event.get_logical_address(), ppn, ppn / NUMBER_OF_ADDRESSABLE_BLOCKS);
	/////////////////////////////////////////////////////////////////////
	controller.stats.numFTLWrite++;

	// if (numPagesActive == NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE)
	// {
	// 	numPagesActive -= BLOCK_SIZE;

	// 	Event eraseEvent = Event(ERASE, event.get_logical_address(), 1, event.get_start_time());
	// 	eraseEvent.set_address(Address(0, PAGE));

	// 	if (controller.issue(eraseEvent) == FAILURE) printf("Erase failed");

	// 	event.incr_time_taken(eraseEvent.get_time_taken());    //////-------

	//  controller.stats.numFTLErase++;
	// }

	// numPagesActive++;


	// showing table
	// printf("------------------------------------------------------------------------------\n");
	// for(int i = 0; i < (NUMBER_OF_ADDRESSABLE_BLOCKS - rs_block_num - 1) * BLOCK_SIZE; i++)
	// {
	// 	printf("MT[%d]>%d | ", i , MT[i]);
	// }
	// printf("\n");
	
	printf("------------------------------------------------------------------------------\n");
	int counter = 0;	
	for(int i = 0; i < NUMBER_OF_ADDRESSABLE_BLOCKS * BLOCK_SIZE; i++)
	{
		if (i > 9) {
			printf("%d[%c][%d]  " , i, pages[i].data, pages[i].state);
		}
		else
		{
			printf("%d [%c][%d]  " , i, pages[i].data, pages[i].state);
		}
		counter ++;
		if (counter == 4) {
			printf("-- Blocks[%d] | invalid_pages => %d | erase_number => %d | free => %d | is_rs => %d ",
					i / BLOCK_SIZE,
					Blocks[i / BLOCK_SIZE].invalid_pages, 
					Blocks[i / BLOCK_SIZE].erase_number, 
					Blocks[i / BLOCK_SIZE].free,
					Blocks[i / BLOCK_SIZE].is_rs);
			printf(" \n");
			counter = 0;
		}
	}
	printf("------------------------------------------------\n");
	printf("------------------------------------------------\n");
	return controller.issue(event);
}

enum status FtlImpl_Page::trim(Event &event)
{
	controller.stats.numFTLTrim++;

	// uint dlpn = event.get_logical_address();

	// if (!trim_map[event.get_logical_address()])
	// 	trim_map[event.get_logical_address()] = true;

	// // Update trim map and update block map if all pages are trimmed. i.e. the state are reseted to optimal.
	// long addressStart = dlpn - dlpn % BLOCK_SIZE;
	// bool allTrimmed = true;
	// for (uint i=addressStart;i<addressStart+BLOCK_SIZE;i++)
	// {
	// 	if (!trim_map[i])
	// 		allTrimmed = false;
	// }

	// if (allTrimmed)
	// {
	// 	Event eraseEvent = Event(ERASE, event.get_logical_address(), 1, event.get_start_time());
	// 	eraseEvent.set_address(Address(0, PAGE));

	// 	if (controller.issue(eraseEvent) == FAILURE) printf("Erase failed");

	// 	event.incr_time_taken(eraseEvent.get_time_taken());

	// 	for (uint i=addressStart;i<addressStart+BLOCK_SIZE;i++)
	// 		trim_map[i] = false;

	// 	controller.stats.numFTLErase++;

	// 	numPagesActive -= BLOCK_SIZE;
	// }

	return SUCCESS;
}
