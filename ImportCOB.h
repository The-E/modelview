//
// stuff to read *.cob files...
// - Garry Knudson's code
//   8-15-99
//

#include "MODVIEW32Doc.h"


class CImportCOB
{
protected:

public:
	int m_Game;
	CString errstring;
	CString errdetails;
	CString logstring;

	CImportCOB();
	~CImportCOB();
	ERRORCODE ReadCOB(CFile *f);
	void ConvertCob2Model();
	void MakePof(/*char *output*/);

private:
	CMODVIEW32Doc *GetDocument();
	void StatusPrnt2(char *fmt, ...);
	void ReduceToUnit(float vector[3]);
	void calcNormal(float v[3][3], float out[3]);
	void ohdr();
	void txtr();
	void Dump0Data();
	void Dump1Data(unsigned int Segment);
	void Dump23Data(int Polygon);
	void Dump4Data(FS_VPNT Normal, FS_VPNT Point, FS_VPNT Min, FS_VPNT Max);
	void Dump5Data(FS_VPNT Min, FS_VPNT Max);
	void split(FS_VPNT Min, FS_VPNT Max,unsigned int SobjNum);
	void sobj(int SobjNum);
	void shld();
	void pinf();


#define D_GROU          0x756f7247
#define D_POLH          0x486c6f50
#define D_MAT1          0x3174614d
#define D_MREC    		0x6365524d
#define D_END           0x20444e45

#define SOBJ_MAX		100
#define VEC_MAX		10000
#define POLY_MAX		10000
#define TEXTURE_MAX	100
#define MATL_MAX		250  
#define SHIELD_MAX	500  
#define DELTA		0.001f

#define COB2FS_VERSION "COB2FS Version 0.05 09/04/1999"


//int run(char *input, char *output);

typedef struct vector {
	float x,y,z;
} VECTOR;

typedef struct quad {
	float j,k,l,m;
} QUAD;

//#pragma pack( push, 1 )
#pragma pack(1)

typedef struct cob_header {
	char Name[9];
	char Version[6];
	char Format;
	char Endian[2];
	char Blank[13];
	char Newline;
} COB_HEADER;

typedef struct chunk_header {
	unsigned int Type;
	unsigned short Major;
	unsigned short Minor;
	unsigned int Chunkid;
	unsigned int Parentid;
	unsigned int Size;
} CHUNK_HEADER;

typedef struct grou_data {
	VECTOR Center,Localx,Localy,localz;
	QUAD   Matrix1,Matrix2,Matrix3;
} GROU_DATA;

typedef struct matl_data {
	unsigned short number;
	char  shader, facet, autofacet;
	float red, green, blue, opacity;
	float ambient, specular, hilight, refraction;
} MATL_DATA;

typedef struct matlt_data {
	float uoffset, voffset, urepeat, vrepeat;
} MATLT_DATA;

typedef struct polh_data {
	VECTOR Center,Localx,Localy,localz;
	QUAD   Matrix1,Matrix2,Matrix3;
} POLH_DATA;


typedef struct cvpnt {
	float  x,y,z;
} CVPNT;

typedef struct cuv {
	float  u,v;
} CUV;

typedef struct cindex {
	long  v,uv;
} CINDEX;

typedef struct cface {
	unsigned int n_index;
	unsigned int segment;
	unsigned int chunk;
	unsigned int chunkmatl;
	char facet;
	unsigned int material;
	unsigned int polytype;
	unsigned int matlindex;
	CINDEX index[25];
} CFACE;

typedef struct cmodel
{
	unsigned int Vcount;
	CVPNT Vpoint[VEC_MAX];
	unsigned int Vpntseg[VEC_MAX];
	unsigned int Vpntchunk[VEC_MAX];
	unsigned int Vpntreal[VEC_MAX];
	unsigned short Vpntfinal[VEC_MAX];
	unsigned int UVcount;
	CUV UVpoint[VEC_MAX*2];
	unsigned int Pcount;
	CFACE Poly[POLY_MAX];
	unsigned int Scount;
	char Sname[SOBJ_MAX][30];
	CVPNT Soffset[SOBJ_MAX];
	unsigned int parent[SOBJ_MAX];
} CMODEL;


/* model stuff... from fs-extra.h*/

#define D_EOF           0   //eof
#define D_DEFPOINTS		1	 //defpoints
#define D_FLATPOLY		2	 //flat-shaded polygon
#define D_TMAPPOLY		3	 //texture-mapped polygon
#define D_SORTNORM		4	 //sort by normal
#define D_BOUNDBOX      5   //bounding box

#define ID_PSPO 0x4f505350L   //pof file id
#define ID_OHDR 0x5244484fL	//Object header
#define ID_SOBJ 0x4a424f53L	//Subobject header
#define ID_TXTR 0x52545854L	//Texture filename list

#define ID_PATH 0x48544150L	//Texture filename list
#define ID_SPCL 0x4C435053L	//Texture filename list
#define ID_SHLD 0x444C4853L	//Texture filename list
#define ID__EYE 0x45594520L	//Texture filename list
#define ID_GPNT 0x544E5047L	//Texture filename list
#define ID_MPNT 0x544E504DL	//Texture filename list
#define ID_TGUN 0x4E554754L	//Texture filename list
#define ID_TMIS 0x53494D54L	//Texture filename list
#define ID_DOCK 0x4B434F44L	//Texture filename list
#define ID_FUEL 0x4C455546L	//Texture filename list
#define ID_PINF 0x464E4950L	//Texture filename list

typedef struct vpnt {
	float  x,y,z;
} VPNT;

typedef struct bbinfo {
	VPNT  min,max;
} BBINFO;

typedef struct pinfo {
	unsigned int Corners;
	unsigned int Colors;
	unsigned int facet;
	unsigned int Segment;
	unsigned int Chunk;
	unsigned int Sobj;
	unsigned int Ptype;
	unsigned int detail;
	FS_VPNT Center;
	FS_VPNT Normal;
	float Radius;
	unsigned short Vp[20];
	unsigned short Np[20];
	float U[20];
	float V[20];
} PINFO;

typedef struct fsmodel {
	unsigned short NScount;
	unsigned short NSobj[VEC_MAX*2];
	unsigned short VScount;
	unsigned short VSobj[VEC_MAX];

	unsigned short Vcount;
	FS_VPNT Vpoint[VEC_MAX];
	unsigned int Vpntseg[VEC_MAX];
	unsigned int Vpntchunk[VEC_MAX];
	unsigned char n_vn[VEC_MAX];
	unsigned short Ncount;
	FS_VPNT Npoint[VEC_MAX*2];
	unsigned int BBcount;
	BBINFO Bbox[POLY_MAX];
	unsigned int Pcount;
	PINFO Poly[POLY_MAX];
	unsigned int Scount;
	char Sname[MAX_FS_SOBJ][30];
	long Sparent[MAX_FS_SOBJ];
	FS_VPNT Soffset[MAX_FS_SOBJ];
	unsigned int Debriscount;
	unsigned int Sdebris[MAX_FS_SOBJ];
	unsigned int Detailcount;
	unsigned int Sdetail[MAX_FS_SOBJ];
	long shield;
} FSMODEL;

typedef struct sinfo {
	unsigned int VScount;
	unsigned int VSlist[SHIELD_MAX];
	unsigned int PScount;
	unsigned int PSlist[SHIELD_MAX];
	unsigned int PSnext[SHIELD_MAX][3];
} SINFO;

MATL_DATA Matldata[MATL_MAX];
MATLT_DATA Matltdata[MATL_MAX];
char Matlname[MATL_MAX][100];
unsigned int Matlcount,numtextures;

// Place to hold the model data...
FSMODEL FSModel;
CMODEL Cmodel;
unsigned int chunknum;

unsigned char *OutputData;
unsigned int OutputLength;

};
//
//  end...
//
//#pragma pack( pop )
