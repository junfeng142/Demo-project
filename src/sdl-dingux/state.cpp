// Driver Save State module
#include "burner.h"
#include "zlib.h"

static UINT8* Comp = NULL;		// Compressed data buffer
static INT32 nCompLen = 0;
static INT32 nCompFill = 0;				// How much of the buffer has been filled so far
static z_stream Zstr;					// Deflate stream

UINT32 nReplayCurrentFrame;
UINT32 nStartFrame;

// If bAll=0 save/load all non-volatile ram to .fs
// If bAll=1 save/load all ram to .fs

// ------------ State len --------------------
static INT32 nTotalLen = 0;

static INT32 __cdecl StateLenAcb(struct BurnArea* pba)
{
	nTotalLen += pba->nLen;

	return 0;
}

static INT32 StateInfo(int* pnLen, int* pnMinVer, INT32 bAll)
{
	INT32 nMin = 0;
	nTotalLen = 0;
	BurnAcb = StateLenAcb;

	BurnAreaScan(ACB_NVRAM, &nMin);						// Scan nvram
	if (bAll) {
		INT32 m;
		BurnAreaScan(ACB_MEMCARD, &m);					// Scan memory card
		if (m > nMin) {									// Up the minimum, if needed
			nMin = m;
		}
		BurnAreaScan(ACB_VOLATILE, &m);					// Scan volatile ram
		if (m > nMin) {									// Up the minimum, if needed
			nMin = m;
		}
	}
	*pnLen = nTotalLen;
	*pnMinVer = nMin;

	return 0;
}

static INT32 CompEnlarge(INT32 nAdd)
{
	void* NewMem = NULL;

	// Need to make more room in the compressed buffer
	NewMem = realloc(Comp, nCompLen + nAdd);
	if (NewMem == NULL) {
		return 1;
	}

	Comp = (UINT8*)NewMem;
	memset(Comp + nCompLen, 0, nAdd);
	nCompLen += nAdd;

	return 0;
}

static INT32 CompGo(INT32 bFinish)
{
	INT32 nResult = 0;
	INT32 nAvailOut = 0;

	bool bRetry, bOverflow;

	do {

		bRetry = false;

		// Point to the remainder of out buffer
		Zstr.next_out = Comp + nCompFill;
		nAvailOut = nCompLen - nCompFill;
		if (nAvailOut < 0) {
			nAvailOut = 0;
		}
		Zstr.avail_out = nAvailOut;

		// Try to deflate into the buffer (there may not be enough room though)
		if (bFinish) {
			nResult = deflate(&Zstr, Z_FINISH);					// deflate and finish
			if (nResult != Z_OK && nResult != Z_STREAM_END) {
				return 1;
			}
		} else {
			nResult = deflate(&Zstr, 0);						// deflate
			if (nResult != Z_OK) {
				return 1;
			}
		}

		nCompFill = Zstr.next_out - Comp;						// Update how much has been filled

		// Check for overflow
		bOverflow = bFinish ? (nResult == Z_OK) : (Zstr.avail_out <= 0);

		if (bOverflow) {
			if (CompEnlarge(4 * 1024)) {
				return 1;
			}

			bRetry = true;
		}
	} while (bRetry);

	return 0;
}

static INT32 __cdecl StateCompressAcb(struct BurnArea* pba)
{
	// Set the data as the next available input
	Zstr.next_in = (UINT8*)pba->Data;
	Zstr.avail_in = pba->nLen;

	CompGo(0);													// Compress this Area

	Zstr.avail_in = 0;
	Zstr.next_in = NULL;

	return 0;
}

static INT32 __cdecl StateDecompressAcb(struct BurnArea* pba)
{
	Zstr.next_out =(UINT8*)pba->Data;
	Zstr.avail_out = pba->nLen;

	inflate(&Zstr, Z_SYNC_FLUSH);

	Zstr.avail_out = 0;
	Zstr.next_out = NULL;

	return 0;
}

