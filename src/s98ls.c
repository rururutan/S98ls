#include	<windows.h>
#include	"stdio.h"
#include	"math.h"
#include	"s98ls.h"

bool (*checkS98NoteReg) (BYTE*);
bool (*checkS98Channel) (BYTE *, int);

/**
 * for PSG / OPN / OPNA
 */
bool checkS98OpnaNoteReg( BYTE *in )
{
	BYTE checkreg[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
		0xa0, 0xa1, 0xa2, 0xa4, 0xa5, 0xa6,
		0xa8, 0xa9, 0xaa, 0xac, 0xad, 0xae,
		0x06, 0xff
	};
	int r = 0;

	while( checkreg[r] != 0xff ) {
		if( in[1] == checkreg[r] ) return true;
		r++;
	}
	return false;
}

/**
 * for OPM
 */
bool checkS98OpmNoteReg( BYTE *in )
{
	BYTE checkreg[] = {
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0xff
	};
	int r = 0;

	while( checkreg[r] != 0xff ) {
		if( in[1] == checkreg[r] ) return true;
		r++;
	}
	return false;
}

/**
 * for PSG / OPN / OPNA
 */
bool checkS98OpnaChannel( BYTE *in, int ch )
{
	BYTE	reg[9][3] = {
		{ 0x00, 0xa0, 0xa4 },
		{ 0x00, 0xa1, 0xa5 },
		{ 0x00, 0xa2, 0xa6 },
		{ 0x01, 0xa0, 0xa4 },
		{ 0x01, 0xa1, 0xa5 },
		{ 0x01, 0xa2, 0xa6 },
		{ 0x00, 0x00, 0x01 },
		{ 0x00, 0x02, 0x03 },
		{ 0x00, 0x04, 0x05 },
	};

	if( ch == 0xff ) return true;
	if( (in[0] == reg[ch][0]) && (in[1] == reg[ch][1]) ) return true;
	if( (in[0] == reg[ch][0]) && (in[1] == reg[ch][2]) ) return true;

	return false;
}

/**
 * for OPM
 */
bool checkS98OpmChannel( BYTE *in, int ch )
{
	BYTE	reg[8][3] = {
		{ 0x00, 0x28, 0x30 },
		{ 0x00, 0x29, 0x31 },
		{ 0x00, 0x2a, 0x32 },
		{ 0x00, 0x2b, 0x33 },
		{ 0x00, 0x2c, 0x34 },
		{ 0x00, 0x2d, 0x35 },
		{ 0x00, 0x2e, 0x36 },
		{ 0x00, 0x2f, 0x37 },
	};

	if( ch == 0xff ) return true;
	if( (in[0] == reg[ch][0]) && (in[1] == reg[ch][1]) ) return true;
	if( (in[0] == reg[ch][0]) && (in[1] == reg[ch][2]) ) return true;

	return false;
}

/*------------------------------------------------------------------------------
 �_���v�f�[�^���̔�Wait�f�[�^�����擾
 @param data [in] S98 dump data
 @param ch   [in] search channel
 @return ���f�[�^��
------------------------------------------------------------------------------*/
int getS98DataNum( BYTE *data, int ch )
{
	int	n = 0;

	while( 1 ) {
		switch( *data ) {
		case 0x00:
		case 0x01:
			if( checkS98NoteReg( data ) ) {
				if( checkS98Channel( data, ch ) ) {
					n++;
				}
			}
			data += 3;
			break;
		case 0xFF:
			data++;
			break;
		case 0xFE:
			do {
				++data;
			} while( *data&0x80 );
			++data;
			break;
		case 0xFD:
			return n;
		default:
			fprintf( stderr, "��̃R�}���h%02x\n", *data );
		}
	}
}

/*------------------------------------------------------------------------------
 S98DATA�\���̂̐���
 @param in    [in] S98 data
 @param ofs   [in] S98 dump data offset
 @param data  [out] S98DATA structure
 @param sync1 [in] timer info1
 @param sync2 [in] timer info2
 @param ch    [in] search channel
------------------------------------------------------------------------------*/
void setS98Data( BYTE *in, DWORD ofs, S98DATA *data, DWORD Sync1, DWORD Sync2, int ch )
{
	DWORD	time = 0;
	DWORD	temp;
	int		s;

	while( 1 ) {
		switch( in[ofs] ) {
		case 0x00:
		case 0x01:
			if( checkS98NoteReg( &in[ofs] ) ) {
				if( checkS98Channel( &in[ofs], ch ) ) {
					data->Offset = ofs;
					data->Time = (DWORD)((__int64)time*Sync1*1000/Sync2);
					data->OPNAData[0] = in[ofs+0];
					data->OPNAData[1] = in[ofs+1];
					data->OPNAData[2] = in[ofs+2];
					data++;
				}
			}
			ofs += 3;
			break;
		case 0xff:
			time++;
			ofs++;
			break;
		case 0xfe:
			s = 0;
			temp = 0;
			do {
				temp |= (in[++ofs]&0x7f)<<s;
				s += 7;
			} while( in[ofs]&0x80 );
			time += temp+2;
			++ofs;
			break;
		case 0xfd:
			return;
		}
	}
}

