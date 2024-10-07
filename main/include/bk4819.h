#ifndef BK4819_H_
#define BK4819_H_


#include <stdbool.h>
#include <stdint.h>


#include "bk4819-regs.h"

enum BK4819_AF_Type_t
{
	BK4819_AF_MUTE     =  0u,
	BK4819_AF_OPEN     =  1u,
	BK4819_AF_ALAM     =  2u,
	BK4819_AF_BEEP     =  3u,
	BK4819_AF_UNKNOWN1 =  4u,
	BK4819_AF_UNKNOWN2 =  5u,
	BK4819_AF_CTCO     =  6u,
	BK4819_AF_AM       =  7u,
	BK4819_AF_FSKO     =  8u,
	BK4819_AF_UNKNOWN3 =  9u,
	BK4819_AF_UNKNOWN4 = 10u,
	BK4819_AF_UNKNOWN5 = 11u,
	BK4819_AF_UNKNOWN6 = 12u,
	BK4819_AF_UNKNOWN7 = 13u,
	BK4819_AF_UNKNOWN8 = 14u,
	BK4819_AF_UNKNOWN9 = 15u
};

typedef enum BK4819_AF_Type_t BK4819_AF_Type_t;

enum BK4819_FilterBandwidth_t
{
	BK4819_FILTER_BW_WIDE = 0,
	BK4819_FILTER_BW_NARROW,
	BK4819_FILTER_BW_NARROWER
};

typedef enum BK4819_FilterBandwidth_t BK4819_FilterBandwidth_t;

enum BK4819_CssScanResult_t
{
	BK4819_CSS_RESULT_NOT_FOUND = 0,
	BK4819_CSS_RESULT_CTCSS,
	BK4819_CSS_RESULT_CDCSS
};

typedef enum BK4819_CssScanResult_t BK4819_CssScanResult_t;

// -----------------------------------------------------------------------------
void BK4819_Init(void);
void BK_SetFreq(uint32_t freq);
uint32_t Read_BK4819_Frequency(void);
void setup_PIN(void);
void BK4819_Init3(void);
uint16_t BK4819_ReadU16(void);
uint16_t BK4819_ReadRegister(BK4819_REGISTER_t Register);
void BK4819_WriteRegister(BK4819_REGISTER_t Register, uint16_t Data);
void BK4819_WriteU8(uint8_t Data);
void BK4819_WriteU16(uint16_t Data);
void BK4819_ToggleGpioOut(BK4819_GPIO_PIN_t Pin, bool bSet);
void BK4819_SetCDCSSCodeWord(uint32_t CodeWord);
void BK4819_SetCTCSSFrequency(uint32_t FreqControlWord);
void BK4819_SetTailDetection(const uint32_t freq_10Hz);
void BK4819_EnableVox(uint16_t VoxEnableThreshold, uint16_t VoxDisableThreshold);
void BK4819_SetFilterBandwidth(const BK4819_FilterBandwidth_t Bandwidth, const bool weak_no_different);
void BK4819_SetupPowerAmplifier(const uint8_t bias, const uint32_t frequency);
void BK4819_SetFrequency(uint32_t Frequency);
void BK4819_SetupSquelch(uint8_t SquelchOpenRSSIThresh, uint8_t SquelchCloseRSSIThresh, uint8_t SquelchOpenNoiseThresh, uint8_t SquelchCloseNoiseThresh, uint8_t SquelchCloseGlitchThresh, uint8_t SquelchOpenGlitchThresh);
void BK4819_SetAF(BK4819_AF_Type_t AF);
void BK4819_RX_TurnOn(void);
void BK4819_PickRXFilterPathBasedOnFrequency(uint32_t Frequency);
void BK4819_DisableScramble(void);
void BK4819_EnableScramble(uint8_t Type);
bool BK4819_CompanderEnabled(void);
void BK4819_SetCompander(const unsigned int mode);
void BK4819_DisableVox(void);
void BK4819_DisableDTMF(void);
void BK4819_EnableDTMF(void);
void BK4819_PlayTone(uint16_t Frequency, bool bTuningGainSwitch);
void BK4819_EnterTxMute(void);
void BK4819_ExitTxMute(void);
void BK4819_Sleep(void);
void BK4819_TurnsOffTones_TurnsOnRX(void);
void BK4819_SetupAircopy(void);
void BK4819_ResetFSK(void);
void BK4819_Idle(void);
void BK4819_ExitBypass(void);
void BK4819_PrepareTransmit(void);
void BK4819_TxOn_Beep(void);
void BK4819_ExitSubAu(void);
void BK4819_Conditional_RX_TurnOn_and_GPIO6_Enable(void);
void BK4819_EnterDTMF_TX(bool bLocalLoopback);
void BK4819_ExitDTMF_TX(bool bKeep);
void BK4819_EnableTXLink(void);
void BK4819_PlayDTMF(char Code);
void BK4819_PlayDTMFString(const char *pString, bool bDelayFirst, uint16_t FirstCodePersistTime, uint16_t HashCodePersistTime, uint16_t CodePersistTime, uint16_t CodeInternalTime);
void BK4819_TransmitTone(bool bLocalLoopback, uint32_t Frequency);
void BK4819_GenTail(uint8_t Tail);
void BK4819_EnableCDCSS(void);
void BK4819_EnableCTCSS(void);
uint16_t BK4819_GetRSSI(void);
uint8_t  BK4819_GetGlitchIndicator(void);
uint8_t  BK4819_GetExNoiceIndicator(void);
uint16_t BK4819_GetVoiceAmplitudeOut(void);
uint8_t BK4819_GetAfTxRx(void);
bool BK4819_GetFrequencyScanResult(uint32_t *pFrequency);
BK4819_CssScanResult_t BK4819_GetCxCSSScanResult(uint32_t *pCdcssFreq, uint16_t *pCtcssFreq);
void BK4819_DisableFrequencyScan(void);
void BK4819_EnableFrequencyScan(void);
void BK4819_SetScanFrequency(uint32_t Frequency);
void BK4819_Disable(void);
void BK4819_StopScan(void);
uint8_t BK4819_GetDTMF_5TONE_Code(void);
uint8_t BK4819_GetCDCSSCodeType(void);
uint8_t BK4819_GetCTCShift(void);
uint8_t BK4819_GetCTCType(void);
void BK4819_SendFSKData(uint16_t *pData);
void BK4819_PrepareFSKReceive(void);
void BK4819_PlayRoger(void);
void BK4819_PlayRogerMDC(void);
void BK4819_Enable_AfDac_DiscMode_TxDsp(void);
void BK4819_GetVoxAmp(uint16_t *pResult);
void BK4819_SetScrambleFrequencyControlWord(uint32_t Frequency);
void BK4819_PlayDTMFEx(bool bLocalLoopback, char Code);
void BK4819_Init2(void);
void BK4819_EnterFSK(void);

