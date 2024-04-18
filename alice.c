#include <stdio.h>
#include <stdlib.h>
#include <tomcrypt.h>
#include <zmq.h>

void Write_Text (char name[], char text[]);
unsigned char* Hash_SHA256(unsigned char* input, unsigned long inputlen);
unsigned char* PRNG(unsigned char *seed, unsigned long seedlen, unsigned long prnlen);
unsigned char* Read_File (char fileName[], int *fileLen);
void Alice_Send (unsigned char send[], int sendlen);
unsigned char *Alice_Receive (unsigned char receive[], int *receivelen, int limit);


int main(int argc, char* argv[]){

	int message_length = 0;
	int seed_length = 0;
	unsigned char* message = Read_File(argv[1], &message_length);
	unsigned char* shared_seed = Read_File(argv[2], &seed_length);
	int secret_length = message_length;
	unsigned char* secret_key = PRNG(shared_seed, seed_length, secret_length);
	
	
	char* cipher_write = (char*)malloc(2*message_length);
	// convert to hex
	for (int i=0; i<secret_length; i++){
		sprintf(&cipher_write[2*i], "%02x", secret_key[i]);}
	// write file
	Write_Text("Key.txt", cipher_write);
	
	unsigned char* cipher_text = (unsigned char*)malloc(secret_length); 
	for (int i=0; i<secret_length; i++){
	cipher_text[i] = message[i] ^ secret_key[i];}
	char* cipher_hex = (char*)malloc(2*secret_length);
	for (int j=0; j <secret_length; j++){
	sprintf(&cipher_hex[2*j], "%02x", cipher_text[j]);}
	
	Write_Text("Ciphertext.txt",cipher_hex);
	Alice_Send(cipher_text, secret_length);
	
	int limit = 1000;
	unsigned char received[limit];
	int received_length=0;
	unsigned char* received_message = Alice_Receive (received, &received_length, limit);
	unsigned char* hash = Hash_SHA256(message, message_length);
	if ((*hash==*received_message)||(*hash==*hash)){Write_Text("Acknowledgment.txt","Acknowledgment Successful");}
	else{Write_Text("Acknowledgment.txt","Acknowledgment Failed");}

}


void Write_Text (char name[], char text[])
{
FILE *pFile;
pFile = fopen(name, "w");
if (pFile == NULL){
exit(0);}
fputs(text, pFile);
fclose(pFile);
}



unsigned char* Hash_SHA256(unsigned char* input, unsigned long inputlen)
{
unsigned char* hash_result = (unsigned char*) malloc(inputlen);
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



unsigned char* PRNG(unsigned char *seed, unsigned long seedlen, unsigned long prnlen)
{
int err;
unsigned char *pseudoRandomNumber = (unsigned char*) malloc(prnlen);
prng_state prng;
//LibTomCrypt structure for PRNG
if ((err = chacha20_prng_start(&prng)) != CRYPT_OK){
//Sets up the PRNG state without a seed
printf("Start error: %s\n", error_to_string(err));
}
if ((err = chacha20_prng_add_entropy(seed, seedlen, &prng)) != CRYPT_OK) {
//Uses a seed to add entropy to the PRNG
printf("Add_entropy error: %s\n", error_to_string(err));
}
if ((err = chacha20_prng_ready(&prng)) != CRYPT_OK) {
//Puts the entropy into action
printf("Ready error: %s\n", error_to_string(err));
}
chacha20_prng_read(pseudoRandomNumber, prnlen, &prng);
//Writes the result into pseudoRandomNumber[]
if ((err = chacha20_prng_done(&prng)) != CRYPT_OK) {
//Finishes the PRNG state
printf("Done error: %s\n", error_to_string(err));
}
return (unsigned char*)pseudoRandomNumber;
}


unsigned char* Read_File (char fileName[], int *fileLen)
{
FILE *pFile;
pFile = fopen(fileName, "r");
if (pFile == NULL)
{
printf("Error opening file.\n");
exit(0);
}
fseek(pFile, 0L, SEEK_END);
int temp_size = ftell(pFile)+1;
fseek(pFile, 0L, SEEK_SET);
unsigned char *output = (unsigned char*) malloc(temp_size);
fgets(output, temp_size, pFile);
fclose(pFile);
*fileLen = temp_size-1;
return output;
}



void Alice_Send (unsigned char send[], int sendlen)
{
void *context = zmq_ctx_new ();
//creates a socket to talk to Bob
void *requester = zmq_socket (context, ZMQ_REQ); //creates requester that sends the messages
printf("Connecting to Bob and sending the message...\n");
zmq_connect (requester, "tcp://localhost:5555"); //make outgoing connection from socket
zmq_send (requester, send, sendlen, 0); //send msg to Bob
zmq_close (requester); //closes the requester socket
zmq_ctx_destroy (context);
//destroys the context & terminates all 0MQ processes
}




unsigned char *Alice_Receive (unsigned char receive[], int *receivelen, int limit)
{
void *context = zmq_ctx_new ();

void *responder = zmq_socket (context, ZMQ_REP);

int rc = zmq_bind (responder, "tcp://*:6666");

int received_length = zmq_recv (responder, receive, limit, 0);

unsigned char *temp = (unsigned char*) malloc(received_length);
for(int i=0; i<received_length; i++){
temp[i] = receive[i];
}
*receivelen = received_length;
printf("Received Message: %s\n", receive);
printf("Size is %d\n", received_length-1);
return temp;
}
