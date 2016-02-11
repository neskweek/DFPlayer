/*
 * DFPlayer V 0.2
 * author: Sébastien CAPOU (neskweek@gmail.com)
 * Source : https://github.com/neskweek/LightSaberOS.
 * Date: 2016-02-11
 * Description:			library for DFPlayer mini sound board
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-sa/4.0/.
 */
#include <Arduino.h>
#include <DFPlayer_SoftwareSerial.h>
#include <SoftwareSerial.h>

//Puts uint16_bigend at the specified memory address
void fillData(uint8_t *buffer, uint16_t data) {
	*buffer = (uint8_t) (data >> 8);
	*(buffer + 1) = (uint8_t) data;
}

DFPlayer::DFPlayer() {
	this->device = 1;  // TFCard by default
	this->noReceiveBit = false;
	this->fifoCount = 0;
}

DFPlayer::~DFPlayer() {
	if (NULL != mp3Serial) {
		delete this->mp3Serial;
	}
}

SoftwareSerial * DFPlayer::getSerial() {
	return this->mp3Serial;
}

void DFPlayer::setSerial(uint8_t receivePin, uint8_t transmitPin,
bool inverse_logic/* = false*/) {

#ifdef DFPLAYER_INFO
	Serial.println(F("Connecting to DFPlayer..."));
#endif
	if (NULL != this->mp3Serial) {
		delete this->mp3Serial;
	}
	this->mp3Serial = new SoftwareSerial(receivePin, transmitPin,
			inverse_logic);
	this->mp3Serial->begin(9600);
	reset();
	if (this->mp3Serial->available()) {
#ifdef DFPLAYER_INFO
		Serial.println(F("Connected to DFPlayer !"));
#endif

	}

}

uint8_t DFPlayer::getDevice() const {
	return this->device;
}

void DFPlayer::setDevice(uint8_t device) {
	this->device = device;
}

bool DFPlayer::isNoReceiveBit() const {
	return this->noReceiveBit;
}

void DFPlayer::setNoReceiveBit(bool noReceiveBit) {
	this->noReceiveBit = noReceiveBit;
}

long DFPlayer::getFifoCount() const {
	return this->fifoCount;
}

long DFPlayer::updateFifoCount() {
	this->fifoCount = mp3Serial->available();
	return this->fifoCount;
}

const uint8_t* DFPlayer::getRecvBuffer() const {
	return this->recvBuffer;
}

void DFPlayer::resetRecvBuffer() {
	for (byte i = 0; i < BUFFER_LENGTH; i++) {
		this->recvBuffer[i] = 0x00;
	}
}

uint8_t* DFPlayer::getSendBuffer() {
	return this->sendBuffer;
}

void DFPlayer::setSendBuffer(uint8_t cmd, uint16_t bit6/*=0*/,
		uint16_t bit5/*=0*/) {
	if (not ((bit5 > 255) or (bit6 > 65535) or (bit6 > 255 and bit5 != 0))) {
		this->sendBuffer[0] = 0x7E; // start code
		this->sendBuffer[1] = 0xFF; // Version
		this->sendBuffer[2] = 0x06;	// Length (3+4+5+6)
		this->sendBuffer[3] = cmd; // Command
		this->sendBuffer[4] = 0x00;	// Command feedback
		if (!(bit5 == 0 && bit6 > 255)) { //256 = 0x01 0x00
			this->sendBuffer[5] = bit5; // High data byte - Command Argument 1 (complement) or Argument 2
			this->sendBuffer[6] = bit6; // Low data byte Command Argument 1
		} else if (bit5 == 0 && bit6 > 255) {
			fillData(this->sendBuffer + 5, bit6);
		}
		this->sendBuffer[7] = 0x00; // Cheksum
		this->sendBuffer[8] = 0x00;	// Cheksum
		this->sendBuffer[9] = 0xEF; // End bit
		checksum();
	} else {
		/*
		 * We fill the sendBuffer with bullshit
		 * this will generate an error message on DFPlayer::Serial
		 */
		for (byte i = 0; i < BUFFER_LENGTH; i++) {
			this->sendBuffer[i] = 0x00;
		}

	}
} //setSendBuffer