// Compress a state using deflate
INT32 BurnStateCompress(UINT8** pDef, INT32* pnDefLen, INT32 bAll)
{
	void* NewMem = NULL;

	memset(&Zstr, 0, sizeof(Zstr));

	Comp = NULL; nCompLen = 0; nCompFill = 0;					// Begin with a zero-length buffer
	if (CompEnlarge(8 * 1024)) {
		return 1;
	}

	deflateInit(&Zstr, Z_DEFAULT_COMPRESSION);

	BurnAcb = StateCompressAcb;									// callback our function with each area

	if (bAll) BurnAreaScan(ACB_FULLSCAN | ACB_READ, NULL);		// scan all ram, read (from driver <- decompress)
	else      BurnAreaScan(ACB_NVRAM    | ACB_READ, NULL);		// scan nvram,   read (from driver <- decompress)

	// Finish off
	CompGo(1);

	deflateEnd(&Zstr);

	// Size down
	NewMem = realloc(Comp, nCompFill);
	if (NewMem) {
		Comp = (UINT8*)NewMem;
		nCompLen = nCompFill;
	}

	// Return the buffer
	if (pDef) {
		*pDef = Comp;
	}
	if (pnDefLen) {
		*pnDefLen = nCompFill;
	}

	return 0;
}

INT32 BurnStateDecompress(UINT8* Def, INT32 nDefLen, INT32 bAll)
{
	memset(&Zstr, 0, sizeof(Zstr));
	inflateInit(&Zstr);

	// Set all of the buffer as available input
	Zstr.next_in = (UINT8*)Def;
	Zstr.avail_in = nDefLen;

	BurnAcb = StateDecompressAcb;								// callback our function with each area

	if (bAll) BurnAreaScan(ACB_FULLSCAN | ACB_WRITE, NULL);		// scan all ram, write (to driver <- decompress)
	else      BurnAreaScan(ACB_NVRAM    | ACB_WRITE, NULL);		// scan nvram,   write (to driver <- decompress)

	inflateEnd(&Zstr);
	memset(&Zstr, 0, sizeof(Zstr));

	return 0;
}

