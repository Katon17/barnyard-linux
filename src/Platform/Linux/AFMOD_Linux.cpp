// Native SDL FSB mixer derived from OpenBarnyard AFMOD_WASM.cpp at the pinned
// commit. Upstream decompilation/provenance and license: third_party/.
// This is an FSB3 subset implementation, NOT a binary FMOD library.
#include "pch.h"
#include "NativeAudio.h"

#include <fmod/fmod.h>

#include <File/TFile.h>



#include <SDL2/SDL.h>
#include <chrono>
#include <atomic>
#include <new>
#include <cstddef>
#include <algorithm>
#include <filesystem>
#include <cmath>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <string.h>

//-----------------------------------------------------------------------------
// Enables memory debugging.
// Note: Should be the last include!
//-----------------------------------------------------------------------------
#include <Core/TMemoryDebugOn.h>

TOSHI_NAMESPACE_USING

// FMOD callers pass MakeInternalPath results, including packed-bank offsets.
// Convert only validated game/user absolute paths back to the virtual TFile root.
static TFile* OpenAudioFile(const char* name) {
    if(!name || !*name) return nullptr;
    namespace fs=std::filesystem;
    try {
        fs::path path(name);
        if(!path.is_absolute()) return TFile::Create(name);
        path=fs::canonical(path);
        for(const char* env:{"BARNYARD_GAME_DIR","BARNYARD_USER_DIR"}) {
            const char* value=getenv(env);if(!value || !*value)continue;
            auto relative=path.lexically_relative(fs::canonical(value));
            if(!relative.empty() && !relative.is_absolute() && *relative.begin()!="..") return TFile::Create(relative.string().c_str());
        }
    } catch(const std::exception&) {}
    return nullptr;
}