/*------------------------------------------------------------------------------
 �w�莞�Ԃ̃R�}���h�I�t�Z�b�g�擾
 @param data [in] command data
 @param time [in] search time (msec)
 @return command offset
------------------------------------------------------------------------------*/
DWORD getS98Offset( S98DATA *data, DWORD time, int num )
{
	int	i;

	for( i = 0; i < num; i++ ) {
		if( data->Time >= time ) break;
		data++;
	}
	return i;
}

/*------------------------------------------------------------------------------
 ���[�v����t������S98�̏o��
 @param outfile [in] output filename
 @param in      [in] S98 data
 @param size    [in] S98 data size
 @param data    [in] S98DATA structure
 @param top     [in] search start command num
 @param end     [in] search end command num
 @return output num
------------------------------------------------------------------------------*/
int outS98Loop( char *outfile, char *in, int size, S98DATA *data, int top, int end )
{
	int		cmp;
	S98HEAD	*s98;
	FILE	*fp;
	int		i, ofs = 0, n = 0;
	char	temp[1024];

	cmp = end-top;
	while( ofs < top-cmp ) {
		if( (data[ofs].OPNAData[2] == data[top].OPNAData[2])
		 && (data[ofs].OPNAData[1] == data[top].OPNAData[1])
		 && (data[ofs].OPNAData[0] == data[top].OPNAData[0]) ) {
			for( i = 0; i < cmp; i++ ) {
				if( (data[ofs+i].OPNAData[2] != data[top+i].OPNAData[2])
				 || (data[ofs+i].OPNAData[1] != data[top+i].OPNAData[1])
				 || (data[ofs+i].OPNAData[0] != data[top+i].OPNAData[0]) ) break;
			}
			// ��v
			if( i == cmp ) {
				s98 = (S98HEAD *)in;
				s98->LoopPointOffset = data[ofs].Offset;
				sprintf( temp, "%s.%04d.s98", outfile, n );
				fp = fopen( temp, "wb" );
				if( fp != NULL ) {
					int taglen, tagofs;
					if ( s98->FormatVer == '3' && s98->SongNameOffset ) {
						tagofs = s98->SongNameOffset;
						taglen = size - tagofs;
						s98->SongNameOffset = data[top].Offset + 1;
					}
					fwrite( in, data[top].Offset, 1, fp );
					fputc( 0xfd, fp );
					if ( s98->FormatVer == '3' && s98->SongNameOffset ) {
						fwrite( in+tagofs, taglen, 1, fp );
					}
					fclose( fp );
#ifdef MESSAGETYPE_JAPANESE
					fprintf( stderr, "���[�v���ʒu�𔭌����܂����B%s�t�@�C���Ƃ��ďo�͂��܂�\n", temp );
#else
					fprintf( stderr, "Found loop point. output file : %s\n", temp );
#endif
				}
				n++;
			}
		}
		ofs++;
	}
	return n;
}

/*------------------------------------------------------------------------------
 ���O�t�@�C������
 @param log    [in] create flag (0 or 1)
 @param infile [in] original file name
 @param data   [in] S98DATA structure
 @param num    [in] total command number
------------------------------------------------------------------------------*/
void OutputLog( int log, char *infile, S98DATA *data, int num )
{
	char	temp[1024];
	FILE	*fp;

	if( log ) {
		sprintf( temp, "%s.log", infile );
		fp = fopen( temp, "wb" );
		if( fp != NULL ) {
			fprintf( fp, "Source file : %s\n", infile );
			fprintf( fp, "time(ms): offset :data\n" );
			do {
				fprintf( fp, "%8d:%8x:%02x,%02x,%02x\n",
					data->Time,
					data->Offset,
					data->OPNAData[0]&0xff,
					data->OPNAData[1]&0xff,
					data->OPNAData[2]&0xff );
				data++;
			} while( --num );
			fclose( fp );
		}
	}
}

