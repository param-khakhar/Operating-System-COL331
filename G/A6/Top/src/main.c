#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <limits.h>

int queries;
int gpages;
int gframes;

struct Node
{
	struct Node* prev;
	struct Node* next;
	int address;			// Address stored by the Node
	int count;				// Number of Accesses, used for LFU scheme
	int useBit;				// Used for Clock Algorithm
	int insertTime;			// Time at which the node was inserted
};

struct LinkedList
{
	struct Node* head;
	struct Node* tail;
	struct Node* hand;		// Clock hand for the clock algorithm
	int size;
};

int list_pop(struct LinkedList* lst)
{
	if(lst->size == 0)
		return -1;

	int temp = lst->head->address;
	lst->head = lst->head->next;
	if(lst->head == NULL)	
		printf("Goodness Me\n");	
	lst->size--;
	return temp;
}

void list_push(struct LinkedList* lst, int i)
{
	struct Node* node = malloc(sizeof(struct Node));
	node->address = i;
	if(lst->size == 0)
	{
		lst->head = node;
		lst->tail = node;
	}
	else
	{
		lst->tail->next = node;
		lst->tail = node;
	}
	lst->size++;
}

void list_insert(struct LinkedList* lst, int i)
{
	struct Node* node = malloc(sizeof(struct Node));
	node->address = i;
	node->count = 1;
	if(lst->size == 0)
	{
		lst->head = node;
		lst->tail = node;
	}
	else
	{
		node->next = lst->head;
		lst->head = node;
	}
	lst->size++;
}

void print_list_D(struct LinkedList* lst)
{
	int j = 0;
	struct Node* temp = lst->hand;
	while(j < lst->size)
	{
		printf("%d\n",temp->address);
		temp = temp->next;
		j++;
	}
}

void list_insert_D(struct LinkedList* lst, int i, int t)
{
	struct Node* node = malloc(sizeof(struct Node));
	node->address = i;
	node->insertTime = t;
	node->useBit = 1;
	if(lst->size == 0)
	{
		lst->hand = node;
		node->prev = node;
		node->next = node;
	}
	else
	{
		struct Node* temp = lst->hand;
		node->prev = temp->prev;
		if(temp->prev)	temp->prev->next = node;

		temp->prev = node;
		node->next = temp;
		lst->hand = node;
	}
	lst->size++;
}

int list_pop_D(struct LinkedList* lst, int n)
{
	if(lst->size == 0)	return -1;
	struct Node* temp = lst->hand;
	struct Node* rem;
	int mn = INT_MAX;
	int j = 0;
	while(j < (lst->size)*n)
	{
		j++;
		if(temp->useBit == 1-n)
		{
			if(temp->insertTime < mn)
			{
				rem = temp;
				mn = rem->insertTime;
			}
		}
		else	temp->useBit--;
		temp = temp->next;
	}
	lst->size--;

	if(rem == lst->hand)	lst->hand = rem->next;

	rem->prev->next = rem->next;
	rem->next->prev = rem->prev;

	rem->next = NULL;
	rem->prev = NULL;

	return rem->address;
}

void setUseBit(struct LinkedList* lst, int i)
{
	struct Node* temp = lst->hand;
	while(temp->address != i)
	{
		temp = temp->next;
	}
	temp->useBit++;
}

void move_back(struct LinkedList* lst, int i)
{
	struct Node* temp = lst->head;
	struct Node* bubble;
	if(temp->address != i)
	{
		while(temp->next && temp->next->address != i)
		{
			temp = temp->next;
		}
		if(temp->next == NULL)
		{
			printf("OMG\n");
//			exit(0);
			return;
		}
		bubble = temp->next;
		temp->next = temp->next->next;
		while(temp->next != NULL)	temp = temp->next;

		temp->next = bubble;
		// if(temp != lst->tail)
		// {
		// 	printf("Pieces!\n");
		// }

		// lst->tail->next = bubble;
		lst->tail = bubble;
		lst->tail->next = NULL;
		assert(lst->tail->address == i);
	}
	else
	{
		//printf("Changing Head\n");
		bubble = lst->head;
		lst->head = lst->head->next;
		lst->tail->next = bubble;
		lst->tail = lst->tail->next;
		lst->tail->next = NULL;

		assert(lst->tail->address == i);
	}
	assert(lst->tail->address == i);
}