static constexpr TINT MIX_RATE=44100, MIX_CHUNK=512, MAX_CHANNELS=64;
static constexpr TINT MAX_VOICES=4, BLOCK_FRAMES=64, BLOCK_BYTES=0x24;
static constexpr TUINT8 FSB_BASICHEADERS=2;
static double AudioNow() { return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static SDL_AudioDeviceID s_device=0;
static TBOOL s_muted=TFALSE;
static int s_bufferMs=12,s_streamBufferMs=2000;
static BarnyardLinux::AudioSnapshot s_snapshot{};
static float s_distanceFactor=1,s_rolloffFactor=1,s_dopplerFactor=1;
static std::atomic<unsigned> s_currentBytes{},s_peakBytes{};
struct alignas(std::max_align_t) AudioHeader { size_t size; };
static void* AudioAlloc(size_t size) {
    if(size>256u*1024*1024) throw std::bad_alloc();
    auto* p=static_cast<AudioHeader*>(malloc(size+sizeof(AudioHeader)));
    if(!p) throw std::bad_alloc();p->size=size;
    unsigned now=s_currentBytes.fetch_add(size)+size, peak=s_peakBytes.load();
    while(now>peak && !s_peakBytes.compare_exchange_weak(peak,now)) {}
    return p+1;
}
static void AudioFree(void* data) { if(!data)return;auto* p=static_cast<AudioHeader*>(data)-1;s_currentBytes.fetch_sub(p->size);free(p); }

struct SampleInfo
{
	TCHAR   szName[ 32 ];
	TUINT32 uiLength;
	TUINT32 uiCompressed;
	TUINT32 uiLoopStart;
	TUINT32 uiLoopEnd;
	TUINT32 uiMode;
	TINT    iFrequency;
	TINT    iVolume;
	TINT    iPan;
	TINT    iPriority;
	TINT    iChannels;
	TFLOAT  fMinDistance;
	TFLOAT  fMaxDistance;
	TUINT32 uiOffset;
};

struct FMUSIC_MODULE;

struct FSOUND_SAMPLE
{
	SampleInfo     oInfo;
	TUINT32        uiMode;
	FMUSIC_MODULE* pOwner;
	const TUINT8*  pData;
	TINT16*        pDecoded; // interleaved, on the first play
};

struct FMUSIC_MODULE
{
	TUINT8*        pBytes;
	TINT           iNumSamples;
	FSOUND_SAMPLE* pSamples;
};

struct FSOUND_STREAM
{
	TCHAR       szPath[ 256 ];
	TUINT8*     pMemory;
	TUINT32     uiBase;
	SampleInfo* pSubs;
	TINT        iNumSubs;
	TINT        iSub;
	TUINT32     uiMode;
	TINT        iChannel;
	TINT        iGeneration;
	TUINT32     uiTimeMs;
	TFile*      pFile;
	TINT        iCachedBlock;
	TINT16      aiBlock[ BLOCK_FRAMES * 4 ];

	// What FSOUND_GetCurrentSample answers for a stream: captions are found by the name of the line
	FSOUND_SAMPLE oCurrentSample;
};

struct Channel
{
	TINT           iGeneration;
	FSOUND_SAMPLE* pSample;
	FSOUND_STREAM* pStream;
	const SampleInfo* pInfo;
	TBOOL          bReserved;
	TBOOL          bPaused;
	TBOOL          b3D;
	TBOOL          bBackwards;
	TUINT32        uiLoop;
	TINT           iNumVoices;
	TINT           aiVolumes[ MAX_VOICES ];
	TINT           aiPans[ MAX_VOICES ];
	TINT           iFrequency;
	TINT           iPriority;
	double         dPosition;
	TFLOAT         afPosition[ 3 ];
    TFLOAT afVelocity[3];
	TFLOAT         fMinDistance;
	TFLOAT         fMaxDistance;
	double         dRequested; // when the game asked for it to play, until the mixer starts it
};

static Channel         s_aChannels[ MAX_CHANNELS ];
static TINT            s_iNextGeneration = 1;
static pthread_mutex_t s_oMutex          = PTHREAD_MUTEX_INITIALIZER;

static TBOOL           s_bMixing;
static TFLOAT          s_afListener[ 3 ];
static TFLOAT s_listenerVelocity[3];
static TFLOAT          s_afRight[ 3 ] = { 1.0f, 0.0f, 0.0f };


class ScopedAudioLock
{
public:
	ScopedAudioLock() { pthread_mutex_lock( &s_oMutex ); }
	~ScopedAudioLock() { pthread_mutex_unlock( &s_oMutex ); }
};

//-----------------------------------------------------------------------------
// FSB3
//-----------------------------------------------------------------------------

static TUINT32 Read32( const TUINT8* a_pData ) { TUINT32 uiValue; memcpy( &uiValue, a_pData, 4 ); return uiValue; }
static TUINT16 Read16( const TUINT8* a_pData ) { TUINT16 uiValue; memcpy( &uiValue, a_pData, 2 ); return uiValue; }

// The sample headers, the offsets are from the start of the bank
static TINT ParseFSB( const TUINT8* a_pData, TUINT32 a_uiSize, SampleInfo** a_ppOutInfos )
{
	if ( a_uiSize < 24 || memcmp( a_pData, "FSB3", 4 ) != 0 )
		return 0;

	const TINT    iNumSamples = TINT( Read32( a_pData + 4 ) );
	const TUINT32 uiHeaders   = Read32( a_pData + 8 );
	const TUINT32 uiBankMode  = Read32( a_pData + 20 );
    const uint64_t dataBytes=Read32(a_pData+12);
    if(iNumSamples<1 || iNumSamples>16384 || uiHeaders>a_uiSize-24 || uint64_t(uiHeaders)+24+dataBytes>UINT32_MAX) return 0;

	SampleInfo* pInfos = TSTATICCAST( SampleInfo, AudioAlloc( sizeof( SampleInfo ) * iNumSamples ) );
	TUINT32     uiPos  = 24;

    int sampleIndex=-1;
    auto bad=[&](const char* why){
        fprintf(stderr,"Native FSB3 rejected: %s; sample=%d count=%d headers=%u data=%llu\n",
            why,sampleIndex,iNumSamples,uiHeaders,(unsigned long long)dataBytes);
        AudioFree(pInfos);return 0;
    };
    for ( TINT i = 0; i < iNumSamples; i++ )
    {
        sampleIndex=i;
        if(uiPos>24+uiHeaders || 24+uiHeaders-uiPos<8) return bad("truncated sample header");
		SampleInfo& rInfo = pInfos[ i ];

		if ( i > 0 && ( uiBankMode & FSB_BASICHEADERS ) )
		{
			rInfo              = pInfos[ 0 ];
			rInfo.uiLength     = Read32( a_pData + uiPos );
			rInfo.uiCompressed = Read32( a_pData + uiPos + 4 );
			uiPos += 8;
			continue;
		}

		const TUINT8* pHeader = a_pData + uiPos;
		const TUINT16 uiSize  = Read16( pHeader );
        if(uiSize<64 || uiSize>24+uiHeaders-uiPos) return bad("invalid sample header size");

		TUtil::MemCopy( rInfo.szName, pHeader + 2, 30 );
		rInfo.szName[ 30 ] = '\0';
		rInfo.uiLength     = Read32( pHeader + 32 );
		rInfo.uiCompressed = Read32( pHeader + 36 );
		rInfo.uiLoopStart  = Read32( pHeader + 40 );
		rInfo.uiLoopEnd    = Read32( pHeader + 44 );
		rInfo.uiMode       = Read32( pHeader + 48 );
		rInfo.iFrequency   = TINT( Read32( pHeader + 52 ) );
		rInfo.iVolume      = Read16( pHeader + 56 );
		rInfo.iPan         = TINT16( Read16( pHeader + 58 ) );
		rInfo.iPriority    = Read16( pHeader + 60 );
		rInfo.iChannels    = TMath::Max( TINT( Read16( pHeader + 62 ) ), 1 );

		TUINT32 uiMin = 0, uiMax = 0;

		if ( uiSize >= 72 )
		{
			uiMin = Read32( pHeader + 64 );
			uiMax = Read32( pHeader + 68 );
			memcpy( &rInfo.fMinDistance, &uiMin, 4 );
			memcpy( &rInfo.fMaxDistance, &uiMax, 4 );
		}
		else
		{
			rInfo.fMinDistance = 1.0f;
			rInfo.fMaxDistance = 1000000.0f;
		}

		uiPos += uiSize;
	}

	TUINT32 uiOffset = 24 + uiHeaders;

	for ( TINT i = 0; i < iNumSamples; i++ )
	{
        const auto& info=pInfos[i];
        sampleIndex=i;
        if(info.uiMode&(FSOUND_VAG|FSOUND_GCADPCM|FSOUND_IT214|FSOUND_IT215)) return bad("unsupported codec");
        // Streaming never allocates uiLength worth of PCM. Long music must not
        // inherit the old arbitrary 16M-frame eager-sample limit. Mixer indices
        // remain signed 32-bit; encoded-length arithmetic below is 64-bit.
        if(info.iChannels<1 || info.iChannels>MAX_VOICES || info.iFrequency<4000 || info.iFrequency>192000 || info.uiLength<1 || info.uiLength>INT32_MAX || info.iVolume>255 || info.iPan>255) return bad("invalid channels/rate/frame count/volume/pan");
        uint64_t needed=(info.uiMode&FSOUND_IMAADPCM) ? ((uint64_t(info.uiLength)+63)/64)*BLOCK_BYTES*info.iChannels : uint64_t(info.uiLength)*info.iChannels*((info.uiMode&FSOUND_8BITS)?1:2);
        if(needed>info.uiCompressed || uint64_t(uiOffset)+info.uiCompressed>24ull+uiHeaders+dataBytes) return bad("encoded sample exceeds declared payload");
        pInfos[ i ].uiOffset = uiOffset;
        uiOffset += info.uiCompressed;
	}

	*a_ppOutInfos = pInfos;
	return iNumSamples;
}

static const TINT16 s_aImaSteps[ 89 ] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
	157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552,
	1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
	12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const TINT8 s_aImaIndex[ 8 ] = { -1, -1, -1, -1, 2, 4, 6, 8 };

// One block of Xbox style IMA ADPCM: multichannel samples keep their headers split and their data in 2 byte groups
static void DecodeImaBlock( const SampleInfo& a_rInfo, const TUINT8* a_pBlock, TINT16* a_pOut )
{
	const TINT  iChannels = a_rInfo.iChannels;
	const TBOOL bMulti    = ( a_rInfo.uiMode & FSOUND_MULTICHANNEL ) != 0 || iChannels > 2;
	const TUINT8* pData   = a_pBlock + 4 * iChannels;

	for ( TINT c = 0; c < iChannels; c++ )
	{
		const TINT iHeader = bMulti ? 2 * c : 4 * c;
		const TINT iStep   = bMulti ? 2 * iChannels + 2 * c : iHeader + 2;

		TINT iPrediction = TINT16( Read16( a_pBlock + iHeader ) );
		TINT iIndex      = TMath::Min( TINT( a_pBlock[ iStep ] ), 88 );

		a_pOut[ c ] = TINT16( iPrediction );

		for ( TINT i = 1; i < BLOCK_FRAMES; i++ )
		{
			const TINT k = i - 1;

			const TINT iAt = bMulti ?
			    ( k >> 2 ) * 2 * iChannels + 2 * c + ( ( k & 3 ) >> 1 ) :
			    ( k >> 3 ) * 4 * iChannels + 4 * c + ( ( k & 7 ) >> 1 );

			const TINT iNibble = ( pData[ iAt ] >> ( ( k & 1 ) << 2 ) ) & 15;
			const TINT iSize   = s_aImaSteps[ iIndex ];

			TINT iDiff = iSize >> 3;
			if ( iNibble & 1 ) iDiff += iSize >> 2;
			if ( iNibble & 2 ) iDiff += iSize >> 1;
			if ( iNibble & 4 ) iDiff += iSize;

			iPrediction = ( iNibble & 8 ) ? TMath::Max( iPrediction - iDiff, -32768 ) : TMath::Min( iPrediction + iDiff, 32767 );
			iIndex      = TMath::Max( 0, TMath::Min( 88, iIndex + s_aImaIndex[ iNibble & 7 ] ) );

			a_pOut[ i * iChannels + c ] = TINT16( iPrediction );
		}
	}
}

static void GetLoopRange( const SampleInfo& a_rInfo, double& a_rdStart, double& a_rdEnd )
{
	const TUINT32 uiEnd = ( a_rInfo.uiLoopEnd > a_rInfo.uiLoopStart && a_rInfo.uiLoopEnd < a_rInfo.uiLength ) ? a_rInfo.uiLoopEnd + 1 : a_rInfo.uiLength;

	a_rdStart = TMath::Min( a_rInfo.uiLoopStart, uiEnd > 0 ? uiEnd - 1 : 0 );
	a_rdEnd   = uiEnd;
}

static TINT16* DecodeSampleData( const FSOUND_SAMPLE* a_pSample )
{
	const SampleInfo& rInfo     = a_pSample->oInfo;
	const TINT        iChannels = rInfo.iChannels;

    const uint64_t decodedBytes=(uint64_t(rInfo.uiLength)+BLOCK_FRAMES)*iChannels*sizeof(TINT16);
    if(decodedBytes>256ull*1024*1024) {
        fprintf(stderr,"Native FSB3: eager sample decode exceeds 256 MiB; use streaming\n");
        return TNULL;
    }
	TINT16* pDecoded = TSTATICCAST( TINT16, AudioAlloc( size_t(decodedBytes) ) );

    TUtil::MemClear(pDecoded, rInfo.uiLength*iChannels*2 + BLOCK_FRAMES*iChannels*2);
	if ( rInfo.uiMode & FSOUND_IMAADPCM )
	{
		const TUINT32 uiBlockBytes = BLOCK_BYTES * iChannels;
		const TUINT32 uiNumBlocks  = rInfo.uiCompressed / uiBlockBytes;

		for ( TUINT32 b = 0; b < uiNumBlocks && b * BLOCK_FRAMES < rInfo.uiLength; b++ )
			DecodeImaBlock( rInfo, a_pSample->pData + b * uiBlockBytes, pDecoded + b * BLOCK_FRAMES * iChannels );
	}
	else if ( rInfo.uiMode & FSOUND_8BITS )
	{
		const TBOOL bUnsigned = ( rInfo.uiMode & FSOUND_UNSIGNED ) != 0;

		for ( TUINT32 i = 0; i < rInfo.uiLength * iChannels; i++ )
			pDecoded[ i ] = bUnsigned ? TINT16( ( TINT( a_pSample->pData[ i ] ) - 128 ) * 256 ) : TINT16( TINT8( a_pSample->pData[ i ] ) * 256 );
	}
	else
	{
		TUtil::MemCopy( pDecoded, a_pSample->pData, TMath::Min( rInfo.uiCompressed, rInfo.uiLength * iChannels * 2 ) );
	}

	return pDecoded;
}

//-----------------------------------------------------------------------------
// Mixing
//-----------------------------------------------------------------------------

static TBOOL IsMultichannel( const SampleInfo& a_rInfo )
{
	return ( a_rInfo.uiMode & FSOUND_MULTICHANNEL ) != 0 || a_rInfo.iChannels > 2;
}

static const TINT16* GetStreamFrame( FSOUND_STREAM* a_pStream, const SampleInfo& a_rInfo, TINT a_iFrame )
{
	const TINT iChannels = a_rInfo.iChannels;
	const TINT iBlock    = a_iFrame / BLOCK_FRAMES;

	if ( iChannels > MAX_VOICES )
		return TNULL;

	if ( iBlock != a_pStream->iCachedBlock )
	{
		const TBOOL   bCompressed  = ( a_rInfo.uiMode & FSOUND_IMAADPCM ) != 0;
		const TUINT32 uiBlockBytes = bCompressed ? BLOCK_BYTES * iChannels : BLOCK_FRAMES * iChannels * sizeof( TINT16 );
		const TUINT32 uiOffset     = a_pStream->uiBase + a_rInfo.uiOffset + iBlock * uiBlockBytes;

		TUINT8  aCompressed[ BLOCK_BYTES * MAX_VOICES ];
		TUINT8* pBlock = bCompressed ? aCompressed : TREINTERPRETCAST( TUINT8*, a_pStream->aiBlock );
        memset(pBlock,0,uiBlockBytes);

		if ( a_pStream->pMemory )
		{
            const auto remaining=a_rInfo.uiCompressed-TUINT32(iBlock)*uiBlockBytes;
            TUtil::MemCopy(pBlock,a_pStream->pMemory+uiOffset-a_pStream->uiBase,std::min(remaining,uiBlockBytes));
		}
		else
		{
			if ( !a_pStream->pFile )
				a_pStream->pFile = OpenAudioFile( a_pStream->szPath );

			if ( !a_pStream->pFile )
				return TNULL;

            if(!a_pStream->pFile->Seek(uiOffset,TSEEK_SET)) return TNULL;
            const auto remaining=a_rInfo.uiCompressed-TUINT32(iBlock)*uiBlockBytes;
            const auto needed=std::min(remaining,uiBlockBytes);
            if(a_pStream->pFile->Read(pBlock,needed)!=needed) return TNULL;
		}

		if ( bCompressed )
			DecodeImaBlock( a_rInfo, aCompressed, a_pStream->aiBlock );

		a_pStream->iCachedBlock = iBlock;
	}

	return a_pStream->aiBlock + ( a_iFrame % BLOCK_FRAMES ) * iChannels;
}

static void MixChannel( Channel& a_rChannel, TFLOAT* a_pOut, TINT a_iFrames )
{
	const SampleInfo& rInfo = *a_rChannel.pInfo;

	if ( a_rChannel.pSample && !a_rChannel.pSample->pDecoded )
		a_rChannel.pSample->pDecoded = DecodeSampleData( a_rChannel.pSample );

	TFLOAT fGain = a_rChannel.aiVolumes[ 0 ] / 255.0f;
	TFLOAT fPan  = a_rChannel.aiPans[ 0 ] < 0 ? 0.5f : a_rChannel.aiPans[ 0 ] / 255.0f;

	if ( a_rChannel.b3D )
	{
		const TFLOAT fX = a_rChannel.afPosition[ 0 ] - s_afListener[ 0 ];
		const TFLOAT fY = a_rChannel.afPosition[ 1 ] - s_afListener[ 1 ];
		const TFLOAT fZ = a_rChannel.afPosition[ 2 ] - s_afListener[ 2 ];

		const TFLOAT fDistance = sqrtf( fX * fX + fY * fY + fZ * fZ );

		if ( fDistance > a_rChannel.fMinDistance )
			fGain *= powf(a_rChannel.fMinDistance / std::max(fDistance,0.001f),s_rolloffFactor);

		if ( fDistance > 0.001f )
			fPan = 0.5f + 0.5f * ( fX * s_afRight[ 0 ] + fY * s_afRight[ 1 ] + fZ * s_afRight[ 2 ] ) / fDistance;
	}

	const TFLOAT fLeftGain  = fGain * sqrtf( 1.0f - fPan );
	const TFLOAT fRightGain = fGain * sqrtf( fPan );
    double doppler=1;
    if(a_rChannel.b3D && s_dopplerFactor>0 && s_distanceFactor>0) {
        float x=a_rChannel.afPosition[0]-s_afListener[0],y=a_rChannel.afPosition[1]-s_afListener[1],z=a_rChannel.afPosition[2]-s_afListener[2];
        float distance=sqrtf(x*x+y*y+z*z);
        if(distance>0.001f) {
            float source=(x*a_rChannel.afVelocity[0]+y*a_rChannel.afVelocity[1]+z*a_rChannel.afVelocity[2])/distance*s_distanceFactor*s_dopplerFactor;
            float listener=(x*s_listenerVelocity[0]+y*s_listenerVelocity[1]+z*s_listenerVelocity[2])/distance*s_distanceFactor*s_dopplerFactor;
            doppler=std::clamp((343.0+listener)/std::max(34.3,343.0+source),0.5,2.0);
        }
    }
	const double dStep      = double( a_rChannel.iFrequency ) / MIX_RATE * doppler;
	const TINT   iChannels  = rInfo.iChannels;

	// Multichannel sounds are one mono voice per channel, each with its own volume and pan
	TFLOAT afVoiceLeft[ MAX_VOICES ];
	TFLOAT afVoiceRight[ MAX_VOICES ];

	for ( TINT i = 0; i < a_rChannel.iNumVoices; i++ )
	{
		const TFLOAT fVoiceGain = a_rChannel.aiVolumes[ i ] / 255.0f;
		const TFLOAT fVoicePan  = a_rChannel.aiPans[ i ] < 0 ? 0.5f : a_rChannel.aiPans[ i ] / 255.0f;

		afVoiceLeft[ i ]  = fVoiceGain * sqrtf( 1.0f - fVoicePan );
		afVoiceRight[ i ] = fVoiceGain * sqrtf( fVoicePan );
	}

	double dLoopStart, dLoopEnd;
	GetLoopRange( rInfo, dLoopStart, dLoopEnd );

	const double dLength = rInfo.uiLength;

	for ( TINT i = 0; i < a_iFrames; i++ )
	{
		const TINT iFrame = TINT( a_rChannel.dPosition );

		if ( iFrame >= TINT( dLength ) || iFrame < 0 )
			break;

		const TINT16* pFrame = a_rChannel.pSample ? a_rChannel.pSample->pDecoded + iFrame * iChannels : GetStreamFrame( a_rChannel.pStream, rInfo, iFrame );

		if ( pFrame )
		{
			TFLOAT afFrame[ MAX_VOICES ];
			const TINT iUsed = TMath::Min( iChannels, MAX_VOICES );

			for ( TINT k = 0; k < iUsed; k++ )
				afFrame[ k ] = pFrame[ k ] / 32768.0f;

			// Most sounds are 11 to 22 kHz, without this they come out harsh. A looping sound continues at its start
			TINT iNext = iFrame + 1;

			if ( a_rChannel.uiLoop && iNext >= TINT( dLoopEnd ) )
				iNext = TINT( dLoopStart );

			if ( iNext < TINT( dLength ) )
			{
				const TINT16* pNext  = a_rChannel.pSample ? a_rChannel.pSample->pDecoded + iNext * iChannels : GetStreamFrame( a_rChannel.pStream, rInfo, iNext );
				const TFLOAT  fBlend = TFLOAT( a_rChannel.dPosition - iFrame );

				if ( pNext )
				{
					for ( TINT k = 0; k < iUsed; k++ )
						afFrame[ k ] += ( pNext[ k ] / 32768.0f - afFrame[ k ] ) * fBlend;
				}
			}

			if ( a_rChannel.iNumVoices > 1 )
			{
				for ( TINT k = 0; k < a_rChannel.iNumVoices; k++ )
				{
					a_pOut[ i * 2 + 0 ] += afFrame[ k ] * afVoiceLeft[ k ];
					a_pOut[ i * 2 + 1 ] += afFrame[ k ] * afVoiceRight[ k ];
				}
			}
			else
			{
				a_pOut[ i * 2 + 0 ] += afFrame[ 0 ] * fLeftGain;
				a_pOut[ i * 2 + 1 ] += afFrame[ iChannels > 1 ? 1 : 0 ] * fRightGain;
			}
		}

		if ( a_rChannel.bBackwards )
		{
			a_rChannel.dPosition -= dStep;

			if ( a_rChannel.dPosition < dLoopStart )
			{
				a_rChannel.dPosition  = dLoopStart;
				a_rChannel.bBackwards = TFALSE;
			}
		}
		else
		{
			a_rChannel.dPosition += dStep;

			if ( a_rChannel.uiLoop && a_rChannel.dPosition >= dLoopEnd && dLoopEnd > dLoopStart )
			{
				if ( a_rChannel.uiLoop & FSOUND_LOOP_BIDI )
				{
					a_rChannel.dPosition  = dLoopEnd - 1.0;
					a_rChannel.bBackwards = TTRUE;
				}
				else
				{
					a_rChannel.dPosition = dLoopStart + fmod( a_rChannel.dPosition - dLoopStart, dLoopEnd - dLoopStart );
				}
			}
		}
	}
}

static TBOOL IsChannelPlaying( const Channel& a_rChannel )
{
	if ( !a_rChannel.pSample && !a_rChannel.pStream )
		return TFALSE;

	return a_rChannel.uiLoop != 0 || a_rChannel.dPosition < a_rChannel.pInfo->uiLength;
}

// SDL owns callback lifecycle; closing the device joins its callback thread.
static void AudioCallback(void*, Uint8* output, int bytes) {
    auto* mix=reinterpret_cast<float*>(output);
    const int frames=bytes/int(2*sizeof(float));
    memset(output,0,bytes);
    ScopedAudioLock lock;
    for(auto& channel:s_aChannels) {
        if(!channel.bPaused && IsChannelPlaying(channel)) MixChannel(channel,mix,frames);
    }
    for(int i=0;i<frames*2;++i) mix[i]=s_muted ? 0.0f : std::max(-1.0f,std::min(1.0f,mix[i]));
    ++s_snapshot.callbacks;s_snapshot.left=frames?mix[0]:0;s_snapshot.right=frames?mix[1]:0;
}
namespace BarnyardLinux {
AudioSnapshot audioSnapshot() {ScopedAudioLock lock;return s_snapshot;}
}

//-----------------------------------------------------------------------------
// Channels
//-----------------------------------------------------------------------------

static TINT MakeHandle( const Channel& a_rChannel )
{
	return TINT( &a_rChannel - s_aChannels ) | ( a_rChannel.iGeneration << 12 );
}

// Same as FMOD: a handle is the whole channel, a plain number from FSOUND_GetSubChannel is one voice of it
static Channel* GetChannel( TINT a_iHandle, TINT* a_piOutVoice = TNULL )
{
	if ( a_piOutVoice )
		*a_piOutVoice = -1;

	if ( a_iHandle < 0 )
		return TNULL;

	TINT       iSlot       = a_iHandle & 0xFFF;
	const TINT iGeneration = a_iHandle >> 12;

	if ( iGeneration == 0 )
	{
		TINT iVoice = 0;

		if ( iSlot >= MAX_CHANNELS )
		{
			iVoice = ( iSlot - MAX_CHANNELS ) % MAX_VOICES;
			iSlot  = ( iSlot - MAX_CHANNELS ) / MAX_VOICES;
		}

		if ( iSlot >= MAX_CHANNELS )
			return TNULL;

		if ( a_piOutVoice )
			*a_piOutVoice = iVoice;

		return &s_aChannels[ iSlot ];
	}

	if ( iSlot >= MAX_CHANNELS || s_aChannels[ iSlot ].iGeneration != iGeneration )
		return TNULL;

	return &s_aChannels[ iSlot ];
}

template <class Func>
static signed char ForChannels( TINT a_iHandle, Func a_fnFunc )
{
	ScopedAudioLock oLock;

	if ( a_iHandle == FSOUND_ALL )
	{
		for ( Channel& rChannel : s_aChannels )
		{
			if ( rChannel.pSample || rChannel.pStream )
				a_fnFunc( rChannel );
		}

		return 1;
	}

	Channel* pChannel = GetChannel( a_iHandle );

	if ( !pChannel )
		return 0;

	a_fnFunc( *pChannel );
	return 1;
}

static void SetVoiceValue( Channel& a_rChannel, TINT a_iVoice, TINT a_iValue, TINT( Channel::*a_pValues )[ MAX_VOICES ] )
{
	if ( a_iVoice >= 0 )
	{
		if ( a_iVoice < a_rChannel.iNumVoices )
			( a_rChannel.*a_pValues )[ a_iVoice ] = a_iValue;

		return;
	}

	for ( TINT i = 0; i < a_rChannel.iNumVoices; i++ )
		( a_rChannel.*a_pValues )[ i ] = a_iValue;
}

static signed char SetVoiceValues( TINT a_iHandle, TINT a_iValue, TINT( Channel::*a_pValues )[ MAX_VOICES ] )
{
	ScopedAudioLock oLock;

	if ( a_iHandle == FSOUND_ALL )
	{
		for ( Channel& rChannel : s_aChannels )
			SetVoiceValue( rChannel, -1, a_iValue, a_pValues );

		return 1;
	}

	TINT     iVoice;
	Channel* pChannel = GetChannel( a_iHandle, &iVoice );

	if ( !pChannel )
		return 0;

	SetVoiceValue( *pChannel, iVoice, a_iValue, a_pValues );
	return 1;
}

static void SetupVoices( Channel& a_rChannel, const SampleInfo& a_rInfo, TINT a_iVolume )
{
	a_rChannel.iNumVoices = IsMultichannel( a_rInfo ) ? TMath::Min( a_rInfo.iChannels, MAX_VOICES ) : 1;

	for ( TINT i = 0; i < a_rChannel.iNumVoices; i++ )
	{
		a_rChannel.aiVolumes[ i ] = a_iVolume;
		a_rChannel.aiPans[ i ]    = a_rChannel.iNumVoices > 1 ? ( ( i & 1 ) ? 255 : 0 ) : a_rInfo.iPan;
	}
}

static Channel* StartChannel( TINT a_iWanted )
{
	Channel* pChannel = TNULL;

	if ( a_iWanted < 0 )
	{
		for ( Channel& rChannel : s_aChannels )
		{
			if ( !rChannel.bReserved && !IsChannelPlaying( rChannel ) )
			{
				pChannel = &rChannel;
				break;
			}
		}
	}
	else
	{
		pChannel = &s_aChannels[ a_iWanted & 0xFFF ];
	}

	if ( !pChannel )
		return TNULL;

	const TBOOL bReserved = pChannel->bReserved;
	TUtil::MemClear( pChannel, sizeof( Channel ) );

	pChannel->bReserved    = bReserved;
	pChannel->iGeneration  = ( s_iNextGeneration++ & 0x7FFFF ) | 1;
	pChannel->fMinDistance = 1.0f;
	pChannel->fMaxDistance = 1000000000.0f;
	pChannel->dRequested   = AudioNow();

	return pChannel;
}

//-----------------------------------------------------------------------------
// FSOUND
//-----------------------------------------------------------------------------

extern "C"
{
	signed char F_API FSOUND_Init( int mixrate, int maxsoftwarechannels, unsigned int flags )
	{
        if(s_device) return 1;
        if(mixrate!=MIX_RATE || maxsoftwarechannels<1 || maxsoftwarechannels>MAX_CHANNELS || flags) return 0;
        if(SDL_InitSubSystem(SDL_INIT_AUDIO)) return 0;
        SDL_AudioSpec want{};want.freq=MIX_RATE;want.channels=2;want.format=AUDIO_F32SYS;
        want.samples=Uint16(std::clamp(MIX_RATE*s_bufferMs/1000,128,4096));want.callback=AudioCallback;
        s_device=SDL_OpenAudioDevice(nullptr,0,&want,nullptr,0);
        if(!s_device) {SDL_QuitSubSystem(SDL_INIT_AUDIO);return 0;}
        s_bMixing=TTRUE;SDL_PauseAudioDevice(s_device,0);return 1;
	}

	void F_API FSOUND_Close()
	{
        if(s_device) { SDL_CloseAudioDevice(s_device);s_device=0;SDL_QuitSubSystem(SDL_INIT_AUDIO); }
        s_bMixing=TFALSE;
		ScopedAudioLock oLock;

		for ( Channel& rChannel : s_aChannels )
		{
			rChannel.pSample = TNULL;
			rChannel.pStream = TNULL;
		}
	}

	void F_API FSOUND_Update() {}
	signed char F_API FSOUND_SetOutput( int outputtype ) { if(outputtype!=FSOUND_OUTPUT_NOSOUND && outputtype!=FSOUND_OUTPUT_OSS) return 0;ScopedAudioLock lock;s_muted=outputtype==FSOUND_OUTPUT_NOSOUND;return 1; }
	signed char F_API FSOUND_SetBufferSize( int len_ms ) { if(len_ms<1 || len_ms>1000)return 0;s_bufferMs=len_ms;return 1; }
	signed char F_API FSOUND_SetMinHardwareChannels( int min ) { return min==0; }
	int F_API FSOUND_GetMaxChannels() { return MAX_CHANNELS; }
	int F_API FSOUND_GetOutput() { return s_muted ? FSOUND_OUTPUT_NOSOUND : FSOUND_OUTPUT_OSS; }
	void* F_API FSOUND_GetOutputHandle() { return nullptr; }

	int F_API FSOUND_GetNumSubChannels( int channel )
	{
		ScopedAudioLock oLock;

		Channel* pChannel = GetChannel( channel );
		return pChannel ? pChannel->iNumVoices : 0;
	}

	int F_API FSOUND_GetSubChannel( int channel, int subchannel )
	{
		ScopedAudioLock oLock;

		Channel* pChannel = GetChannel( channel );

		if ( !pChannel || subchannel < 0 || subchannel >= pChannel->iNumVoices )
			return -1;

		const TINT iSlot = TINT( pChannel - s_aChannels );
		return subchannel == 0 ? iSlot : MAX_CHANNELS + iSlot * MAX_VOICES + subchannel;
	}

	signed char F_API FSOUND_GetNumHWChannels( int* num2d, int* num3d, int* total )
	{
		if ( num2d ) *num2d = 0;
		if ( num3d ) *num3d = 0;
		if ( total ) *total = 0;
		return 1;
	}

	void F_API FSOUND_GetMemoryStats( unsigned int* currentalloced, unsigned int* maxalloced )
	{
		if ( currentalloced ) *currentalloced = s_currentBytes.load();
		if ( maxalloced ) *maxalloced = s_peakBytes.load();
	}

	void F_API FSOUND_3D_SetDistanceFactor( float scale ) { ScopedAudioLock lock;if(std::isfinite(scale) && scale>=0) s_distanceFactor=scale; }
	void F_API FSOUND_3D_SetDopplerFactor( float scale ) { ScopedAudioLock lock;if(std::isfinite(scale) && scale>=0) s_dopplerFactor=scale; }
	void F_API FSOUND_3D_SetRolloffFactor( float scale ) { ScopedAudioLock lock;if(std::isfinite(scale) && scale>=0) s_rolloffFactor=scale; }

	void F_API FSOUND_3D_Listener_SetAttributes( const float* pos, const float* vel, float fx, float fy, float fz, float tx, float ty, float tz )
	{
		ScopedAudioLock oLock;

		if ( pos )
			TUtil::MemCopy( s_afListener, pos, sizeof( s_afListener ) );
        if(vel) memcpy(s_listenerVelocity,vel,sizeof(s_listenerVelocity));

		// Up x forward, FMOD's space is left handed by default
		s_afRight[ 0 ] = ty * fz - tz * fy;
		s_afRight[ 1 ] = tz * fx - tx * fz;
		s_afRight[ 2 ] = tx * fy - ty * fx;
	}

	signed char F_API FSOUND_3D_SetAttributes( int channel, const float* pos, const float* vel )
	{
		return ForChannels( channel, [ & ]( Channel& a_rChannel ) {
			if ( pos ) TUtil::MemCopy( a_rChannel.afPosition, pos, sizeof( a_rChannel.afPosition ) );
            if(vel) memcpy(a_rChannel.afVelocity,vel,sizeof(a_rChannel.afVelocity));
		} );
	}

	signed char F_API FSOUND_3D_SetMinMaxDistance( int channel, float min, float max )
	{
		return ForChannels( channel, [ & ]( Channel& a_rChannel ) {
			a_rChannel.fMinDistance = min;
			a_rChannel.fMaxDistance = max;
		} );
	}

	int F_API FSOUND_PlaySoundEx( int channel, FSOUND_SAMPLE* sptr, FSOUND_DSPUNIT* dsp, signed char startpaused )
	{
		if ( !sptr )
			return -1;

		if ( !sptr->pDecoded )
		{
			TINT16* pDecoded = DecodeSampleData( sptr );
            if(!pDecoded) return -1;

			ScopedAudioLock oLock;

			if ( sptr->pDecoded )
				AudioFree( pDecoded );
			else
				sptr->pDecoded = pDecoded;
		}

		ScopedAudioLock oLock;

		Channel* pChannel = StartChannel( channel );

		if ( !pChannel )
			return -1;

		const SampleInfo& rInfo = sptr->oInfo;

		pChannel->pSample      = sptr;
		pChannel->pInfo        = &rInfo;
		pChannel->uiLoop       = sptr->uiMode & ( FSOUND_LOOP_NORMAL | FSOUND_LOOP_BIDI );
		pChannel->iFrequency   = rInfo.iFrequency;
		pChannel->iPriority    = rInfo.iPriority;
		pChannel->b3D          = ( sptr->uiMode & FSOUND_HW3D ) != 0;
		pChannel->fMinDistance = rInfo.fMinDistance;
		pChannel->fMaxDistance = rInfo.fMaxDistance;
		pChannel->bPaused      = startpaused != 0;

		SetupVoices( *pChannel, rInfo, rInfo.iVolume );

		// The world starts many copies of an ambient loop at once. Sound cards start them a little apart, here they
		// would add up in phase and sound far too loud
		if ( pChannel->uiLoop && rInfo.uiLength > 0 )
		{
			const TUINT32 uiSeed = TUINT32( pChannel->iGeneration ) * 2654435761u;
			pChannel->dPosition  = ( uiSeed >> 8 ) % rInfo.uiLength;
		}

		return MakeHandle( *pChannel );
	}

	signed char F_API FSOUND_StopSound( int channel )
	{
		return ForChannels( channel, []( Channel& a_rChannel ) {
			a_rChannel.pSample = TNULL;
			a_rChannel.pStream = TNULL;
		} );
	}

	signed char F_API FSOUND_SetPaused( int channel, signed char paused )
	{
		return ForChannels( channel, [ & ]( Channel& a_rChannel ) {
			// The game starts sounds paused and sets them up first, the wait counts from here
			if ( a_rChannel.bPaused && !paused && a_rChannel.dRequested > 0.0 )
				a_rChannel.dRequested = AudioNow();

			a_rChannel.bPaused = paused != 0;
		} );
	}

	signed char F_API FSOUND_SetVolume( int channel, int vol )
	{
		return SetVoiceValues( channel, TMath::Max( TMath::Min( vol, 255 ), 0 ), &Channel::aiVolumes );
	}

	signed char F_API FSOUND_SetPan( int channel, int pan )
	{
		return SetVoiceValues( channel, pan == -1 ? -1 : TMath::Max( TMath::Min( pan, 255 ), 0 ), &Channel::aiPans );
	}

	signed char F_API FSOUND_SetFrequency( int channel, int freq )
	{
		return ForChannels( channel, [ & ]( Channel& a_rChannel ) { a_rChannel.iFrequency = freq; } );
	}

	signed char F_API FSOUND_SetPriority( int channel, int priority )
	{
		return ForChannels( channel, [ & ]( Channel& a_rChannel ) { a_rChannel.iPriority = priority; } );
	}

	signed char F_API FSOUND_SetReserved( int channel, signed char reserved )
	{
		ScopedAudioLock oLock;

		TINT     iVoice;
		Channel* pChannel = GetChannel( channel, &iVoice );

		if ( !pChannel )
			return 0;

		// The other voices don't take a channel of their own here
		if ( iVoice <= 0 )
			pChannel->bReserved = reserved != 0;

		return 1;
	}

	signed char F_API FSOUND_GetReserved( int channel )
	{
		ScopedAudioLock oLock;

		Channel* pChannel = GetChannel( channel );
		return pChannel && pChannel->bReserved ? 1 : 0;
	}

	signed char F_API FSOUND_IsPlaying( int channel )
	{
		ScopedAudioLock oLock;

		Channel* pChannel = GetChannel( channel );
		return pChannel && IsChannelPlaying( *pChannel ) ? 1 : 0;
	}

	FSOUND_SAMPLE* F_API FSOUND_GetCurrentSample( int channel )
	{
		ScopedAudioLock oLock;

		Channel* pChannel = GetChannel( channel );

		if ( !pChannel )
			return TNULL;

		if ( pChannel->pStream )
		{
			FSOUND_SAMPLE& rSample = pChannel->pStream->oCurrentSample;

			rSample.oInfo  = *pChannel->pInfo;
			rSample.uiMode = pChannel->pInfo->uiMode;

			return &rSample;
		}

		return pChannel->pSample;
	}

	signed char F_API FSOUND_Sample_SetMode( FSOUND_SAMPLE* sptr, unsigned int mode )
	{
		if ( !sptr )
			return 0;

        ScopedAudioLock lock;
		sptr->uiMode = ( sptr->uiMode & ~7u ) | ( mode & 7u );
		return 1;
	}

	unsigned int F_API FSOUND_Sample_GetMode( FSOUND_SAMPLE* sptr )
	{
		return sptr ? sptr->uiMode : 0;
	}

	const char* F_API FSOUND_Sample_GetName( FSOUND_SAMPLE* sptr )
	{
		return sptr ? sptr->oInfo.szName : TNULL;
	}

	signed char F_API FSOUND_Sample_GetDefaults( FSOUND_SAMPLE* sptr, int* deffreq, int* defvol, int* defpan, int* defpri )
	{
		if ( !sptr )
			return 0;

		if ( deffreq ) *deffreq = sptr->oInfo.iFrequency;
		if ( defvol ) *defvol = sptr->oInfo.iVolume;
		if ( defpan ) *defpan = sptr->oInfo.iPan;
		if ( defpri ) *defpri = sptr->oInfo.iPriority;
		return 1;
	}

	FMUSIC_MODULE* F_API FMUSIC_LoadSongEx( const char* name_or_data, int offset, int length, unsigned int mode, const int* samplelist, int samplelistnum )
	{
        if(!name_or_data || offset<0 || length<0 || length>256*1024*1024) return TNULL;
		TUINT8* pBytes = TNULL;
		TUINT32 uiSize = 0;

		if ( mode & FSOUND_LOADMEMORY )
		{
			uiSize = length;
			pBytes = TSTATICCAST( TUINT8, AudioAlloc( uiSize ) );
			TUtil::MemCopy( pBytes, name_or_data, uiSize );
		}
		else
		{
			TFile* pFile = OpenAudioFile( name_or_data );

			if ( !pFile )
				return TNULL;

            if(TUINT32(offset)>pFile->GetSize() || (length && TUINT32(length)>pFile->GetSize()-offset)) {pFile->Destroy();return TNULL;}
			uiSize = length ? length : pFile->GetSize() - offset;
            if(uiSize>256u*1024*1024) {pFile->Destroy();return TNULL;}
			pBytes = TSTATICCAST( TUINT8, AudioAlloc( uiSize ) );

            bool readOk=pFile->Seek(offset,TSEEK_SET) && pFile->Read(pBytes,uiSize)==uiSize;
            pFile->Destroy();if(!readOk) {AudioFree(pBytes);return TNULL;}
		}

        if(uiSize<24 || uint64_t(Read32(pBytes+8))+24+Read32(pBytes+12)>uiSize) { AudioFree(pBytes);return TNULL; }
		SampleInfo* pInfos  = TNULL;
		const TINT  iNumber = ParseFSB( pBytes, uiSize, &pInfos );

		if ( iNumber == 0 )
		{
			AudioFree( pBytes );
			return TNULL;
		}

		FMUSIC_MODULE* pSong = new FMUSIC_MODULE;
		pSong->pBytes        = pBytes;
		pSong->iNumSamples   = iNumber;
		pSong->pSamples      = new FSOUND_SAMPLE[ iNumber ];

		for ( TINT i = 0; i < iNumber; i++ )
		{
			FSOUND_SAMPLE& rSample = pSong->pSamples[ i ];

			rSample.oInfo    = pInfos[ i ];
			rSample.uiMode   = pInfos[ i ].uiMode;
			rSample.pOwner   = pSong;
			rSample.pData    = pBytes + pInfos[ i ].uiOffset;
			rSample.pDecoded = TNULL;
		}

		AudioFree( pInfos );
		return pSong;
	}

	signed char F_API FMUSIC_FreeSong( FMUSIC_MODULE* mod )
	{
		if ( !mod )
			return 0;

		ScopedAudioLock oLock;

		for ( Channel& rChannel : s_aChannels )
		{
			if ( rChannel.pSample && rChannel.pSample->pOwner == mod )
				rChannel.pSample = TNULL;
		}

		for ( TINT i = 0; i < mod->iNumSamples; i++ )
		{
			if ( mod->pSamples[ i ].pDecoded )
				AudioFree( mod->pSamples[ i ].pDecoded );
		}

		delete[] mod->pSamples;
		AudioFree( mod->pBytes );
		delete mod;

		return 1;
	}

	int F_API FMUSIC_GetOpenState( FMUSIC_MODULE* mod ) { return mod ? 0 : -1; }
	int F_API FMUSIC_GetNumSamples( FMUSIC_MODULE* mod ) { return mod ? mod->iNumSamples : 0; }

	FSOUND_SAMPLE* F_API FMUSIC_GetSample( FMUSIC_MODULE* mod, int sampno )
	{
		return mod && sampno >= 0 && sampno < mod->iNumSamples ? &mod->pSamples[ sampno ] : TNULL;
	}

	FSOUND_STREAM* F_API FSOUND_Stream_Open( const char* name_or_data, unsigned int mode, int offset, int length )
	{
        if(!name_or_data || offset<0 || length<0 || ((mode&FSOUND_LOADMEMORY) && length>256*1024*1024)) return TNULL;
		FSOUND_STREAM* pStream = new FSOUND_STREAM;
		TUtil::MemClear( pStream, sizeof( FSOUND_STREAM ) );

		pStream->uiMode       = mode;
		pStream->uiBase       = offset;
		pStream->iCachedBlock = -1;
		pStream->iChannel     = -1;

		TUINT8* pHeader = TNULL;
		TUINT32 uiSize  = 0;

		if ( mode & FSOUND_LOADMEMORY )
		{
			uiSize           = length;
			pStream->pMemory = TSTATICCAST( TUINT8, AudioAlloc( uiSize ) );
			TUtil::MemCopy( pStream->pMemory, name_or_data, uiSize );
			pHeader = pStream->pMemory;
		}
		else
		{
            if(strlen(name_or_data)>=sizeof(pStream->szPath)) {delete pStream;return TNULL;}
			T2String8::Copy( pStream->szPath, name_or_data, sizeof( pStream->szPath ) - 1 );

			TFile* pFile = OpenAudioFile( name_or_data );

			if ( !pFile )
			{
				delete pStream;
				return TNULL;
			}

            const uint64_t fileSize=pFile->GetSize();
            const uint64_t bankSize=length ? uint64_t(length) : fileSize-offset;
            if(uint64_t(offset)>fileSize || bankSize>fileSize-offset || bankSize<24) {
                fprintf(stderr,"Native FSB3 stream rejected: invalid file offset/length\n");
                pFile->Destroy();delete pStream;return TNULL;
            }
            unsigned char fixed[24];
            bool readOk=pFile->Seek(offset,TSEEK_SET) && pFile->Read(fixed,24)==24;
            const uint64_t headerSize=readOk ? 24ull+Read32(fixed+8) : 0;
            const uint64_t declaredSize=readOk ? headerSize+Read32(fixed+12) : 0;
            if(!readOk || memcmp(fixed,"FSB3",4) || headerSize>16u*1024*1024 || declaredSize>bankSize) {
                fprintf(stderr,"Native FSB3 stream rejected: magic/header/payload; header=%llu declared=%llu available=%llu\n",
                    (unsigned long long)headerSize,(unsigned long long)declaredSize,(unsigned long long)bankSize);
                pFile->Destroy();delete pStream;return TNULL;
            }
            // Read the exact complete header, not an arbitrary first 1 MiB of
            // payload. Audio remains streamed from disk with one block cache.
            uiSize=TUINT32(headerSize);
            pHeader=TSTATICCAST(TUINT8,AudioAlloc(uiSize));
            readOk=pFile->Seek(offset,TSEEK_SET) && pFile->Read(pHeader,uiSize)==uiSize;
            pFile->Destroy();if(!readOk) {AudioFree(pHeader);delete pStream;return TNULL;}

		}

        if(pStream->pMemory && (uiSize<24 || 24ull+Read32(pHeader+8)+Read32(pHeader+12)>uiSize)) {AudioFree(pStream->pMemory);delete pStream;return TNULL;}
		pStream->iNumSubs = ParseFSB( pHeader, uiSize, &pStream->pSubs );

		if ( !pStream->pMemory )
			AudioFree( pHeader );

		if ( pStream->iNumSubs == 0 )
		{
            fprintf(stderr,"Native FSB3 stream could not open bank: %s\n", (mode&FSOUND_LOADMEMORY)?"<memory>":name_or_data);
            AudioFree(pStream->pMemory);
			delete pStream;
			return TNULL;
		}

        for(int i=0;i<pStream->iNumSubs;++i) if(pStream->pSubs[i].uiMode&FSOUND_8BITS) {AudioFree(pStream->pMemory);AudioFree(pStream->pSubs);delete pStream;return TNULL;}
		return pStream;
	}

	static void StopStreamChannel( FSOUND_STREAM* a_pStream )
	{
		if ( a_pStream->iChannel >= 0 )
		{
			Channel& rChannel = s_aChannels[ a_pStream->iChannel ];

			if ( rChannel.pStream == a_pStream )
				rChannel.pStream = TNULL;

			a_pStream->iChannel = -1;
		}
	}

	static Channel* GetStreamChannel( FSOUND_STREAM* a_pStream )
	{
		if ( a_pStream->iChannel >= 0 && s_aChannels[ a_pStream->iChannel ].pStream == a_pStream )
			return &s_aChannels[ a_pStream->iChannel ];

		return TNULL;
	}

	int F_API FSOUND_Stream_PlayEx( int channel, FSOUND_STREAM* stream, FSOUND_DSPUNIT* dsp, signed char startpaused )
	{
		if ( !stream )
			return -1;

		ScopedAudioLock oLock;

		StopStreamChannel( stream );

		Channel* pChannel = StartChannel( channel );

		if ( !pChannel )
			return -1;

		const SampleInfo& rInfo = stream->pSubs[ stream->iSub ];

		pChannel->pStream      = stream;
		pChannel->pInfo        = &rInfo;
		pChannel->uiLoop       = stream->uiMode & ( FSOUND_LOOP_NORMAL | FSOUND_LOOP_BIDI );
		pChannel->iFrequency   = rInfo.iFrequency;
		pChannel->b3D          = ( stream->uiMode & FSOUND_HW3D ) != 0;
		pChannel->fMinDistance = rInfo.fMinDistance;
		pChannel->fMaxDistance = rInfo.fMaxDistance;
		pChannel->bPaused      = startpaused != 0;
		pChannel->dPosition    = stream->uiTimeMs * double( rInfo.iFrequency ) / 1000.0;

		SetupVoices( *pChannel, rInfo, 255 );

		stream->iChannel = TINT( pChannel - s_aChannels );
		return MakeHandle( *pChannel );
	}

	signed char F_API FSOUND_Stream_Stop( FSOUND_STREAM* stream )
	{
		if ( !stream )
			return 0;

		ScopedAudioLock oLock;

		StopStreamChannel( stream );
		stream->uiTimeMs = 0;

		return 1;
	}

	signed char F_API FSOUND_Stream_Close( FSOUND_STREAM* stream )
	{
		if ( !stream )
			return 0;

		ScopedAudioLock oLock;

		StopStreamChannel( stream );

		if ( stream->pFile )
			stream->pFile->Destroy();

		if ( stream->pMemory )
			AudioFree( stream->pMemory );

		AudioFree( stream->pSubs );
		delete stream;

		return 1;
	}

	signed char F_API FSOUND_Stream_SetMode( FSOUND_STREAM* stream, unsigned int mode )
	{
		if ( !stream )
			return 0;

		ScopedAudioLock oLock;

		stream->uiMode = mode;

		// The game sets it after playing the stream paused
		if ( Channel* pChannel = GetStreamChannel( stream ) )
		{
			pChannel->uiLoop = mode & ( FSOUND_LOOP_NORMAL | FSOUND_LOOP_BIDI );
			pChannel->b3D    = ( mode & FSOUND_HW3D ) != 0;
		}

		return 1;
	}

	unsigned int F_API FSOUND_Stream_GetMode( FSOUND_STREAM* stream ) { return stream ? stream->uiMode : 0; }
	signed char F_API FSOUND_Stream_SetBufferSize( int ms ) { if(ms<1 || ms>60000)return 0;s_streamBufferMs=ms;return 1; }
	int F_API FSOUND_Stream_GetOpenState( FSOUND_STREAM* stream ) { return stream ? 0 : -3; }
	int F_API FSOUND_Stream_GetNumSubStreams( FSOUND_STREAM* stream ) { return stream ? stream->iNumSubs : 0; }

	int F_API FSOUND_Stream_GetLengthMs( FSOUND_STREAM* stream )
	{
		if ( !stream )
			return 0;

		const SampleInfo& rInfo = stream->pSubs[ stream->iSub ];
		return TINT( 1000.0 * rInfo.uiLength / rInfo.iFrequency );
	}

	int F_API FSOUND_Stream_GetTime( FSOUND_STREAM* stream )
	{
		if ( !stream )
			return 0;

		ScopedAudioLock oLock;

		if ( Channel* pChannel = GetStreamChannel( stream ) )
			return TINT( TMath::Min( pChannel->dPosition, double( pChannel->pInfo->uiLength ) ) * 1000.0 / pChannel->pInfo->iFrequency );

		return stream->uiTimeMs;
	}

	signed char F_API FSOUND_Stream_SetTime( FSOUND_STREAM* stream, int ms )
	{
		if ( !stream )
			return 0;

		ScopedAudioLock oLock;

		stream->uiTimeMs = ms;

		if ( Channel* pChannel = GetStreamChannel( stream ) )
			pChannel->dPosition = ms * double( pChannel->pInfo->iFrequency ) / 1000.0;

		return 1;
	}

	signed char F_API FSOUND_Stream_SetSubStream( FSOUND_STREAM* stream, int index )
	{
		if ( !stream || index < 0 || index >= stream->iNumSubs )
			return 0;

		ScopedAudioLock oLock;

		stream->iSub         = index;
		stream->uiTimeMs     = 0;
		stream->iCachedBlock = -1;

		return 1;
	}
}
