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

#include "Arduino.h"
#include "SoftwareSerial.h"

#ifndef _DFPLAYER_SOFTWARESERIAL_H_
#define _DFPLAYER_SOFTWARESERIAL_H_

class DFPlayer {
#define DFPLAYER_INFO
//#define DFPLAYER_DEBUG
#ifdef DFPLAYER_DEBUG
//#define DFPLAYER_HEAVY_DEBUG
#endif

#define BUFFER_LENGTH	10
	/*
	 * MAX_FIFO_SIZE
	 * Actually the real size of the DFPlayer serial FIFO queue is 68 bits.
	 * As each message sent weights 10 bytes, this FIFO can only contains 6
	 * full valid messages.
	 */
#define DFPLAYER_FIFO_SIZE   60

	/* OPERATING_DELAY
	 * Uncompressible delay needed for the DFPlayer to receive and apply commands correctly
	 * if this delay is reduced:
	 * - at best your DFPlayer won't handle the mp3Serial line correctly
	 *   resulting in unpredictable behaviour.
	 * - at worse it may crash the DFPlayer that will require a reset
	 * */
#define OPERATING_DELAY 20

	/**
	 * The value below comes from different source.
	 * See the number right after the comment
	 * 1 => From datasheet PDF
	 * 2 => From DFRobot wiki http://www.dfrobot.com/wiki/index.php/DFPlayer_Mini_SKU:DFR0299
	 * 3 => From original DFPlayer librarie V1.0 made by DFRobot
	 * 4 => From Personal reverse engineering
	 *
	 *
	 * All the value commented and noted UNKNOW :
	 * - were documented by DFRobot but doesn't produce what they are supposed to do
	 * OR
	 * - must exists but I don't know how to use them or what they do.
	 *
	 * Further reverse engineering to come on those.
	 */
#define NEXT 			0x01 //1 command "Next"
#define	PREVIOUS 		0x02 //1 command "Previous"
#define PLAY_TRACK		0x03 //1 command "Play specified track"(NUM) 0-2999
#define VOL_UP			0x04 //1 command "Increase volume"
#define VOL_DWN			0x05 //1 command "Decrease volume"
#define VOLUME			0x06 //1 command "Set specified volume"  range :0-30
#define EQUALIZER		0x07 //1 command "Set Specified EQ" (0/1/2/3/4/5) Normal/Pop/Rock/Jazz/Classic/Base
#define	PLAY_MODE  		0x08 //1 command "Set specified playback mode" (0/1/2/3) Repeat/folder repeat/single repeat/ random
#define SOURCE			0x09 //1 command "Set Specified playback source"(0/1/2/3/4) U/TF/AUX/SLEEP/FLASH
#define STANDBY			0x0A //1 command "Enter into standby/low power loss"
#define NORMAL			0x0B //1 command "Normal working"
#define	RESET			0x0C //1 command "Reset module"
#define PLAYBACK		0x0D //1 command "Playback"
#define	PAUSE			0x0E //1 command "Pause"
#define FOLDER  		0x0F //1 command "Play track in specified folder"
#define	VOL_ADJUST 		0x10 //1 command "Volume adjust set" {DH:Open volume adjust} {DL: set volume gain 0~31}
#define REPEAT 			0x11 //1 command "Repeat play" {1:start repeat play} {0:stop play}2).
#define	TRACK_FOLDER	0x12 //22 Specify MP3 tracks folder	0-9999
#define	ADVERT 			0x13 //2	Commercials	0-9999
//#define	UNKNOWN			0x14 //2	Support 15 folder
#define	BACKGROUND		0x15 //2	Stop playback, play background
#define	STOP			0x16 //2	Stop playback
//#define UNKNOWN			0x17 //4
//#define UNKNOWN			0x18 //4
#define	SINGLE_REPEAT	0x19 //3	Single repeat mode
//#define UNKNOWN			0x1A //4
//#define UNKNOWN			0x1B //4
//#define UNKNOWN			0x1C //4
//#define UNKNOWN			0x1D //4
//#define UNKNOWN			0x1E //4
//#define UNKNOWN			0x1F //4
//#define UNKNOWN			0x20 //4
//#define UNKNOWN			0x21 //4
//#define UNKNOWN			0x22 //4
//#define UNKNOWN			0x23 //4
//#define UNKNOWN			0x24 //4
//#define UNKNOWN			0x25 //4
//#define UNKNOWN			0x26 //4
//#define UNKNOWN			0x27 //4
//#define UNKNOWN			0x28 //4
//#define UNKNOWN			0x29 //4
//#define UNKNOWN			0x2A //4
//#define UNKNOWN			0x2B //4
//#define UNKNOWN			0x2C //4
//#define UNKNOWN			0x2D //4
//#define UNKNOWN			0x2E //4
//#define UNKNOWN			0x2F //4
//#define UNKNOWN			0x30 //4
//#define UNKNOWN			0x31 //4
//#define UNKNOWN			0x32 //4
//#define UNKNOWN			0x33 //4
//#define UNKNOWN			0x34 //4
//#define UNKNOWN			0x35 //4
//#define UNKNOWN			0x36 //4
//#define UNKNOWN			0x37 //4
//#define UNKNOWN			0x38 //4
//#define UNKNOWN			0x39 //4
#define	PLUG_IN 		0x3A //4
#define	PLUG_OUT 		0x3B //4
#define U_END_PLAY 		0x3C //2U device finished playing last track
#define TF_END_PLAY		0x3D //2TF device finished playing last track
#define	FLASH_END_PLAY 	0x3E //2STAY
#define	INIT 			0x3F //2Send initialization parameters 0 - 0x0F(each bit represent one device of the low-four bits)
#define	ERROR 			0x40 //2Returns an error, request retransmission
#define	REPLY 			0x41 //2Reply
#define	STATUS 			0x42 //2Query the current status
#define	QUERY_VOLUME	0x43 //2Query the current volume
#define	QUERY_EQ		0x44 //2Query the current EQ
#define	QUERY_PLAYMODE 	0x45 //2Query the current playback mode
#define	QUERY_VERSION 	0x46 //2Query the current software version
#define	QUERY_U_FILES 	0x47 //2Query the total number of TF card files
#define	QUERY_TF_FILES 	0x48 //2Query the total number of U-disk files
#define	QUERY_F_FILES	0x49 //2Query the total number of flash files
#define	QUERY_KEEPON 	0x4A //2Keep on
#define	QUERY_U_CUR		0x4B //2Queries the current track of U-Disk
#define	QUERY_TF_CUR 	0x4C //2Queries the current track of TF card
#define	QUERY_F_CUR 	0x4D //2Queries the current track of Flash
	/***************************************/