void update_count(struct LinkedList* lst, int i)
{
	// printf("Here: %d %d\n", lst->head->address, lst->size);
	struct Node* temp = lst->head;
	if(lst->head == NULL)	return;

	if(temp->address == i)
	{
		temp->count++;
		if(temp->next && temp->next->count < temp->count)
		{
			lst->head = temp->next;
			struct Node* temp2 = temp->next;
			while(temp2->next && temp2->next->count < temp->count)
			{
				temp2 = temp2->next;
			}
			temp->next = temp2->next;
			temp2->next = temp;
		}
	}
	else
	{
		while(temp->next && temp->next->address != i)
		{
			// printf("Addr: %d\n", temp->next->address);
			temp = temp->next;
		}
		assert((temp->next->address == i));
		
		struct Node* temp1 = temp->next;
		temp1->count++;
		if(temp1->next && temp1->next->count < temp1->count)
		{
			struct Node* temp2 = temp1;
			while(temp2->next && temp2->next->count < temp1->count)
			{
				temp2 = temp2->next;
			}
			if(temp2->next == NULL)	lst->tail = temp1;
			temp->next = temp1->next;
			temp1->next = temp2->next;
			temp2->next = temp1;
		}
	}
}


int FIFO(FILE* tc)
{
	struct LinkedList* lst = malloc(sizeof(struct LinkedList));
	int pages = 0;
	int frames = 0;
	fscanf(tc, "%d", &pages);
	fscanf(tc, "%d", &frames);
	gpages = pages;
	gframes = frames;
	lst->size = 0;
	int page_faults = 0;

	int* present = calloc(frames+5, sizeof(int));
	int i;
	while (!feof (tc))
	{  
		queries++;
		fscanf (tc, "%d", &i);
		if(lst->size == pages)
		{
			if(present[i] == 0)
			{
				//Check Only for Misses

				int rem = list_pop(lst);
				list_push(lst,i);
				page_faults++;
				present[i] = 1;
				present[rem] = 0;
			}
		}
		else
		{	
			if(present[i] == 0)				
			{
				//Check Only for Misses

				page_faults++;
				list_push(lst, i);
				present[i] = 1;
			}
		}
	}
	return page_faults;
}



int LFU(FILE* tc){
	struct LinkedList* lst = malloc(sizeof(struct LinkedList));
	int pages = 0;
	int frames = 0;
	fscanf(tc, "%d", &pages);
	fscanf(tc, "%d", &frames);
	lst->size = 0;
	int page_faults = 0;

	int* present = calloc(frames+5, sizeof(int));
	int i;
	while(!feof(tc))
	{
		// printf("Here\n");
		fscanf(tc, "%d", &i);
		// printf("Access: %d\n", i);
		if(lst->size == pages)
		{
			if(present[i] == 0)
			{
				int rem = list_pop(lst);
				list_insert(lst,i);
				page_faults++;
				present[i] = 1;
				present[rem] = 0;
			}
			else
			{
				// Updates the count of the node containing the address i and updates the position in the list.
				update_count(lst, i);
			}
		}
		else
		{
			if(present[i] == 0)				
			{
				page_faults++;
				list_insert(lst, i);
				present[i] = 1;
			}
			else
			{
				update_count(lst, i);
			}
		}
	}

	return page_faults;
}