void BK4819_FskIdle();
void BK4819_EnterMdc();
void BK4819_ExitMdc();
unsigned char BK4819_MdcTransmit();
unsigned char BK4819_MdcReceive();

void RF_EnterFsk();
void RF_ExitFsk();
void RF_MyFskIdle();
unsigned char RF_MyFskTransmit();
unsigned char RF_MyFskReceive();


// -----------------------------------------------------------------------------
#define FSK_TX_RX_t unsigned char
enum {
	FSK_OFF = 0,
	FSK_TX,
	FSK_RX,
};
#define FSK_MODULATION_TYPE_t unsigned char
enum {
	FSK_MODULATION_TYPE_FSK1K2 = 0,	  // AFSK 1200 bps
	FSK_MODULATION_TYPE_FSK2K4,       // AFSK 2400 bps
	FSK_MODULATION_TYPE_MSK1200_1800, // MSK 1200 bps (aka FFSK)
	FSK_MODULATION_TYPE_MSK1200_2400, // MSK 2400 bps (aka FFSK)
};
#define FSK_NO_SYNC_BYTES_t unsigned char
enum  {
	FSK_NO_SYNC_BYTES_2 = 0,
	FSK_NO_SYNC_BYTES_4 = 1,
};
#define FSK_IRQ_t unsigned char
enum  {
	FSK_TX_FINISHED = 0,
	FSK_FIFO_ALMOST_EMPTY = 1,
	FSK_RX_FINISHED = 2,
	FSK_FIFO_ALMOST_FULL = 3,
	FSK_RX_SYNC = 4,
	FSK_OTHER = 9,
};
void BK4819_FskEnterMode(FSK_TX_RX_t txRx, FSK_MODULATION_TYPE_t fskModulationType, uint8_t fskTone2Gain, FSK_NO_SYNC_BYTES_t fskNoSyncBytes, uint8_t fskNoPreambleBytes, bool fskScrambleEnable, bool fskCrcEnable, bool fskInvertData);
void BK4819_FskExitMode(void);
void BK4819_FskIdle(void);
FSK_IRQ_t BK4819_FskCheckInterrupt(void);
int16_t BK4819_FskTransmitPacket(void * tx_buffer_ptr, uint16_t tx_packet_len_bytes);
unsigned char BK4819_FskReceivePacket(uint16_t * rx_buffer_ptr);

unsigned char RF_FskReceive();
unsigned char RF_FskTransmit();
void RF_FskIdle();
void RF_ExitFsk();
void RF_EnterFsk();
#endif