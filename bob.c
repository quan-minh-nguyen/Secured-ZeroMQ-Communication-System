#include <stdio.h>
#include <stdlib.h>
#include <tomcrypt.h>
#include <zmq.h>

unsigned char *Bob_Receive (unsigned char receive[], int *receivelen, int limit);
void Bob_Send (unsigned char send[], int sendlen);
unsigned char* Read_File (char fileName[], int *fileLen);

void Write_Text (char name[], char text[]);

unsigned char* PRNG(unsigned char *seed, unsigned long seedlen, unsigned long prnlen);
unsigned char* Hash_SHA256(unsigned char* input, unsigned long inputlen);


int main(int argc, char* argv[]){
	int limit = 1000;
	unsigned char received[limit];
	int received_length = 0;
	unsigned char* received_message = Bob_Receive(received, &received_length, limit);
	
	int seed_length = 0;

	unsigned char* shared_seed = Read_File(argv[1], &seed_length);
	int secret_length = seed_length;
	unsigned char* secret_key = PRNG(shared_seed, seed_length, secret_length);
	
	unsigned char* plain_text = (unsigned char*)malloc(seed_length);
	 
	//XOR then Plaintext.txt
	for (int i=0; i<received_length; i++){
	plain_text[i] = received_message[i] ^ secret_key[i];}
	Write_Text("Plaintext.txt", plain_text);
	
	//convert hash to hex
	unsigned char* plain_hash = Hash_SHA256(plain_text, received_length);
	char* plain_hash_hex = (char*)malloc(2*received_length);
	for (int j=0; j<32; j++){
	sprintf(&plain_hash_hex[2*j], "%02x", plain_hash[j]);}
	Write_Text("Hash.txt", plain_hash_hex);
	
	
	//send back to Alice
	Bob_Send(plain_hash_hex, received_length);
	return 0;
}

void Write_Text(char name[], char text[]){
FILE *pFile;
pFile = fopen(name, "w");
if(pFile == NULL){
exit(0);}
fputs(text, pFile);
fclose(pFile);}



unsigned char *Bob_Receive (unsigned char receive[], int *receivelen, int limit)
{
void *context = zmq_ctx_new ();
//creates a socket to talk to Alice
void *responder = zmq_socket (context, ZMQ_REP);
//creates responder that receives the messages
int rc = zmq_bind (responder, "tcp://*:5555");
//make outgoing connection from socket
int received_length = zmq_recv (responder, receive, limit, 0);
//receive message from Alice
unsigned char *temp = (unsigned char*) malloc(received_length);
for(int i=0; i<received_length; i++){
temp[i] = receive[i];
}
*receivelen = received_length;
printf("Received Message: %s\n", receive);
printf("Size is %d\n", received_length-1);
return temp;
}


void Bob_Send (unsigned char send[], int sendlen)
{
void *context = zmq_ctx_new ();
//creates a socket to talk to Bob
void *requester = zmq_socket (context, ZMQ_REQ); //creates requester that sends the messages
zmq_connect (requester, "tcp://localhost:6666"); //make outgoing connection from socket
zmq_send (requester, send, sendlen, 0); //send msg to Bob
zmq_close (requester); //closes the requester socket
zmq_ctx_destroy (context);
//destroys the context & terminates all 0MQ processes
}


unsigned char* Read_File (char fileName[], int *fileLen)
{
FILE *pFile;
pFile = fopen(fileName, "r");
if (pFile == NULL){
printf("Error opening file.\n");
exit(0);}
fseek(pFile, 0L, SEEK_END);
int temp_size = ftell(pFile)+1;
fseek(pFile, 0L, SEEK_SET);
unsigned char* output = (unsigned char*) malloc(temp_size);
fgets(output, temp_size, pFile);
fclose(pFile);
*fileLen = temp_size-1;
return output;
}



unsigned char* PRNG(unsigned char *seed, unsigned long seedlen, unsigned long prnlen){
int err;
unsigned char *pseudoRandomNumber = (unsigned char*) malloc(prnlen);
prng_state prng;
//LibTomCrypt structure for PRNG
if ((err = chacha20_prng_start(&prng)) != CRYPT_OK){
//Sets up the PRNG state without a seed
printf("Start error: %s\n", error_to_string(err));}

if ((err = chacha20_prng_add_entropy(seed, seedlen, &prng)) != CRYPT_OK) {
//Uses a seed to add entropy to the PRNG
printf("Add_entropy error: %s\n", error_to_string(err));}

if ((err = chacha20_prng_ready(&prng)) != CRYPT_OK) {
//Puts the entropy into action
printf("Ready error: %s\n", error_to_string(err));}

chacha20_prng_read(pseudoRandomNumber, prnlen, &prng);

//Writes the result into pseudoRandomNumber[]
if ((err = chacha20_prng_done(&prng)) != CRYPT_OK) {
//Finishes the PRNG state
printf("Done error: %s\n", error_to_string(err));}

return (unsigned char*)pseudoRandomNumber;
}


unsigned char* Hash_SHA256(unsigned char* input, unsigned long inputlen){

unsigned char *hash_result = (unsigned char*) malloc(inputlen);
//int err;
hash_state md;
//LibTomCrypt structure for hash
sha256_init(&md);
//Initializing the hash set up
sha256_process(&md, (const unsigned char*)input, inputlen);
//Hashing the data given as input with specified length
sha256_done(&md, hash_result);
//Produces the hash (message digest)
return hash_result;

}