// State load
INT32 BurnStateLoadEmbed(FILE* fp, INT32 nOffset, INT32 bAll, INT32 (*pLoadGame)())
{
	const char* szHeader = "FS1 ";						// Chunk identifier

	INT32 nLen = 0;
	INT32 nMin = 0, nFileVer = 0, nFileMin = 0;
	INT32 t1 = 0, t2 = 0;
	char ReadHeader[4];
	char szForName[33];
	INT32 nChunkSize = 0;
	UINT8 *Def = NULL;
	INT32 nDefLen = 0;									// Deflated version
	INT32 nRet = 0;

	if (nOffset >= 0) {
		fseek(fp, nOffset, SEEK_SET);
	} else {
		if (nOffset == -2) {
			fseek(fp, 0, SEEK_END);
		} else {
			fseek(fp, 0, SEEK_CUR);
		}
	}

	memset(ReadHeader, 0, 4);
	fread(ReadHeader, 1, 4, fp);						// Read identifier
	if (memcmp(ReadHeader, szHeader, 4)) {				// Not the right file type
		return -2;
	}

	fread(&nChunkSize, 1, 4, fp);
	if (nChunkSize <= 0x40) {							// Not big enough
		return -1;
	}

	INT32 nChunkData = ftell(fp);

	fread(&nFileVer, 1, 4, fp);							// Version of FB that this file was saved from

	fread(&t1, 1, 4, fp);								// Min version of FB that NV  data will work with
	fread(&t2, 1, 4, fp);								// Min version of FB that All data will work with

	if (bAll) {											// Get the min version number which applies to us
		nFileMin = t2;
	} else {
		nFileMin = t1;
	}

	fread(&nDefLen, 1, 4, fp);							// Get the size of the compressed data block

	memset(szForName, 0, sizeof(szForName));
	fread(szForName, 1, 32, fp);

	if (nBurnVer < nFileMin) {							// Error - emulator is too old to load this state
		return -5;
	}

	// Check the game the savestate is for, and load it if needed.
	{
		bool bLoadGame = false;

		if (nBurnDrvActive < nBurnDrvCount) {
			if (strcmp(szForName, BurnDrvGetTextA(DRV_NAME))) {	// The save state is for the wrong game
				bLoadGame = true;
			}
		} else {										// No game loaded
			bLoadGame = true;
		}

		if (bLoadGame) {
			UINT32 nCurrentGame = nBurnDrvActive;
			UINT32 i;
			for (i = 0; i < nBurnDrvCount; i++) {
				nBurnDrvActive = i;
				if (strcmp(szForName, BurnDrvGetTextA(DRV_NAME)) == 0) {
					break;
				}
			}
			if (i == nBurnDrvCount) {
				nBurnDrvActive = nCurrentGame;
				return -3;
			} else {
				if (nCurrentGame != nBurnDrvActive) {
					INT32 nOldActive = nBurnDrvActive;  // Exit current game if loading a state from another game
					nBurnDrvActive = nCurrentGame;
					BurnDrvExit();
					nBurnDrvActive = nOldActive;
				}
				if (pLoadGame == NULL) {
					return -1;
				}
				if (pLoadGame()) {
					return -1;
				}
			}
		}
	}

	StateInfo(&nLen, &nMin, bAll);
	if (nLen <= 0) {									// No memory to load
		return -1;
	}

	// Check if the save state is okay
	if (nFileVer < nMin) {								// Error - this state is too old and cannot be loaded.
		return -4;
	}

	fseek(fp, nChunkData + 0x30, SEEK_SET);				// Read current frame
	fread(&nReplayCurrentFrame, 1, 4, fp);
	nCurrentFrame = nStartFrame + nReplayCurrentFrame;

	fseek(fp, 0x0C, SEEK_CUR);							// Move file pointer to the start of the compressed block
	Def = (UINT8*)malloc(nDefLen);
	if (Def == NULL) {
		return -1;
	}
	memset(Def, 0, nDefLen);
	fread(Def, 1, nDefLen, fp);							// Read in deflated block

	nRet = BurnStateDecompress(Def, nDefLen, bAll);		// Decompress block into driver
	free(Def);											// free deflated block

	fseek(fp, nChunkData + nChunkSize, SEEK_SET);

	if (nRet)
		return -1;

   return 0;
}

// State load
INT32 BurnStateLoad(TCHAR* szName, INT32 bAll, INT32 (*pLoadGame)())
{
	const char szHeader[] = "FB1 ";						// File identifier
	char szReadHeader[4] = "";
	INT32 nRet = 0;

	FILE* fp = _tfopen(szName, _T("rb"));
	if (fp == NULL) {
		return 1;
	}

	fread(szReadHeader, 1, 4, fp);						// Read identifier
	if (memcmp(szReadHeader, szHeader, 4) == 0) {		// Check filetype
		nRet = BurnStateLoadEmbed(fp, -1, bAll, pLoadGame);
	}

	fclose(fp);

	if (nRet < 0)
		return -nRet;

   return 0;
}

