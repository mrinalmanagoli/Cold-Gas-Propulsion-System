#include <stdlib.h>

/*
This is to be added to the final firmware code of the Cortex controller.
This code is responsible for receiving data from the ground station, processing it and storing it in a buffer
*/

/*
PROTOTYPE: For sending data from satellite to ground station.
This is a stretch goal and only if time permits, for primarily sending sensor/gyro data from satellite to ground station
*/
void transmit_rf_data(uint8_t* rf_data, uint16_t payload_size);

/*
For receiving data from the ground station.
*/
void receive_rf_data(uint8_t* receive_rf_data);

int main(void) 
{
	
	MX_USART2_UART_Init();
	
	//Initial handshake, involves transmitting from satellite to ground station
	uint8_t rf_data = {0xFF};
	transmit_rf_data(rf_data, 1);
	
	//Receive code
	uint8_t receive_rf_data[]; //TODO: This is where received data is stored, it is a buffer
	
	//Call everytime data is to be received
	receive_rf_data(receive_rf_data, receive_size, payload);
	
}

/*
 * receive_rf_data: Array of byte values received by way of transmission
 * size: Number of bytes received
 * payload: An array (buffer) where the payload is stored
 * CAUTION: IT IS ASSUMED THAT THE LENGTH OF payload IS 8 BYTES
 */
void receive_rf_data(uint8_t* receive_rf_data)
{
	uint8_t upper_length_byte = 0;
	uint8_t lower_length_byte = 0;
	uint8_t i; //loop variable
	
	//First, determine what the length of the frame is (no escapes counted)
	if (receive_rf_data[1] == 0x7D)
	{
		upper_length_byte = receive_rf_data[2] ^ 0x20;
		i = 3;
	}
	else
	{
		upper_length_byte = receive_rf_data[1];
		i = 2;
	}
	
	if (receive_rf_data[i] == 0x7D)
	{
		lower_length_byte = receive_rf_data[i+1] ^ 0x20;
	}
	else
	{
		lower_length_byte = receive_rf_data[i];
	}
	
	uint16_t receive_size_without_escapes = (upper_length_byte << 8) + lower_length_byte + 4;
	uint16_t number_of_characters_escaped = 0;
	
	//Second, determine the number of escaped characters
	i = 1;
	while (i < receive_size_without_escapes + number_of_characters_escaped)
	{
		if (receive_rf_data[i] == 0x7D)
		{
				number_of_characters_escaped++;
		}
		
		i++;
	}
	
	//Number of received bytes is the frame length + number of escaped characters
	uint16_t receive_size = number_of_characters_escaped + receive_size_without_escapes;
	uint8_t receive_buffer[receive_size - number_of_characters_escaped];
	uint16_t index = 0;
	i = 1;
	receive_buffer[index++] = 0x7E;

	//Copy over data to a receive_buffer WITHOUT the escaped characters
	while (i < receive_size)
	{
		if (receive_rf_data[i] == 0x7D)
		{	//The next byte has to be demasked
			i++;

			//Demask receive_rf_data[i]
			receive_buffer[index++] = receive_rf_data[i] ^ 0x20;
		}
		else
		{	//Carry on
			receive_buffer[index++] = receive_rf_data[i];
		}
		i++;
	}
    
    Error_Vector.mag_error = (receive_buffer[17] << 24) + (receive_buffer[18] << 16) + (receive_buffer[19] << 8) + receive_buffer[20];
	Error_Vector.ang_error = (receive_buffer[21] << 24) + (receive_buffer[22] << 16) + (receive_buffer[22] << 8) + receive_buffer[23];
}

/*
 * rf_data: Array of Byte values of data to be transmitted
 * payload_size: Number of bytes to be transmitted
 */
void transmit_rf_data(uint8_t* rf_data, uint16_t payload_size)
{
	/*
	 * Construct frame containing:
	 * 1) Appropriate length field
	 * 2) RF data
	 * 3) Checksum
	 * 4) Before transmitting, frame must be escaped
	 */

	uint8_t transmit_buffer[18 + size];
	uint16_t number_of_characters_to_be_escaped = 0;

	transmit_buffer[0] = 0x7E; //Start delimiter
	transmit_buffer[1] = ((size + 14 ) >> 8) & 0xFF; //Length
	if (transmit_buffer[1] == 0x7E || transmit_buffer[1] == 0x7D || transmit_buffer[1] == 0x11 || transmit_buffer[1] == 0x13)
	{
		number_of_characters_to_be_escaped++;
	}

	transmit_buffer[2] = ((size + 14 ) >> 0) & 0xFF; //Length
	if (transmit_buffer[2] == 0x7E || transmit_buffer[2] == 0x7D || transmit_buffer[2] == 0x11 || transmit_buffer[2] == 0x13)
	{
		number_of_characters_to_be_escaped++;
	}

	transmit_buffer[3] = 0x10; //Frame type
	transmit_buffer[4] = 0x01; //Frame ID, kept fixed for simplicity
	transmit_buffer[5] = 0x00; //64 bit MAC
	transmit_buffer[6] = 0x13; //64 bit MAC
	number_of_characters_to_be_escaped++;
	transmit_buffer[7] = 0xA2; //64 bit MAC
	transmit_buffer[8] = 0x00; //64 bit MAC
	transmit_buffer[9] = 0x41; //64 bit MAC
	transmit_buffer[10] = 0x5B; //64 bit MAC
	transmit_buffer[11] = 0xAD; //64 bit MAC
	transmit_buffer[12] = 0x6C; //64 bit MAC
	transmit_buffer[13] = 0xFF; //16 bit MAC
	transmit_buffer[14] = 0xFE; //16 bit MAC
	transmit_buffer[15] = 0x00; //Broadcast radius
	transmit_buffer[16] = 0x00; //Options

	//RF data
	uint8_t i; //loop variable
	for (i = 0 ; i < size ; i++)
	{
		transmit_buffer[17 + i] = rf_data[i];

		if (rf_data[i] == 0x7E || rf_data[i] == 0x7D || rf_data[i] == 0x11 || rf_data[i] == 0x13)
		{
			number_of_characters_to_be_escaped++;
		}
	}

	//Checksum
	uint8_t sum = 0;
	for (i = 3 ; i < 18 + size - 1; i++)
	{
		sum += transmit_buffer[i];
	}

	transmit_buffer[18 + size - 1] = 0xFF - sum;
	if (transmit_buffer[18 + size - 1] == 0x7E || transmit_buffer[18 + size - 1] == 0x7D || transmit_buffer[18 + size - 1] == 0x11 || transmit_buffer[18 + size - 1] == 0x13)
	{
		number_of_characters_to_be_escaped++;
	}

	//Some bytes need to be escaped
	uint8_t transmit_buffer_escaped[18 + size+ number_of_characters_to_be_escaped];
	uint16_t index = 1;

	transmit_buffer_escaped[0] = 0x7E;
	for (i = 1 ; i < 18 + size ; i++)
	{
		if (transmit_buffer[i] == 0x7E || transmit_buffer[i] == 0x7D || transmit_buffer[i] == 0x11 || transmit_buffer[i] == 0x13)
		{
			transmit_buffer_escaped[index++] = 0x7D;
			transmit_buffer_escaped[index++] = 0x20 ^ transmit_buffer[i];
		}
		else
		{
			transmit_buffer_escaped[index++] = transmit_buffer[i];
		}
	}

	HAL_UART_Transmit(&huart2, transmit_buffer_escaped, 18 + size + number_of_characters_to_be_escaped, 1000);
	//You can probably do more shit with the return value from transmit, handle fail cases and all that...
}