	/***
	 * Corrected value from personal tests
	 */

public:

	DFPlayer();
	~DFPlayer();

	SoftwareSerial* getSerial();
	void setSerial(uint8_t receivePin, uint8_t transmitPin, bool inverse_logic =
	false);

	uint8_t getDevice() const;
	void setDevice(uint8_t device);

	bool isNoReceiveBit() const;
	void setNoReceiveBit(bool noReceiveBit);

	long getFifoCount() const;
	long updateFifoCount();

	const uint8_t* getRecvBuffer() const;
	void resetRecvBuffer();
	uint8_t* getSendBuffer();
	void setSendBuffer(uint8_t cmd, uint16_t bit5 = 0, uint16_t bit4 = 0);
	//calculates checksum : 0 - (sum of bits 1 to 6)
	void checksum();
	void send();
	void receive();
	void execute();
	void printRecvBuffer();
	void printSendBuffer();
	void printHumanReadableRecvBuffer();
	void printHumanReadableSendBuffer();

	void reset();
	void setVolume(uint8_t value);
	void setEqualizer(uint8_t value);


	uint16_t playTrackFromDir(uint8_t track, uint8_t folder,
	bool returnPhysical = true);

	void setSingleLoop(bool value);

	uint16_t getCurrentTrack();


/*
 * IMPLEMENTED BUT NOT TESTED YET !!!
 * Be Aware those functions may not work as intended
 */
	void next();
	void previous();
	uint16_t playTrack(uint16_t value);
	void volumeUp();
	void volumeDown();
	void playMode(uint8_t value);
	uint16_t play();
	void pause();
	void stop();
	uint8_t countTrackInDir(uint8_t folder);
	void playAdvert(uint16_t value);
	void getStatus();
	uint16_t countTotalFiles();


private:
	SoftwareSerial * mp3Serial;
	uint8_t sendBuffer[10];
	uint8_t recvBuffer[10];
	uint8_t device;bool noReceiveBit;
	long fifoCount;
};
#endif // _DFPLAYER_SOFTWARESERIAL_H_