// Write a savestate as a chunk of an "FB1 " file
// nOffset is the absolute offset from the beginning of the file
// -1: Append at current position
// -2: Append at EOF
INT32 BurnStateSaveEmbed(FILE* fp, INT32 nOffset, INT32 bAll)
{
	const char* szHeader = "FS1 ";						// Chunk identifier

	INT32 nLen = 0;
	INT32 nNvMin = 0, nAMin = 0;
	INT32 nZero = 0;
	char szGame[33];
	UINT8 *Def = NULL;
	INT32 nDefLen = 0;									// Deflated version
	INT32 nRet = 0;

	if (fp == NULL) {
		return -1;
	}

	StateInfo(&nLen, &nNvMin, 0);						// Get minimum version for NV part
	nAMin = nNvMin;
	if (bAll) {											// Get minimum version for All data
		StateInfo(&nLen, &nAMin, 1);
	}

	if (nLen <= 0) {									// No memory to save
		return -1;
	}

	if (nOffset >= 0) {
		fseek(fp, nOffset, SEEK_SET);
	} else {
		if (nOffset == -2) {
			fseek(fp, 0, SEEK_END);
		} else {
			fseek(fp, 0, SEEK_CUR);
		}
	}

	fwrite(szHeader, 1, 4, fp);							// Chunk identifier
	INT32 nSizeOffset = ftell(fp);						// Reserve space to write the size of this chunk
	fwrite(&nZero, 1, 4, fp);							//

	fwrite(&nBurnVer, 1, 4, fp);						// Version of FB this was saved from
	fwrite(&nNvMin, 1, 4, fp);							// Min version of FB NV  data will work with
	fwrite(&nAMin, 1, 4, fp);							// Min version of FB All data will work with

	fwrite(&nZero, 1, 4, fp);							// Reserve space to write the compressed data size

	memset(szGame, 0, sizeof(szGame));					// Game name
	sprintf(szGame, "%.32s", BurnDrvGetTextA(DRV_NAME));			//
	fwrite(szGame, 1, 32, fp);							//

	nReplayCurrentFrame = GetCurrentFrame() - nStartFrame;
	fwrite(&nReplayCurrentFrame, 1, 4, fp);					// Current frame

	fwrite(&nZero, 1, 4, fp);							// Reserved
	fwrite(&nZero, 1, 4, fp);							//
	fwrite(&nZero, 1, 4, fp);							//

	nRet = BurnStateCompress(&Def, &nDefLen, bAll);		// Compress block from driver and return deflated buffer
	if (Def == NULL) {
		return -1;
	}

	nRet = fwrite(Def, 1, nDefLen, fp);					// Write block to disk
	free(Def);											// free deflated block and close file

	if (nRet != nDefLen) {								// error writing block to disk
		return -1;
	}

	if (nDefLen & 3) {									// Chunk size must be a multiple of 4
		fwrite(&nZero, 1, 4 - (nDefLen & 3), fp);		// Pad chunk if needed
	}

	fseek(fp, nSizeOffset + 0x10, SEEK_SET);			// Write size of the compressed data
	fwrite(&nDefLen, 1, 4, fp);							//

	nDefLen = (nDefLen + 0x43) & ~3;					// Add for header size and align

	fseek(fp, nSizeOffset, SEEK_SET);					// Write size of the chunk
	fwrite(&nDefLen, 1, 4, fp);							//

	fseek (fp, 0, SEEK_END);							// Set file pointer to the end of the chunk

	return nDefLen;
}

// State save
INT32 BurnStateSave(TCHAR* szName, INT32 bAll)
{
	const char szHeader[] = "FB1 ";						// File identifier
	INT32 nLen = 0, nVer = 0;
	INT32 nRet = 0;

	if (bAll) {											// Get amount of data
		StateInfo(&nLen, &nVer, 1);
	} else {
		StateInfo(&nLen, &nVer, 0);
	}
	if (nLen <= 0) {									// No data, so exit without creating a savestate
		return 0;										// Don't return an error code
	}

	FILE* fp = _tfopen(szName, _T("wb"));
	if (fp == NULL) {
		return 1;
	}

	fwrite(&szHeader, 1, 4, fp);
	nRet = BurnStateSaveEmbed(fp, -1, bAll);

	fclose(fp);

	if (nRet < 0)
		return 1;

   return 0;
}

int nSavestateSlot = 0;
 
static char szSavestateName[MAX_PATH];

int StatedLoad(int nSlot)
{
	sprintf(szSavestateName, "%s/%s%i.sav", szAppSavePath, BurnDrvGetText(DRV_NAME), nSlot);
	printf("StatedLoad: %s\n", szSavestateName);
	return BurnStateLoad(szSavestateName, 1, &DrvInitCallback);
}

int StatedSave(int nSlot)
{
	sprintf(szSavestateName, "%s/%s%i.sav", szAppSavePath, BurnDrvGetText(DRV_NAME), nSlot);
	printf("StatedSave: %s\n", szSavestateName);
	return BurnStateSave(szSavestateName, 1);
}