int LRU(FILE* tc)
{
	struct LinkedList* lst = malloc(sizeof(struct LinkedList));
	int pages = 0;
	int frames = 0;
	fscanf(tc, "%d", &pages);
	fscanf(tc, "%d", &frames);
	lst->size = 0;
	int page_faults = 0;

	int* present = calloc(frames+5, sizeof(int));
	int i;
	while (!feof (tc))
	{  
		fscanf (tc, "%d", &i);
		// printf("Access: %d\n", i);

		if(lst->size == pages)
		{
			if(present[i] == 0)
			{
				// printf("Here2\n");
				int rem = list_pop(lst);
				list_push(lst,i);
				assert(lst->tail->address == i);
				page_faults++;
				present[i] = 1;
				present[rem] = 0;
			}
			else
			{
				// printf("Here1\n");
				//Iterates over the list, and moves the node to the tail.
				move_back(lst, i);
				assert(lst->tail->address == i);
			}
		}
		else
		{
			if(present[i] == 0)				
			{
				// printf("Here3\n");
				page_faults++;
				list_push(lst, i);
				assert(lst->tail->address == i);
				present[i] = 1;
			}
			else
			{
				// printf("Here4\n");
				move_back(lst, i);
				assert(lst->tail->address == i);
			}
		}
	}
	return page_faults;
}

// Clock + FIFO

int Clock(FILE* tc)
{
	struct LinkedList* lst = malloc(sizeof(struct LinkedList));
	int pages = 0;
	int frames = 0;
	fscanf(tc, "%d", &pages);
	fscanf(tc, "%d", &frames);
	lst->size = 0;
	int page_faults = 0;
	int timer = 0;
	int* present = calloc(frames+5, sizeof(int));
	int i;
	while (!feof (tc))
	{  
		fscanf (tc, "%d", &i);
		// printf("Address: %d %d\n",i, lst->size);
		if(lst->size == pages)
		{
			if(present[i] == 0)
			{
				int rem = list_pop_D(lst, 1);
				list_insert_D(lst, i, timer);
				page_faults++;
				present[i] = 1;
				present[rem] = 0;
			}
			else
			{
				//Iterates over the list, and moves the node to the tail.
				setUseBit(lst, i);
			}
		}
		else
		{
			if(present[i] == 0)				
			{
				page_faults++;
				list_insert_D(lst, i, timer);
				present[i] = 1;
			}
			else
			{
				setUseBit(lst, i);
			}
		}
		timer++;
	}
	return page_faults;
}


int N_Chance(FILE* tc, int n)
{
	struct LinkedList* lst = malloc(sizeof(struct LinkedList));
	int pages = 0;
	int frames = 0;
	fscanf(tc, "%d", &pages);
	fscanf(tc, "%d", &frames);
	lst->size = 0;
	int page_faults = 0;
	int timer = 0;
	int* present = calloc(frames+5, sizeof(int));
	int i;
	while (!feof (tc))
	{  
		fscanf (tc, "%d", &i);
		// printf("Address: %d %d\n",i, lst->size);
		if(lst->size == pages)
		{
			if(present[i] == 0)
			{
				int rem = list_pop_D(lst, n);
				list_insert_D(lst, i, timer);
				page_faults++;
				present[i] = 1;
				present[rem] = 0;
			}
			else
			{
				//Iterates over the list, and moves the node to the tail.
				setUseBit(lst, i);
			}
		}
		else
		{
			if(present[i] == 0)				
			{
				page_faults++;
				list_insert_D(lst, i, timer);
				present[i] = 1;
			}
			else
			{
				setUseBit(lst, i);
			}
		}
		timer++;
	}
	return page_faults;
}