/*
 *calculates checksum : 0 - (sum of bits 1 to 6)
 */
void DFPlayer::checksum() {
	uint16_t sum = 0;
	for (int i = 1; i < 7; i++) {
		sum += this->sendBuffer[i];
	}
	fillData(this->sendBuffer + 7, -sum);
} //checksum

/*
 *
 */
void DFPlayer::send() {
	this->mp3Serial->write(this->sendBuffer, BUFFER_LENGTH);
#ifdef DFPLAYER_HEAVY_DEBUG
	printSendBuffer();
#endif
#ifdef DFPLAYER_DEBUG
	printHumanReadableSendBuffer();
#endif
	//waiting for command to be treated
	delay(OPERATING_DELAY);

	/*
	 while (this->fifoCount == 0
	 or (lastFifoCount == this->fifoCount and this->fifoCount != 0
	 or (this->fifoCount % BUFFER_LENGTH != 0))) {
	 Serial.print(F("Waiting for command "));
	 Serial.print(this->sendBuffer[3],HEX);
	 Serial.print(F(" to be treated\tfifoCount="));
	 Serial.println(this->fifoCount);
	 this->fifoCount = this->mp3Serial->available();
	 }
	 //#ifdef DFPLAYER_DEBUG
	 Serial.print(F("DFPLAYER Serial ready at "));
	 Serial.print(millis());
	 Serial.print(F("ms\tfifoCount="));
	 Serial.println(this->fifoCount);
	 receive();
	 printRecvBuffer();
	 //#endif
	 *
	 */

} //send

/*
 *
 */
void DFPlayer::receive() {
	resetRecvBuffer();

	while (!isNoReceiveBit() and this->mp3Serial->available()
			and (this->mp3Serial->available() % BUFFER_LENGTH == 0)
			and this->recvBuffer[0] != 0x7E) {
		mp3Serial->readBytesUntil(0xEF, this->recvBuffer, BUFFER_LENGTH);
#ifdef DFPLAYER_DEBUG
		this->printHumanReadableRecvBuffer();
#endif
#ifdef DFPLAYER_HEAVY_DEBUG
		this->printRecvBuffer();
#endif
	}
	this->fifoCount = this->mp3Serial->available();
	// sometimes the serial Line bug so we test we have positioned ourselves at the start of a valid buffer
	//*** due to mp3Serial FIFO queue overflow
	/*	if (this->recvBuffer[0] != 0x7E) {
	 resetRecvBuffer();
	 }
	 if (this->recvBuffer[3] == 0x7E) {
	 for (int i = 0; i < BUFFER_LENGTH - 3; i++) {
	 this->recvBuffer[i] = this->recvBuffer[i + 3];
	 }
	 }
	 */
} //receive

void DFPlayer::reset() {
	setSendBuffer(RESET);
	send();
	delay(1500); //Mandatory !!
}
void DFPlayer::setVolume(uint8_t value) {
	setSendBuffer(VOLUME, value);
	// this command never receive an answer !
	send();
}

void DFPlayer::setEqualizer(uint8_t value) {
	setSendBuffer(EQUALIZER, value);
	// this command never receive an answer !
	send();
}

uint16_t DFPlayer::playTrackFromDir(uint8_t track, uint8_t folder,
bool returnPhysical/*=true*/) {
	setSendBuffer(FOLDER, track, folder);
	send();
	delay(OPERATING_DELAY * 3);
	if (returnPhysical) {
		return getCurrentTrack();
	} else {
		return 0;
	}
}

void DFPlayer::setSingleLoop(bool value) {
	setSendBuffer(SINGLE_REPEAT, !value);  // !value o_0 ?
	// this command never receive an answer !
	// so we don't use send to avoid being stuck in an infinite loop
	send();
	//this->mp3Serial->write(this->sendBuffer, BUFFER_LENGTH);

}

