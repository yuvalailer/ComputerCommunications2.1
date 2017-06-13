#include <errno.h>	/* ERANGE, errno */
#include <limits.h>	/* LONG_MAX, LONG_MIN */
#include <stdio.h>	/* FILE, printf, fprintf, sprintf, stderr, fopen, fclose */
#include <stdlib.h>	/* EXIT_FAILURE, EXIT_SUCCESS, NULL, strtol */
#include <string.h>	/* strncmp, //strerror_s */

#define USAGE_OPERANDS_MISSING_MSG		"Missing operands\nUsage: %s <TYPE [RR/DRR]> <INPUT FILE> <OUTPUT FILE> <WEIGHT> <QUANTUM>\n"
#define USAGE_OPERANDS_SURPLUS_MSG		"Too many operands\nUsage: %s <TYPE [RR/DRR]> <INPUT FILE> <OUTPUT FILE> <WEIGHT> <QUANTUM>\n"
#define ERROR_EXIT_MSG					"Exiting...\n"
#define F_ERROR_ENQUEUE_FAILED_MSG		"[Error] Program failed to enqueue packed ID: %ld\n"
#define F_ERROR_FUNCTION_SPRINTF_MSG	"[Error] sprintf() failed with an error\n"
#define F_ERROR_FUNCTION_STRTOL_MSG		"[Error] strtol() failed with an error: %s\n"
#define F_ERROR_INPUT_CLOSE_MSG			"[Error] Close input file: %s\n"
#define F_ERROR_INPUT_FILE_MSG			"[Error] Input file '%s': %s\n"
#define F_ERROR_IP_INVALID_MSG			"[Error] The ip address %s is not a valid IPv4\n"
#define F_ERROR_LENGTH_INVALID_MSG		"[Error] The length '%s' is not a number between 64 to 16384\n"
#define F_ERROR_OUTPUT_CLOSE_MSG		"[Error] Close output file: %s\n"
#define F_ERROR_OUTPUT_FILE_MSG			"[Error] Output file '%s': %s\n"
#define F_ERROR_QUANTUM_INVALID_MSG		"[Error] The quantum '%s' is not non negative integer\n"
#define F_ERROR_PKTID_INVALID_MSG		"[Error] The packet ID '%s' is not a valid long integer\n"
#define F_ERROR_PORT_INVALID_MSG		"[Error] The port '%s' is not a number between 0 to 65535\n"
#define F_ERROR_TIME_INVALID_MSG		"[Error] The time '%s' is not a valid long integer\n"
#define F_ERROR_TYPE_MSG				"[Error] The only valid types are RR and DRR\n"
#define F_ERROR_WEIGHT_INVALID_MSG		"[Error] The weight '%s' is not a positive integer\n"

typedef struct DataStructure {
	struct Packets* head;	/* Pointer to the head of the round double linked list */
	int count;		/* The total number of packets, Need to be updated in every insert & delete */
	int same_flow_send_count; /* the number of packets sent from a certain flow  */
	struct Packets* flow_pk; /* a pointer to a pk for 'same_flow' use perpose */ 
} structure;
typedef struct Packets {
	long pktID;				/* Unique ID (long int [-9223372036854775808,9223372036854775807]) */
	long Time;				/* Arrival time (long int [-9223372036854775808,9223372036854775807]) */
	char Sadd[15];			/* Source ip address (string Ex. '192.168.0.1') */
	int Sport;				/* Source port (int [0,65535]) */
	char Dadd[15];			/* Destination ip address (string Ex. '192.168.0.1') */
	int Dport;				/* Destination port (int [0,65535]) */
	int length;				/* Packet length (int [64,16384]) */
	int weight;				/* Flow weight (int [1,2147483647]) */
	struct Packets* next;	/* Pointer to next packet in the list */
	struct Packets* prev;	/* Pointer to previously packet in the list */
	struct Packets* up;		/* Pointer to upper packet in the queue */
	struct Packets* down;	/* Pointer to lower packet in the queue */
} packet;
char TYPE[4];				/* RR or DRR */
FILE* IN_FILE = NULL;		/* The input file */
FILE* OUT_FILE = NULL;		/* The output file */
int QUANTUM = 0;			/* The quantum */
long CLOCK = LONG_MIN;		/* The current time */
structure STRUCTURE;		/* Our data structure */