/*------------------------------------------------------------------------------
 ���[�v�������C��
 @param infile  [in] input file name
 @param outfile [in] output file nmame
 @param start   [in] search time [msec]
 @param len     [in] search length [msec]
 @param ch      [in] search channel no
 @param log     [in] enable [0 or 1]
------------------------------------------------------------------------------*/
int s98Loop( char *infile, char *outfile, DWORD start, DWORD len, int ch, int log )
{
	FILE	*fp;
	int		n = 0, num, size;
	char	*in;
	S98HEAD	*s98;
	S98DATA	*data;
	int		top, end;
	DWORD	Sync1, Sync2;

	if( (start == 0)  || (len == 0) ) return 0;

	fp = fopen( infile, "rb" );
	if( fp == NULL ) {
#ifdef MESSAGETYPE_JAPANESE
		fprintf( stderr, "�t�@�C�����J���܂���ł���:%s\n", infile );
#else
		fprintf( stderr, "Don't Open File:%s\n", infile );
#endif
		return 0;
	}
	// �t�@�C���T�C�Y���擾
	fseek( fp, 0, SEEK_END );
	size = ftell( fp );
	fseek( fp, 0, SEEK_SET );

	// �t�@�C���ǂݍ��݃o�b�t�@�쐬
	in = (char *)malloc( size );
	if( in == NULL ) {
		fclose( fp );
#ifdef MESSAGETYPE_JAPANESE
		fprintf( stderr, "���������m�ۂł��܂���ł���\n", infile );
#else
		fprintf( stderr, "Don't allocate memory\n", infile );
#endif
		return 0;
	}
	fread( in, size, 1, fp );
	fclose( fp );

	s98 = (S98HEAD *)in;
	if( strncmp( s98->MAGIC, "S98", 3 ) != 0 ) {
#ifdef MESSAGETYPE_JAPANESE
		fprintf( stderr, "s98�t�@�C���ł͂���܂���\n" );
#else
		fprintf( stderr, "Don't s98 File\n" );
#endif
		free( in );
		return 0;
	}

	checkS98NoteReg = checkS98OpnaNoteReg;
	checkS98Channel = checkS98OpnaChannel;
	switch( s98->FormatVer ) {
	case '1':
		break;
	case '3':
		if ( s98->DeviceCount == 0 ) {
			break;
		}
		if ( s98->DeviceCount == 1 ) {
			S98DEV *dev = (S98DEV*)&in[0x20];
			if ( dev->DevType == 0x01 || dev->DevType == 0x02 ||
				 dev->DevType == 0x04 || dev->DevType == 0x0f) {
				break;
		    }
			if ( dev->DevType == 0x05 ) {
				checkS98NoteReg = checkS98OpmNoteReg;
				checkS98Channel = checkS98OpmChannel;
				break;
			}
		}
#ifdef MESSAGETYPE_JAPANESE
		fprintf( stderr, "OPNA/OPN/PSG/OPM��S98�t�@�C�����w�肵�Ă�������\n" );
#else
		fprintf( stderr, "Only OPNA/OPN/PSG/OPM file\n" );
#endif
		free( in );
		return 0;
	default:
#ifdef MESSAGETYPE_JAPANESE
		fprintf( stderr, "���Ή��̃o�[�W�����ł�\n" );
#else
		fprintf( stderr, "Undefined Version\n" );
#endif
		free( in );
		return 0;
	}
	Sync1 = s98->Sync1;
	if( Sync1 == 0 ) Sync1 = 10;
	Sync2 = s98->Sync2;
	if( Sync2 == 0 ) Sync2 = 1000;

	num = getS98DataNum( &in[s98->DumpDataOffset], ch );
	data = (S98DATA *)malloc( num*sizeof(S98DATA) );
	if( data == NULL ) {
#ifdef MESSAGETYPE_JAPANESE
		fprintf( stderr, "���������m�ۂł��܂���ł���\n", infile );
#else
		fprintf( stderr, "Don't allocate memory\n", infile );
#endif
		free( in );
		return 0;
	}
	memset( (char *)data, 0, n*sizeof(S98DATA) );
	setS98Data( in, s98->DumpDataOffset, data, Sync1, Sync2, ch );
	top = getS98Offset( data, start, num );
	end = getS98Offset( data, start+len, num );
	if( end == num ) end = num-1;
#ifdef	_DEBUG
	printf( "%d-%d\n", top, end );
#endif
	if( top >= end ) {
#ifdef MESSAGETYPE_JAPANESE
		fprintf( stderr, "���[�v�|�C���g�������Ԑݒ肪����������܂���" );
#else
		fprintf( stderr, "Wrong search time by loop point\n" );
#endif
		free( data );
		free( in );
		return 0;
	}
	n = outS98Loop( outfile, in, size, data, top, end );
	OutputLog( log, infile, data, num );
	if( n == 0 ) {
#ifdef MESSAGETYPE_JAPANESE
		fprintf( stderr, "���[�v�|�C���g��₪������܂���ł���" );
#else
		fprintf( stderr, "no found to loop point\n" );
#endif
	} else {
#ifdef MESSAGETYPE_JAPANESE
		fprintf( stderr, "%d���̃��[�v�|�C���g��₪������܂���", n );
#else
		fprintf( stderr, "%d found to loop point \n" );
#endif
	}
	free( data );
	free( in );
	return n;
}