uint16_t DFPlayer::getCurrentTrack() {
	uint8_t command;
	long result = 0;
	bool found = false;
	switch (this->device) {
	case 0: // U
		command = QUERY_U_CUR;
		break;
	case 1: // TFCard
		command = QUERY_TF_CUR;
		break;
	case 2: // SLEEP ????
		break;
	case 3: // FLASH
		command = QUERY_F_CUR;
		break;
	}

	setSendBuffer(command);
	send();

	while (this->mp3Serial->available()
			and (this->mp3Serial->available() % BUFFER_LENGTH == 0)) {
		receive();

		result = 256 * this->recvBuffer[5] + this->recvBuffer[6];
#ifdef DFPLAYER_HEAVY_DEBUG
		Serial.print(F("DFPLAYER getCurrentTrack searching; command="));
		Serial.print(this->recvBuffer[3], HEX);
		Serial.print(F(" track="));
		Serial.print(result);
		Serial.print(F(" millis="));
		Serial.print(millis());
		Serial.print(F("ms fifoCount="));
		Serial.println(this->fifoCount);
		printRecvBuffer();
#endif
		if (this->recvBuffer[3] == command) {
			found = true;

#ifdef DFPLAYER_HEAVY_DEBUG
			Serial.print(F("DFPLAYER getCurrentTrack command found="));
			Serial.print(this->recvBuffer[3], HEX);
			Serial.print(F("! Will return ="));
			Serial.print(256 * this->recvBuffer[5] + this->recvBuffer[6]);
			Serial.print(F("ms fifoCount="));
			Serial.println(this->fifoCount);
#endif

		}
	}
	if (!found) {
#ifdef DFPLAYER_HEAVY_DEBUG
		Serial.println(
				F(
						"DFPLAYER getCurrentTrack didn't found matching receive code"));
#endif
		// returning bullshit
	}
	return result;
}