int program_end(int error);								/* Close gracefully everything */
int validate_IPv4(const char *s);						/* Verify that the string is valid IPv4 */
int convert_strin2long(char *input, long *output, long from, long to, char *error_string);	/* Convert the string to int */
int read_packet(packet* pk, int default_weight);		/* Read one line from the input file and parse her */
void copy_packet(packet* src, packet* dst);				/* Copy the content of the source packet to the destination packet */
void write_packet(packet* pk);							/* Write the packet ID to the output file */
int same_flow(packet* pacA, packet* pacB);				/* Check if two packets belong to the same flow */
int enqueue(packet* new_pk);							/* Add packet to our data structure */
int dequeue(packet* pk);								/* Remove packet from our data structure */
packet* find_packet();									/* xxxxxxxxxxxxx */
int send_packet();										/* xxxxxxxxxxxxx */
void print();											/* Print the packets in line */
int main(int argc, char *argv[]);						/* Simulate round robin algorithm */

/* int program_end(int error) { }
 *
 * Receive exit code,
 * Close gracefully everything,
 * Return exit code,
 */
int program_end(int error) {
	char errmsg[256];
	int res = 0;
	if ((IN_FILE != NULL) && (fclose(IN_FILE) == EOF)) { /* Upon successful completion 0 is returned. Otherwise, EOF is returned and errno is set to indicate the error. */
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_INPUT_CLOSE_MSG, errmsg);
		res = errno;
	}
	if ((OUT_FILE != NULL) && (fclose(OUT_FILE) == EOF)) { /* Upon successful completion 0 is returned. Otherwise, EOF is returned and errno is set to indicate the error. */
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_OUTPUT_CLOSE_MSG, errmsg);
		res = errno;
	}
	if ((error != 0) || (res != 0)) {
		fprintf(stderr, ERROR_EXIT_MSG);
		if (error != 0) { /* If multiple error occurred, Print the error that called 'program_end' function. */
			res = error;
		}
	}
	return res;
}
/* intvalidate_IPv4(const char *s) { }
 *
 * Receive string,
 * Verify that the string is valid IPv4
 * Return 0 if input is a valid IPv4 address,
 * return -1 otherwise
 */
int validate_IPv4(const char *s) { /* http://stackoverflow.com/questions/791982/determine-if-a-string-is-a-valid-ip-address-in-c#answer-14181669 */
	char tail[16];
	int c, i;
	int len = strlen(s);
	unsigned int d[4];
	if (len < 7 || 15 < len) {
		return -1;
	}
	tail[0] = 0;
	c = sscanf_s(s, "%3u.%3u.%3u.%3u%s", &d[0], &d[1], &d[2], &d[3], tail);
	if (c != 4 || tail[0]) {
		return -1;
	}
	for (i = 0; i<4; i++) {
		if (d[i] > 255) {
			return -1;
		}
	}
	return 0;
}
/* int convert_strin2long(char *input, long *output, long from, long to, char *error_string) { }
 *
 * Receive:	input string
 * 		pointer where to save the output int
 * 		limitation (included).on the int range
 * 		message to print in case of an error
 * Convert the string to int
 * Return 0 if succeed,
 * Return errno if error occurred.
 */