/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/
void dispHelpMessage( void )
{
	fprintf( stderr,
		"S98 Loop Searcher Version 0.0.6 by Manbow-J / RuRuRu / UME-3\n"
#ifdef MESSAGETYPE_JAPANESE
		"�g����: s98ls <���̓t�@�C��> [<�o�̓t�@�C��>] [Option]\n"
		"Option:\n"
		"        -sxx ... xx�b�̈ʒu���烋�[�v�̌������J�n���܂��B�K���w�肵�Ă�������\n"
		"        -lxx ... �J�n�ʒu����xx�b�̊Ԃ����[�v�����f�[�^�Ƃ��܂�(default:1�b)�B\n"
		"        -cxx ... ����̃`�����l���f�[�^�݂̂Ń��[�v�ʒu���������܂��B\n"
		"        -o   ... ���[�v�������̃��O���o�͂��܂��B\n"
		"                 �o�̓t�@�C����\"<���̓t�@�C��>.log\"�ł��B\n"
		"  * �o�̓t�@�C���͏ȗ��ł��܂��B�ȗ������ꍇ�A\"<���̓t�@�C��>.xxxx.s98\"\n"
		"    �Ƃ����t�@�C�����ŏo�͂���܂�\n"
#else
		"Usage: s98ls <input file(.s98)> [<outputfile>] [Option]\n"
		"Option:\n"
		"        -sxx ... Searching loop start position from xx sec. needed!\n"
		"        -lxx ... xx sec in loop search data from the starting position(default:1sec\n"
		"        -cxx ... Searching loop point for Specific channel data.\n"
		"        -o   ... Output log by loop seaching.\n"
		"                 Output filename is <input file>.log.\n"
		"  * Output file is omissible. If you omissivle output file, output file name\n"
		"    is \"<input file>+xxxx.s98\"\n"
#endif
	);
}

/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/
int main( int argc, char *argv[] )
{
	char	*nArg[2];
	int		i = 1, n = 0;
	DWORD	LoopPoint = 0;
	DWORD	start = 0;
	DWORD	len = 1;
	DWORD	log = 0;
	int		ch = 0xff;

	if( argc <= 1 ) {
		dispHelpMessage();
		return 1;
	}

	nArg[0] = nArg[1] = NULL;
	while ( argv[i] != NULL ) {
		if( argv[i][0] == '-' ) {
			switch( argv[i][1] ) {
			case 's':
			case 'S':
				sscanf( &argv[i][2], "%d", &start );
				break;
			case 'l':
			case 'L':
				sscanf( &argv[i][2], "%d", &len );
				break;
			case 'c':
			case 'C':
				sscanf( &argv[i][2], "%d", &ch );
				if( ch > 9 ) {
#ifdef MESSAGETYPE_JAPANESE
					fprintf( stderr, "�`�����l���̎w�肪�Ԉ���Ă��܂�: %s\n", argv[i] );
#else
					fprintf( stderr, "Abnormal channel no: %s\n", argv[i] );
#endif
				}
				break;
			case 'o':
			case 'O':
				log = 1;
				break;
			default:
#ifdef MESSAGETYPE_JAPANESE
				fprintf( stderr, "����`�ȃI�v�V����:%s\n", argv[i] );
#else
				fprintf( stderr, "Undefined Option:%s\n", argv[i] );
#endif
				dispHelpMessage();
				return 1;
			}
		} else {
			nArg[n] = argv[i];
			n++;
		}
		i++;
	}
	if( start == 0 ) {
#ifdef MESSAGETYPE_JAPANESE
				fprintf( stderr, "�����ʒu�͕K���w�肵�Ă�������\n" );
#else
				fprintf( stderr, "Need Search point\n" );
#endif
		return 1;
	}	
	if( nArg[1] == NULL ) {
		nArg[1] = nArg[0];
	}
	s98Loop( nArg[0], nArg[1], start*1000, len*1000, ch, log );

	return 0;
}