#ifdef DFPLAYER_HEAVY_DEBUG
void DFPlayer::printRecvBuffer() {
	Serial.print(F("DFPLAYER Received at "));
	Serial.print(millis());
	Serial.print(F("ms:\t"));
	for (byte i = 0; i < BUFFER_LENGTH; i++) {
		Serial.print(this->recvBuffer[i], HEX);
		Serial.print(F(" ");
			}
			Serial.print(F("\tfifoCount="));
			Serial.print(this->fifoCount);
			Serial.println();

		}
		void DFPlayer::printSendBuffer() {
			Serial.print(F("DFPLAYER Sended at "));
			Serial.print(millis());
			Serial.print(F("ms:\t"));
			for (byte i = 0; i < BUFFER_LENGTH; i++) {
				Serial.print(this->sendBuffer[i], HEX);
				Serial.print(F(" ");
					}
					Serial.println();
				}

#endif
#ifdef DFPLAYER_DEBUG

void DFPlayer::printHumanReadableSendBuffer() {
	// processing outgoing message
	uint8_t command = this->sendBuffer[3];

	uint8_t highBit = this->sendBuffer[5];
	uint8_t lowBit = this->sendBuffer[6];
	uint16_t data = 256 * this->sendBuffer[5] + this->sendBuffer[6];

	switch (command) {
		case SINGLE_REPEAT:
		if (!lowBit)
		Serial.println(F("\t\t\tDFPlayer: Enabling single play mode "));
		else
		Serial.println(F("\t\t\tDFPlayer: Disabling single play mode "));
		break;
		case FOLDER:
		Serial.print(F("\t\t\tDFPlayer: Require play of file number "));
		Serial.print(lowBit);
		Serial.print(F(" in directory number "));
		Serial.println(highBit);
		break;
		case VOLUME:
		Serial.print(F("\t\t\tDFPlayer: Setting volume to "));
		Serial.println(lowBit);
		break;
		case RESET:
		Serial.println(F("\t\t\tDFPlayer: Asking for Reset..."));
		break;
		case QUERY_TF_CUR:
		case QUERY_U_CUR:
		case QUERY_F_CUR:
		Serial.println(F("\t\t\tDFPlayer: Query current playing track... "));
		break;
		default:
		// this recvBuffer needs to be added to the switch/case scenario
		Serial.print(F("\t\t\tDFPlayer: Unhandled sended message: "));
		for (byte i = 0; i < BUFFER_LENGTH; i++) {
			Serial.print(this->sendBuffer[i], HEX);
			Serial.print(F(" "));
		}

		break;
	}
} // end printHumanReadableRecvBuffer
void DFPlayer::printHumanReadableRecvBuffer() {
	// processing incoming message
	const char* errorText[] = {"Module busy", "Currently sleep mode",
		"Serial receiving error", "Checksum incorrect",
		"Specified track is out of current track scope",
		"Specified track is not found", "Advertise error",
		"SD card reading failed", "Entered into sleep mode"};

	uint8_t answer = this->recvBuffer[3];

	uint8_t highBit = this->recvBuffer[5];
	uint8_t lowBit = this->recvBuffer[6];
	uint16_t data = 256 * this->recvBuffer[5] + this->recvBuffer[6];

	switch (answer) {
		case ERROR:
		// error occurs
		Serial.print(F("\t\t\tDFPlayer: Error: "));
		Serial.println(errorText[lowBit - 1]);
		break;
		case PLUG_IN:

		if (lowBit == 2) { // sd card plugged in
			Serial.println(F("\t\t\tDFPlayer: SD card plugged in!"));
		}
		break;
		case PLUG_OUT:
		if (lowBit == 2) { // sd card plugged out
			Serial.println(F("\t\t\tDFPlayer: SD card plugged out!"));
		}
		break;
		case TF_END_PLAY:
		case U_END_PLAY:
		// track finished
		Serial.print(F("\t\t\tDFPlayer: Track finished playing: "));
		Serial.println(data);
		break;
		case INIT:
		if (lowBit == 2) { // sd card online (startup)
			Serial.println(F("\t\t\tDFPlayer: SD card online"));
		}
		break;
		case QUERY_TF_CUR:
		case QUERY_U_CUR:
		case QUERY_F_CUR:
		Serial.print(F("\t\t\tDFPlayer: Current track playing: "));
		Serial.println(data);
		break;
		default:
		// this recvBuffer needs to be added to the switch/case scenario
		Serial.print(F("\t\t\tDFPlayer: Unhandled received message: "));
		for (byte i = 0; i < BUFFER_LENGTH; i++) {
			Serial.print(this->recvBuffer[i], HEX);
			Serial.print(F(" "));
		}

		break;
	}
} // end printHumanReadableRecvBuffer
#endif

/*
 * IMPLEMENTED BUT NOT TESTED YET !!!
 * Be Aware those functions may not work as intended
 */
void DFPlayer::next() {
	setSendBuffer(NEXT);
	send();
}
void DFPlayer::previous() {
	setSendBuffer(PREVIOUS);
	send();
}
uint16_t DFPlayer::playTrack(uint16_t value) {
	setSendBuffer(PLAY_TRACK, value);
	send();
}
void DFPlayer::volumeUp() {
	setSendBuffer(VOL_UP);
	send();
}
void DFPlayer::volumeDown() {
	setSendBuffer(VOL_DWN);
	send();
}
void DFPlayer::playMode(uint8_t value) {
	setSendBuffer(PLAY_MODE,value);
	send();
}
uint16_t DFPlayer::play() {
	setSendBuffer(PLAYBACK);
	send();
}
void DFPlayer::pause() {
	setSendBuffer(PAUSE);
	send();
}
void DFPlayer::stop() {
	setSendBuffer(STOP);
	send();
}
uint8_t DFPlayer::countTrackInDir(uint8_t folder) {
	setSendBuffer(TRACK_FOLDER);
	send();
}
void DFPlayer::playAdvert(uint16_t value) {
	setSendBuffer(ADVERT);
	send();
}
void DFPlayer::getStatus() {
	setSendBuffer(STATUS);
	send();
}
uint16_t DFPlayer::countTotalFiles() {
	uint8_t command;
	switch (this->device) {
	case 0: // U
		command = QUERY_U_FILES;
		break;
	case 1: // TFCard
		command = QUERY_TF_FILES;
		break;
	case 2: // SLEEP ????
		break;
	case 3: // FLASH
		command = QUERY_F_FILES;
		break;
	}

	setSendBuffer(command);
	send();
}