int convert_strin2long(char *input, long *output, long from, long to, char *error_string) { /* https://linux.die.net/man/3/strtol */
	/* Function variables */
	char errmsg[256];		/* The message to print in case of an error */
	char output_char[10];	/* variable for sprintf() */
	char *endptr;			/* variable for strtol() */
	/* Function body */
	*output = strtol(input, &endptr, 10); /* If an underflow occurs. strtol() returns LONG_MIN. If an overflow occurs, strtol() returns LONG_MAX. In both cases, errno is set to ERANGE. */
	if ((errno == ERANGE && (*output == LONG_MAX || *output == LONG_MIN)) || (errno != 0 && *output == 0)) {
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_FUNCTION_STRTOL_MSG, errmsg);
		return errno;
	} else if ((endptr == input) || (*output < from) || (*output > to)) { /* (Empty string) or (Not in range) */
		fprintf(stderr, error_string, input);
		return EXIT_FAILURE;
	} else if (sprintf(output_char, "%ld", *output) < 0) { /* sprintf(), If an output error is encountered, a negative value is returned. */
		fprintf(stderr, F_ERROR_FUNCTION_SPRINTF_MSG);
		return EXIT_FAILURE;
	} else if (strncmp(output_char, input, 10) != 0) { /* Contain invalid chars */
		fprintf(stderr, error_string, input);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
/* int read_packet(packet* pk, int default_weight) { }
 *
 * Receive packet object and the default weight
 * Read one line from the input file and parse her
 * Return 1 if read succeed,
 * Return 0 in case EOF reached or if an error occurred.
 */
int read_packet(packet* pk, int default_weight) {
	/* Function variables */
	char line[105];	/* 105 == pktID[20]+Time[20]+Sadd[15]+Sport[5]+Dadd[15]+Dport[5]+length[5]+weight[11]+spaces[7]+"\r\n"[2] */
	char *word;		/* Each string splited by whitespace */
	char *newline;
	int count;		/* Count how many words was in the current line */
	int res = 0;	/* Temporary variable to store function response */
	long temp = 0;	/* Temporary variable */
	/* Function body */
	if ((fgets(line, sizeof(line), IN_FILE)) && (strlen(line) > 26)) { /* return s on success, and NULL on error or when end of file occurs while no characters have been read. */
		pk->weight = default_weight; /* Use the default weight */
		count = 0; /* Init the counter */
		word = strtok(line, " ");
		while (word != NULL) {
			/* Remove newline chars */
			newline = strchr(word, '\r');
			if (newline) {
				word[newline - word] = '\0';
			}
			newline = strchr(word, '\n');
			if (newline) {
				word[newline - word] = '\0';
			}
			/* Parse the current word */
			if ((count == 0) && (strlen(word) <= 20)) {
				if ((res = convert_strin2long(word, &temp, LONG_MIN, LONG_MAX, F_ERROR_PKTID_INVALID_MSG)) > 0) {
					return 0; /* Error occurred, Exit */
				} else {
					pk->pktID = temp;
				}
			} else if ((count == 1) && (strlen(word) <= 20)) {
				if ((res = convert_strin2long(word, &temp, LONG_MIN, LONG_MAX, F_ERROR_TIME_INVALID_MSG)) > 0) {
					return 0; /* Error occurred, Exit */
				} else {
					pk->Time = temp;
				}
			} else if ((count == 2) && (strlen(word) <= 15)) {
				if (validate_IPv4(word) == -1) {
					fprintf(stderr, F_ERROR_IP_INVALID_MSG, word);
					return 0;
				} else {
					strncpy(pk->Sadd, word, 15);
				}
			} else if ((count == 3) && (strlen(word) <= 5)) {
				if ((res = convert_strin2long(word, &temp, 0, 65535, F_ERROR_PORT_INVALID_MSG)) > 0) {
					return 0; /* Error occurred, Exit */
				} else {
					pk->Sport = (int)temp; /* This is secure because we alreade validate 'temp' max&min values */
				}
			} else if ((count == 4) && (strlen(word) <= 15)) {
				if (validate_IPv4(word) == -1) {
					fprintf(stderr, F_ERROR_IP_INVALID_MSG, word);
					return 0;
				} else {
					strncpy(pk->Dadd, word, 15);
				}
			} else if ((count == 5) && (strlen(word) <= 5)) {
				if ((res = convert_strin2long(word, &temp, 0, 65535, F_ERROR_PORT_INVALID_MSG)) > 0) {
					return 0; /* Error occurred, Exit */
				} else {
					pk->Dport = (int)temp; /* This is secure because we alreade validate 'temp' max&min values */
				}
			} else if ((count == 6) && (strlen(word) <= 5)) {
				if ((res = convert_strin2long(word, &temp, 1, 16384, F_ERROR_LENGTH_INVALID_MSG)) > 0) { /* TODO TODO TODO Change from 1 to 64 TODO TODO TODO !!!!!!!!!!!!! */
					return 0; /* Error occurred, Exit */
				} else {
					pk->length = (int)temp; /* This is secure because we alreade validate 'temp' max&min values */
				}
			} else if ((count == 7) && (strlen(word) <= 11)) {
				if ((res = convert_strin2long(word, &temp, 1, INT_MAX, F_ERROR_WEIGHT_INVALID_MSG)) > 0) {
					return 0; /* Error occurred, Exit */
				} else if (temp) { /* There was a value for weight in the input file */
					pk->weight = (int)temp; /* This is secure because we alreade validate 'temp' max&min values */
				}
			} else {
				return 0; /* Invalid input, Length is too long */
			}
			count += 1; /* Inc the counter */
			word = strtok(NULL, " ");
		}
		if (count < 7) {
			return 0; /* Invalid input, Length is too short */
		}
		printf("[A] pktID='%ld', Time='%ld', Sadd='%s', Sport='%d', Dadd='%s', Dport='%d', length='%d', weight='%d'\n", pk->pktID, pk->Time, pk->Sadd, pk->Sport, pk->Dadd, pk->Dport, pk->length, pk->weight); /* TODO DEBUG XXX DELME XXX XXX */
		pk->next = NULL; /* Pointer to next packet in the list */
		pk->prev = NULL; /* Pointer to prev packet in the list */
		pk->up = NULL; /* Pointer to upper packet in the queue */
		pk->down = NULL; /* Pointer to lower packet in the queue */
		return 1;
	} else {
		return 0; /* EOF */
	}
}
/* void copy_packet(packet* src, packet* dst) { }
 *
 * Receive pointers of two packets
 * Copy the content of the source packet to the destination packet
 */
void copy_packet(packet* src, packet* dst) {
	printf("~~~START copy_packet~~~\n"); /* XXX */
	dst->pktID = src->pktID;
	dst->Time = src->Time;
	strncpy(dst->Sadd, src->Sadd, 15);
	dst->Sport = src->Sport;
	strncpy(dst->Dadd, src->Dadd, 15);
	dst->Dport = src->Dport;
	dst->length = src->length;
	dst->weight = src->weight;
	dst->next = src->next;
	dst->prev = src->prev;
	dst->up = src->up;
	dst->down = src->down;
	printf("~~~END copy_packet~~~\n"); /* XXX */
	return;
}
/* void write_packet(packet* pk) { }
 *
 * Receive pointer to a packet
 * Write the packet ID to the output file
 */
void write_packet(packet* pk) {
	fprintf(OUT_FILE, "%ld: %ld\r\n", CLOCK, pk->pktID);
	fflush(OUT_FILE);
}
/* int same_flow(packet* pacA, packet* pacB) { }
 *
 * Receive pointer of two packets
 * Check if they belong to the same flow
 * Return 1 if both packets have the same source and destination (IP&port)
 * Return 0 o.w
 */
int same_flow(packet* pacA, packet* pacB) {
	if ((STRUCTURE.count == 4) && (STRUCTURE.head->pktID == 8)) { /* TODO XXX DELME XXX */
		printf("TODO DEBUG DELME XXX\n");/* TODO XXX DELME XXX */
	} /* TODO XXX DELME XXX */
	printf("~~~START same_flow~~~\n"); /* XXX */
	printf("HHH\n"); /* XXX */
	printf("[E] Pointers: [pacA]%p [pacB]%p\n", (void *)pacA, (void *)pacB); /* XXX */
	printf("[C] pktID='%ld', Time='%ld', Sadd='%s', Sport='%d', Dadd='%s', Dport='%d', length='%d', weight='%d'\n", pacA->pktID, pacA->Time, pacA->Sadd, pacA->Sport, pacA->Dadd, pacA->Dport, pacA->length, pacA->weight); /* TODO DEBUG XXX DELME XXX XXX */
	printf("[D] pktID='%ld', Time='%ld', Sadd='%s', Sport='%d', Dadd='%s', Dport='%d', length='%d', weight='%d'\n", pacB->pktID, pacB->Time, pacB->Sadd, pacB->Sport, pacB->Dadd, pacB->Dport, pacB->length, pacB->weight); /* TODO DEBUG XXX DELME XXX XXX */
	
	printf("%d ", !strncmp(pacA->Sadd, pacB->Sadd, 15));
	printf("%d ", !strncmp(pacA->Dadd, pacB->Dadd, 15));
	printf("%d ", (pacA->Sport == pacB->Sport));
	printf("%d \n", (pacA->Dport == pacB->Dport));

	if ((!strncmp(pacA->Sadd, pacB->Sadd, 15)) &&
		(!strncmp(pacA->Dadd, pacB->Dadd, 15)) &&
		(pacA->Sport == pacB->Sport) &&
		(pacA->Dport == pacB->Dport)) {
		printf("III_1\n"); /* XXX */
		printf("~~~END same_flow~~~\n"); /* XXX */
		return 1;
	}
	printf("III_0\n"); /* XXX */
	printf("~~~END same_flow~~~\n"); /* XXX */
	return 0;
}
/* int enqueue(packet* pk) { }
 *
 * Receive packet
 * Add the packet to our data structure
 * Return 0 on success and 1 in case of an error
 */
int enqueue(packet* new_pk) {
	printf(" >>>>>>>>>>>>>>started enqueue on pk - %d ",new_pk->pktID);
	/* Function variables */
	packet* search_head = NULL; /* Pointer to the current element in our round double linked list */
	/* Init the new packet */
	if (STRUCTURE.head) { printf("[F] Pointers: [head]%p=>%p\n", (void *)(STRUCTURE.head), (void *)((*STRUCTURE.head).next)); } /* XXX */
	if (STRUCTURE.head == NULL) { /* This is the first packet in our data structure */
		STRUCTURE.head = new_pk; /* The new packet is our new head */
		(*new_pk).prev = new_pk; /* And he his the prev and next of himself */
		(*new_pk).next = new_pk;
		printf("[C] Pointers: [new_pk]%p=>%p [head]%p=>%p\n", (void *)new_pk, (void *)(new_pk->next), (void *)(STRUCTURE.head), (void *)((*STRUCTURE.head).next)); /* XXX */
		STRUCTURE.count = 1; /* We only have one packet in our data structure */
		copy_packet(new_pk, STRUCTURE.flow_pk);
	} else { /* Search if the packet belong to an old flow */
		search_head = STRUCTURE.head;
		do {
			printf("GGG\n"); /* XXX */
			printf("[D] Pointers: [new_pk]%p=>%p [head]%p=>%p\n", (void *)new_pk, (void *)(new_pk->next), (void *)(STRUCTURE.head), (void *)((*STRUCTURE.head).next)); /* XXX */
			if (same_flow(search_head, new_pk)) { /* If we found a flow that the new packet belongs to */
				printf("JJJ\n"); /* XXX */
				/* Connect the new packet to the packets before and after */
				new_pk->next = search_head->next;
				new_pk->prev = search_head->prev;
				/* Update the packet before and after to point on the new packet */
				search_head->prev->next = new_pk;
				search_head->next->prev = new_pk;
				/* Disconect the old packet */
				search_head->next = NULL;
				search_head->prev = NULL;
				/* Connect the old and the new packets with the up/down pointers */
				new_pk->down = search_head;
				search_head->up = new_pk;
				/* Was he the head? */
				if (STRUCTURE.head == search_head) {
					STRUCTURE.head = new_pk;
				}
				if (STRUCTURE.flow_pk->pktID == search_head->pktID) {
					STRUCTURE.flow_pk = new_pk;
				}
				/* Finish */
				STRUCTURE.count++; /* We added one packet to the data structure */
				printf("~~~END enqueue~~~\n"); /* XXX */
				return 0;
			}
			search_head = search_head->next; /* Move our search head to the next element */
		} while (search_head != STRUCTURE.head); /* Until we complets a single round over the list */
		/* Havn't found a flow that the new packet belongs to => Create a new one */
		new_pk->next = STRUCTURE.head;
		new_pk->prev = (*STRUCTURE.head).prev;
		(*(*STRUCTURE.head).prev).next = new_pk;
		(*STRUCTURE.head).prev = new_pk;
		/* Finish */
		STRUCTURE.count++; /* We added one packet to the data structure */
	}
	printf(" <<<<<<<<<<<<<<<< ended enqueue on pk - %d ", new_pk->pktID);
	printf("~~~END enqueue~~~\n"); /* XXX */
	return 0;
}
/* int dequeue(packet* pk) { }
 *
 * Receive pointer to the packet that need to be removed
 * Remove packet from our data structure
 * Return 0 on success & 1 on failure
 */
int dequeue(packet* pk) {
	if ((pk->next == NULL) || (pk->prev == NULL) || (pk->up != NULL)) { /* Invalid dequeue request */
		return 1;
	}
	if (STRUCTURE.count <= 1) { /* The last packet in our data structure */
		STRUCTURE.head = NULL;
		STRUCTURE.count = 0;
	} else { /* Disconnect that node from the chain */
		if (pk->down != NULL) { /* There are more packets in that flow */
			if (STRUCTURE.head == pk) {
				STRUCTURE.head = pk->down;
			}
			pk->prev->next = pk->down; /* Move the next packet to be the upper one */
			pk->next->prev = pk->down;
			pk->down->next = pk->next;
			pk->down->prev = pk->prev;
			pk->down->up = NULL;
		} else {/* There are no more packets in that flow */
			if (STRUCTURE.head == pk) {
				STRUCTURE.head = pk->next;
			}
			pk->prev->next = pk->next;
			pk->next->prev = pk->prev;
		}
	}
	/*free(pk); /* TODO TODO TODO WTF??? */
	STRUCTURE.count--;
	return 0;
}
/* packet* find_packet() { }
 *
 * Receive ??? XXX ??? XXX
 * ??? XXX ??? XXX
 * Return ??? XXX ??? XXX
 */
packet* find_packet() {
	if ((STRUCTURE.count == 4) && (STRUCTURE.head->pktID == 8)) { /* TODO XXX DELME XXX */
		printf("TODO DEBUG DELME XXX\n");/* TODO XXX DELME XXX */
	} /* TODO XXX DELME XXX */
	packet* search_head = STRUCTURE.head;
	do {
		if (same_flow(search_head, STRUCTURE.flow_pk)) { /* Head points to a packet from the correct flow */
			printf("[1/3] search_head->pktID %d\n", search_head->pktID); /* TODO XXX DELME */
			if (search_head->down == NULL) { /* No more pakets on flow */
				if (search_head->next != search_head){ /* Not last one on the stractue */
					STRUCTURE.flow_pk = search_head->next;
				}
			} else {
				while (search_head->down != NULL) {
					search_head = search_head->down;
				}
			}
			printf("[2/3] search_head->pktID %d\n", search_head->pktID); /* TODO XXX DELME */
			if (search_head->weight >= STRUCTURE.same_flow_send_count) { /* Sent less then requested by flow. */
				STRUCTURE.same_flow_send_count++;
				return  search_head;
			} else { /* Already sent more then enagth */
				/*if (STRUCTURE.flow_pk->next != STRUCTURE.flow_pk) { /* Not last one on the stractue */
				printf("[1] flow pk is - %d \n\n\n", STRUCTURE.flow_pk->pktID);
				STRUCTURE.flow_pk = STRUCTURE.flow_pk->next;
				printf("[2] flow pk is - %d \n\n\n", STRUCTURE.flow_pk->pktID);
				STRUCTURE.same_flow_send_count = 0;
				while (search_head->up != NULL) {
					search_head = search_head->up;
				}
			}
			printf("[3/3] search_head->pktID %d\n", search_head->pktID); /* TODO XXX DELME */
		}
		search_head = search_head->next;
	} while (1);
}
/* int send_packet() { }
 *
 * Receive ??? XXX ??? XXX
 * ??? XXX ??? XXX
 * Return ??? XXX ??? XXX
 */
int send_packet() { /* TODO TODO TODO  Write to output file */
	printf(" __________________________started send packet \n\n");
	/* find the pacekt to be sent*/
	packet* pk = find_packet();
	int return_time = (int)((pk->length) / QUANTUM);
	printf("%d",return_time); /* TODO XXX DELME XXX TODO */
	/* send packet */
	write_packet(pk);
	/* move weight up */
	if(pk->up != NULL){
		(*pk->up).weight = pk->weight;
	}
	/* delete packet */
	dequeue(pk);
	printf(" ------------------------- finished send packet \n\n");
	return return_time;
}
/* void print() { }
 *
 * Print the packets in line
 * For debugging and support use only!!!
 */
void print() {
	packet* pkt = STRUCTURE.head;
	packet* dive_in;
	if (pkt) {
		printf("~~~START print~~~\n"); /* XXX */
		printf("[A] Pointers: [pkt]%p=>%p [head]%p=>%p\n", (void *)pkt, (void *)pkt->next, (void *)(STRUCTURE.head), (void *)((*STRUCTURE.head).next)); /* XXX */
		printf("|========================================================================|\n");
		printf("| Current time=%-20ld Number of waiting packets=%-10i |\n", CLOCK, STRUCTURE.count);
		printf("|=========================|===========================|==================|\n");
		do {
			printf("| ID=%-20ld | Time=%-20ld | Length=%-5i     |\n", pkt->pktID, pkt->Time, pkt->length);
			dive_in = pkt->down;
			while (dive_in) {
				printf("|==ID=%-20ld| Time=%-20ld | Length=%-5i     |\n", dive_in->pktID, dive_in->Time, dive_in->length);
				dive_in = dive_in->down;
			}
			pkt = pkt->next; /* Move to the next element */
		} while (pkt != STRUCTURE.head); /* Until we complets a single round over the list */
		printf("|=========================|===========================|==================|\n");
		/*╔==================================╤=======================╤================╗*/
		/*║               Col1               │         Col2          │      Col3      ║*/
		/*╠==================================╪=======================╪================╣*/
		/*║                                  │                       │                ║*/
		/*║                                  │                       │                ║*/
		/*║                                  │                       │                ║*/
		/*╚==================================╧=======================╧================╝*/
		if (STRUCTURE.head) { printf("[H] Pointers: [head]%p=>%p\n", (void *)(STRUCTURE.head), (void *)((*STRUCTURE.head).next)); } /* XXX */
		printf("~~~END print~~~\n"); /* XXX */
	}
}
/* int main(int argc, char *argv[]) { }
 *
 * Receive command line arguments
 * Simulate round robin algorithm
 * Return the program exit code
 */
int main(int argc, char *argv[]) {
	/* Function variables */
	char errmsg[256];		/* The message to print in case of an error */
	int input_weight = 0;	/* The weight */
	int res = 0;			/* Temporary variable to store function response */
	long temp = 0;			/* Temporary variable */
	packet* last_packet;	/* Temporary variable for the last readed packet from the input file */
	packet* comper_packet; /* packet for same_flow perposes */
	/* Check correct call structure */
	if (argc != 6) {
		if (argc < 6) {
			printf(USAGE_OPERANDS_MISSING_MSG, argv[0]);
		}
		else {
			printf(USAGE_OPERANDS_SURPLUS_MSG, argv[0]);
		}
		return EXIT_FAILURE;
	}
	/* Check type => argv[1] */
	if ((strncmp(argv[1], "RR", 2) != 0) && (strncmp(argv[1], "DRR", 3) != 0)) {
		fprintf(stderr, F_ERROR_TYPE_MSG);
		return program_end(EXIT_FAILURE);
	}
	if (strncpy_s(TYPE, 4, argv[1], strlen(argv[1])) != 0) {
		fprintf(stderr, F_ERROR_TYPE_MSG);
		return program_end(EXIT_FAILURE);
	}
	/* Check input file => argv[2] */
	if ((IN_FILE = fopen(argv[2], "rb")) == NULL) { /* Upon successful completion ... return a FILE pointer. Otherwise, NULL is returned and errno is set to indicate the error. */
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_INPUT_FILE_MSG, argv[2], errmsg);
		return program_end(errno);
	}
	/* Check output file => argv[3] */
	if ((OUT_FILE = fopen(argv[3], "wb")) == NULL) { /* Upon successful completion ... return a FILE pointer. Otherwise, NULL is returned and errno is set to indicate the error. */
		strerror_s(errmsg, 255, errno);
		fprintf(stderr, F_ERROR_OUTPUT_FILE_MSG, argv[3], errmsg);
		return program_end(errno);
	}
	/* Check weight => argv[4] */
	if ((res = convert_strin2long(argv[4], &temp, 1, INT_MAX, F_ERROR_WEIGHT_INVALID_MSG)) > 0) {
		return program_end(res); /* Error occurred, Exit */
	}
	else {
		input_weight = (int)temp; /* This is secure because we alreade validate 'temp' max&min values */
	}
	/* Check quantum => argv[5] */
	if ((res = convert_strin2long(argv[5], &temp, 0, INT_MAX, F_ERROR_QUANTUM_INVALID_MSG)) > 0) {
 		return program_end(res); /* Error occurred, Exit */
	}
	else {
		QUANTUM = (int)temp; /* This is secure because we alreade validate 'temp' max&min values */
	}
	/* Init variables */
	STRUCTURE.head = NULL;
	STRUCTURE.count = 0;
	STRUCTURE.same_flow_send_count = 0;
	comper_packet = malloc(sizeof(packet)); /* TODO free memory & allocation check */
	STRUCTURE.flow_pk = comper_packet;
	last_packet = malloc(sizeof(packet)); /* TODO free memory & allocation check */
	QUANTUM = input_weight; /* TODO XXX DELME XXX TODO */
	QUANTUM = QUANTUM; /* TODO XXX DELME XXX TODO */
	/* Weighted Round Robin
	 * 1) If total weights is W, it takes W rounds to complete a cycle.
	 * 2) Each flow i transmits w[i] packets in a cycle.
	 *
	 * Deficit Round Robin
	 * 1) Each flow has a credit counter.
	 * 2) Credit counter is increased by “quantum” with every cycle.
	 * 3) A packet is sent only if there is enough credit.
	 */
	/* Start the clock :) */
	int readed = 0;
	while ( (readed = read_packet(last_packet, input_weight)) || (STRUCTURE.count > 0) ) { /* while there are more packets to work on */
		if (readed == 0) {
			last_packet->Time = LONG_MAX;
		}
		if ((readed == 0) || (last_packet->Time > CLOCK)) {
			while (last_packet->Time > CLOCK) {
				if (STRUCTURE.count > 0){
					CLOCK += send_packet();
				} else {
					CLOCK = last_packet->Time;
				}
			}
		}
		packet* new_pk = (packet *)malloc(sizeof(packet));
		copy_packet(last_packet, new_pk);
		if (enqueue(new_pk)) { /* Add the new packet to the data structure and keep reading for more packets with the same time */
			fprintf(stderr, F_ERROR_ENQUEUE_FAILED_MSG, last_packet->pktID);
			return program_end(EXIT_FAILURE); /* Error occurred in enqueue() */
		}
		print(); /* TODO DEBUG DELME XXX TODO */
	}
	/* Exit */
	return program_end(EXIT_SUCCESS);
}