int main(int argc, char* argv[]) 
{
	FILE* tc;
	bool ff, lf, lr, cl, sc;
	ff = false;
	lf = false;
	lr = false;
	cl = false;
	sc = false;

	FILE* data = fopen("../output/data.dat", "w");

	char* fifo = "FF";
	char* lfu = "LF";
	char* lru = "LR";
	char* clock = "CL";
	char* nchance= "SC";
	int* page_faults = calloc(5, sizeof(int));
	int j = 1;
	while(j < argc)
	{
		if(j == 1)
		{
			tc = fopen(argv[j], "r");
		}
		else
		{
			char* temp = argv[j];
			// printf("Temp: %s\n",temp);
			if(strcmp(temp, fifo) == 0)	ff = true;
			else if(strcmp(temp, lfu) == 0)	lf = true;
			else if(strcmp(temp, lru) == 0)	lr = true;
			else if(strcmp(temp, clock) == 0)	cl = true;
			else if(strcmp(temp, nchance) == 0)	sc = true; 
			else break;
		}
		j++;
	}

	if(j == 1){
		printf("Invalid Input File\n");
		return 0;
	}

	else if(j == 2){
		ff = true;
		lf = true;
		lr = true;
		cl = true;
		sc = true;
	}

	int count = 0;

	if(ff){
		//Execute FIFO
		printf("FIFO\n");
		tc = fopen(argv[1], "r");
		int pf = FIFO(tc);
		printf("Faults: FIFO: %d\n", pf);
		fprintf(data, "%d FIFO %d\n",count++, pf);
		page_faults[0] = pf;
	}

	if(lf){
		//Execute LFU
		printf("LFU\n");
		tc = fopen(argv[1], "r");
		int pf = LFU(tc);
		printf("Faults: LFU: %d\n",pf);
		fprintf(data, "%d LFU %d\n",count++, pf);
		page_faults[1] = pf;
	}

	if(lr){
		//Execute LRU
		printf("LRU\n");
		tc = fopen(argv[1], "r");
		int pf = LRU(tc);
		printf("Faults: LRU: %d\n", pf);
		fprintf(data, "%d LRU %d\n",count++, pf);
		page_faults[2] = pf;
	}

	if(cl){
		//Execute Clock
		printf("Clock\n");
		tc = fopen(argv[1], "r");
		int pf = Clock(tc);
		printf("Faults: Clock: %d\n",pf);
		fprintf(data, "%d CLOCK %d\n",count++, pf);
		page_faults[3] = pf;
	}

	if(sc){
		//Execute N-Chance
		printf("Nchance\n");
		tc = fopen(argv[1], "r");
		int chance = 1;
		int pf = N_Chance(tc, chance);
		printf("Faults: Nchance: %d\n", pf);
		fprintf(data, "%d %d-Chance %d\n",count++, chance, pf);
		page_faults[4] = pf;
	}

	float maxy = 0.0;
	for(int i=0;i<5;i++)
	{
		if(page_faults[i] > maxy)	maxy = page_faults[i];
	}

	FILE* gnuplotPipe = popen ("gnuplot -persistent", "w");
	char* commands [] = {"set title \"Comparison between different Replacement Policies \\n for %d queries, %d pages, %d frames", "set boxwidth 0.5", "set style fill solid", "plot \"../output/data.dat\" using 1:3:xtic(2) title \"Miss\" with boxes ls 1 lt rgb \"#406090\"","exit"};

	char setOut [100] = "set output \"../output/";
	char* result = strcat(setOut,"plot.png");
	char* res1 = strcat(result,"\"");
	char res [10000] = "";

	strcpy(res,res1);
	fprintf(gnuplotPipe, "set xrange [-1:%d]\n", count);
	fprintf(gnuplotPipe, "%s\n","set xlabel \"Policies\"");
	fprintf(gnuplotPipe, "%s\n","set ylabel \"Page Faults\"");
	fprintf(gnuplotPipe, "set yrange [0:%f]\n", maxy + maxy/4);
	fprintf(gnuplotPipe, "%s\n","set term png");
	fprintf(gnuplotPipe, "%s\n",res);


	for(int i=0; i < strlen(commands); i++)
	{
		if(i == 0)	fprintf(gnuplotPipe, commands[i], queries, gpages, gframes);	
		else	fprintf(gnuplotPipe, commands[i]);
		fprintf(gnuplotPipe, "\n");
	}
	fclose(data);
	return 0; 
} 